#include "NFCAuthenticator.h"

NFCAuthenticator::NFCAuthenticator(Adafruit_PN532* nfcModule, CardDatabase* db, int irq, int reset)
    : nfc(nfcModule), cardDatabase(db), irqPin(irq), resetPin(reset),
      currentState(NFC_IDLE), lastCardTime(0), lastCardUID(""),
      irqCurr(HIGH), irqPrev(HIGH) {
}

bool NFCAuthenticator::initialize() {
    nfc->begin();
    uint32_t version = nfc->getFirmwareVersion();
    if (!version) {
        Serial.println("PN532 not found");
        return false;
    }
    
    Serial.print("Found PN532 with firmware version: 0x");
    Serial.println(version, HEX);
    
    nfc->SAMConfig();
    
    // 配置IRQ引脚
    pinMode(irqPin, INPUT_PULLUP);
    irqPrev = irqCurr = digitalRead(irqPin);
    
    // 启动第一次NFC监听
    startNFCListening();
    
    return true;
}

bool NFCAuthenticator::readCardUID(uint8_t* uid, uint8_t* uidLen) {
    return nfc->readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLen);
}

bool NFCAuthenticator::authenticateBlock(uint8_t* uid, uint8_t uidLen, uint8_t blockNumber, uint8_t* key) {
    return nfc->mifareclassic_AuthenticateBlock(uid, uidLen, blockNumber, 0, key);
}

bool NFCAuthenticator::writeSectorTrailer(uint8_t* newKey) {
    uint8_t trailer[TRAILER_SIZE];
    
    // Set new KeyA
    memcpy(trailer, newKey, Utils::KEY_SIZE);
    
    // Set default access bits
    memcpy(trailer + 6, "\xFF\x07\x80\x69", 4);
    
    // Keep default KeyB
    memcpy(trailer + 10, defaultKey, Utils::KEY_SIZE);
    
    return nfc->mifareclassic_WriteDataBlock(SECTOR_TRAILER_BLOCK, trailer);
}

void NFCAuthenticator::startNFCListening() {
    // 重置IRQ状态
    irqPrev = irqCurr = HIGH;
    
    // 启动被动检测
    if (!nfc->startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
        // 返回false：没有立即检测到卡片，进入被动监听状态
        currentState = NFC_DETECTING;
        Serial.println("NFC: Waiting for card...");
    } else {
        // 返回true：卡片在极短时间内就被读到（卡片未移开）
        currentState = NFC_CARD_PRESENT;
        Serial.println("NFC: Card already present, ignoring...");
        
        // 直接读取卡片但不处理认证
        uint8_t uid[7] = {0};
        uint8_t uidLength = 0;
        if (nfc->readDetectedPassiveTargetID(uid, &uidLength)) {
            String uidString = Utils::uidToString(uid, uidLength);
            Serial.print("NFC: Ignored card: ");
            Serial.println(uidString);
        }
        delay(100);
    }
}

bool NFCAuthenticator::handleCardAuthentication(uint8_t* uid, uint8_t uidLength) {
    String uidString = Utils::uidToString(uid, uidLength);
    
    // 检查是否在冷却期内（同一张卡连续认证）
    if (uidString == lastCardUID && (millis() - lastCardTime) < CARD_COOLDOWN_MS) {
        Serial.println("NFC: Card in cooldown, ignored.");
        return false;
    }
    
    Serial.print("NFC: Card detected: ");
    Serial.println(uidString);
    
    // 在数据库中查找卡片
    String keyHex;
    if (!cardDatabase->findCardByUID(uidString, keyHex)) {
        Serial.println("NFC: Card not registered");
        return false;
    }
    
    // 获取存储的密钥并认证
    uint8_t key[Utils::KEY_SIZE];
    Utils::hexStringToKey(keyHex, key);
    
    if (authenticateBlock(uid, uidLength, AUTH_BLOCK, key)) {
        Serial.println("NFC: Authentication successful");
        
        // 更新最后认证的卡片和时间
        lastCardUID = uidString;
        lastCardTime = millis();
        return true;
    } else {
        Serial.println("NFC: Authentication failed");
        return false;
    }
}

bool NFCAuthenticator::hasAuthenticationRequest() {
    switch (currentState) {
        case NFC_IDLE:
            // 空闲状态，启动NFC监听
            startNFCListening();
            return false;

        case NFC_DETECTING:
            // 检测IRQ引脚状态
            irqCurr = digitalRead(irqPin);

            // 当IRQ引脚从高变低时，表示卡片靠近
            if (irqCurr == LOW && irqPrev == HIGH) {
                irqPrev = irqCurr;
                return true;
            }

            irqPrev = irqCurr;
            return false;

        case NFC_CARD_PRESENT:
            // 卡片持续存在状态，等待卡片移开
            if (digitalRead(irqPin) == HIGH) {
                currentState = NFC_IDLE;
            }
            return false;

        default:
            return false;
    }
}

bool NFCAuthenticator::authenticate() {
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // 读取卡片UID
    if (nfc->readDetectedPassiveTargetID(uid, &uidLength)) {
        bool result = handleCardAuthentication(uid, uidLength);
        // 重置状态，准备下一次检测
        currentState = NFC_IDLE;
        return result;
    }

    // 读取失败，重置状态
    currentState = NFC_IDLE;
    return false;
}

const char* NFCAuthenticator::getName() const {
    return "NFC Authenticator";
}

void NFCAuthenticator::reset() {
    currentState = NFC_IDLE;
    lastCardTime = 0;
    lastCardUID = "";
    irqCurr = irqPrev = HIGH;
}

bool NFCAuthenticator::registerNewCard() {
    Serial.println("NFC: Tap blank card to register");

    uint8_t uid[7], uidLen;
    if (!readCardUID(uid, &uidLen)) {
        Serial.println("NFC: No card detected");
        return false;
    }

    String uidString = Utils::uidToString(uid, uidLen);
    Serial.print("NFC: UID: ");
    Serial.println(uidString);

    // 检查卡片是否已注册
    if (cardDatabase->isCardRegistered(uidString)) {
        Serial.println("NFC: Card already registered");
        return false;
    }

    // 使用默认密钥认证
    if (!authenticateBlock(uid, uidLen, SECTOR_TRAILER_BLOCK, defaultKey)) {
        Serial.println("NFC: Authentication with default key failed");
        return false;
    }

    // 生成并写入新密钥
    uint8_t newKey[Utils::KEY_SIZE];
    Utils::generateRandomKey(newKey);

    if (!writeSectorTrailer(newKey)) {
        Serial.println("NFC: Failed to write sector trailer");
        return false;
    }

    // 添加到数据库
    String keyHex = Utils::keyToHexString(newKey);
    if (cardDatabase->addCard(uidString, keyHex)) {
        Serial.println("NFC: Card registered successfully");
        return true;
    } else {
        Serial.println("NFC: Failed to save card to database");
        return false;
    }
}
