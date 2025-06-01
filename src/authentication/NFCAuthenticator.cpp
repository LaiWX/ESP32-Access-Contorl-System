#include "NFCAuthenticator.h"

NFCAuthenticator::NFCAuthenticator(Adafruit_PN532* nfcModule, CardDatabase* db, int irq, int reset)
    : nfc(nfcModule), cardDatabase(db), irqPin(irq), resetPin(reset),
      currentState(NFC_IDLE), lastCardTime(0), lastCardUID(""),
      irqCurr(HIGH), irqPrev(HIGH), operationStartTime(0), targetUID(""),
      operationCompleted(false), operationSuccess(false), currentOperation(OP_NONE) {
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

void NFCAuthenticator::startOperationListening() {
    // 重置IRQ状态
    irqPrev = irqCurr = HIGH;

    // 启动被动检测
    if (!nfc->startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
        // 返回false：没有立即检测到卡片，进入被动监听状态
        Serial.println("NFC: Waiting for card...");
        // 注意：不改变currentState，保持在NFC_REGISTERING或NFC_ERASING状态
    } else {
        // 返回true：卡片在极短时间内就被读到，立即处理
        Serial.println("NFC: Card detected immediately");

        // 根据当前操作类型处理
        if (currentState == NFC_REGISTERING) {
            handleRegistration();
        } else if (currentState == NFC_ERASING) {
            handleErase();
        }
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

        case NFC_REGISTERING:
            // 注册状态，检查超时
            if (millis() - operationStartTime > OPERATION_TIMEOUT_MS) {
                Serial.println("NFC: Registration timeout");
                operationSuccess = false;
                operationCompleted = true;
                currentState = NFC_IDLE;
                return false;
            }

            // 检测IRQ引脚状态
            irqCurr = digitalRead(irqPin);
            if (irqCurr == LOW && irqPrev == HIGH) {
                irqPrev = irqCurr;
                // 在注册状态下检测到卡片，处理注册
                handleRegistration();
                return false;
            }
            irqPrev = irqCurr;
            return false;

        case NFC_ERASING:
            // 擦除状态，检查超时
            if (millis() - operationStartTime > OPERATION_TIMEOUT_MS) {
                Serial.println("NFC: Erase timeout");
                operationSuccess = false;
                operationCompleted = true;
                currentState = NFC_IDLE;
                return false;
            }

            // 检测IRQ引脚状态
            irqCurr = digitalRead(irqPin);
            if (irqCurr == LOW && irqPrev == HIGH) {
                irqPrev = irqCurr;
                // 在擦除状态下检测到卡片，处理擦除
                handleErase();
                return false;
            }
            irqPrev = irqCurr;
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
    operationStartTime = 0;
    targetUID = "";
    operationCompleted = false;
    operationSuccess = false;
    currentOperation = OP_NONE;
}

bool NFCAuthenticator::registerNewCard() {
    Serial.println("NFC: Tap blank card to register (10s timeout)");

    // 进入注册状态
    currentState = NFC_REGISTERING;
    operationStartTime = millis();
    currentOperation = OP_REGISTER;

    // 启动NFC监听（专门用于操作）
    startOperationListening();

    return true; // 返回true表示已启动注册流程
}

void NFCAuthenticator::handleRegistration() {
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // 读取卡片UID
    if (!nfc->readDetectedPassiveTargetID(uid, &uidLength)) {
        Serial.println("NFC: Failed to read card during registration");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    String uidString = Utils::uidToString(uid, uidLength);
    Serial.print("NFC: Registration UID: ");
    Serial.println(uidString);

    // 检查卡片是否已注册
    if (cardDatabase->isCardRegistered(uidString)) {
        Serial.println("NFC: Card already registered");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 使用默认密钥认证
    if (!authenticateBlock(uid, uidLength, SECTOR_TRAILER_BLOCK, defaultKey)) {
        Serial.println("NFC: Authentication with default key failed");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 生成并写入新密钥
    uint8_t newKey[Utils::KEY_SIZE];
    Utils::generateRandomKey(newKey);

    if (!writeSectorTrailer(newKey)) {
        Serial.println("NFC: Failed to write sector trailer");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 添加到数据库
    String keyHex = Utils::keyToHexString(newKey);
    if (cardDatabase->addCard(uidString, keyHex)) {
        Serial.println("NFC: Card registered successfully");
        operationSuccess = true;
    } else {
        Serial.println("NFC: Failed to save card to database");
        operationSuccess = false;
    }

    // 标记操作完成并重置状态
    operationCompleted = true;
    currentState = NFC_IDLE;
}

bool NFCAuthenticator::eraseCard(const String& uid) {
    Serial.println("NFC: Tap card " + uid + " to erase (10s timeout)");

    // 进入擦除状态
    currentState = NFC_ERASING;
    operationStartTime = millis();
    targetUID = uid;
    currentOperation = OP_ERASE;

    // 启动NFC监听（专门用于操作）
    startOperationListening();

    return true; // 返回true表示已启动擦除流程
}

void NFCAuthenticator::handleErase() {
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // 读取卡片UID
    if (!nfc->readDetectedPassiveTargetID(uid, &uidLength)) {
        Serial.println("NFC: Failed to read card during erase");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    String uidString = Utils::uidToString(uid, uidLength);
    Serial.print("NFC: Erase detected UID: ");
    Serial.println(uidString);

    // 检查是否是目标卡片
    if (uidString != targetUID) {
        Serial.println("NFC: Wrong card, expected " + targetUID);
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 获取存储的密钥
    String keyHex;
    if (!cardDatabase->findCardByUID(uidString, keyHex)) {
        Serial.println("NFC: Card not found in database");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 使用存储的密钥认证
    uint8_t key[Utils::KEY_SIZE];
    Utils::hexStringToKey(keyHex, key);

    if (!authenticateBlock(uid, uidLength, SECTOR_TRAILER_BLOCK, key)) {
        Serial.println("NFC: Authentication with stored key failed");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    // 写入默认密钥（全F）
    if (!writeSectorTrailer(defaultKey)) {
        Serial.println("NFC: Failed to erase card key");
        operationSuccess = false;
        operationCompleted = true;
        currentState = NFC_IDLE;
        return;
    }

    Serial.println("NFC: Card key erased successfully");
    operationSuccess = true;

    // 标记操作完成并重置状态
    operationCompleted = true;
    currentState = NFC_IDLE;
}

bool NFCAuthenticator::isOperationCompleted() const {
    return operationCompleted;
}

bool NFCAuthenticator::getOperationResult() const {
    return operationSuccess;
}

NFCAuthenticator::OperationType NFCAuthenticator::getCurrentOperation() const {
    return currentOperation;
}

String NFCAuthenticator::getTargetUID() const {
    return targetUID;
}

void NFCAuthenticator::clearOperationFlag() {
    operationCompleted = false;
    operationSuccess = false;
    currentOperation = OP_NONE;
}
