#include "NFCAuthenticator.h"

NFCAuthenticator::NFCAuthenticator(NFCManager* manager, CardDatabase* db)
    : nfcManager(manager), cardDatabase(db), lastCardTime(0), lastCardUID("") {
}

bool NFCAuthenticator::initialize() {
    // NFC管理器已经初始化了PN532
    Serial.println("NFC: Authenticator initialized");
    return true;
}

bool NFCAuthenticator::readCardUID(uint8_t* uid, uint8_t* uidLen) {
    return nfcManager->readCardUID(uid, uidLen);
}

bool NFCAuthenticator::authenticateBlock(uint8_t* uid, uint8_t uidLen, uint8_t blockNumber, uint8_t* key) {
    return nfcManager->authenticateBlock(uid, uidLen, blockNumber, key);
}

void NFCAuthenticator::startNFCListening() {
    // 由NFC管理器处理
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
    // 使用新的NFCManager检测卡片
    NFCManager::CardDetectionResult result = nfcManager->detectCard();

    // 只有检测到新卡片时才返回true
    return result == NFCManager::CARD_DETECTED;
}

bool NFCAuthenticator::authenticate() {
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;

    // 读取卡片UID
    if (nfcManager->readCardUID(uid, &uidLength)) {
        return handleCardAuthentication(uid, uidLength);
    }

    return false;
}

const char* NFCAuthenticator::getName() const {
    return "NFC Authenticator";
}

void NFCAuthenticator::reset() {
    lastCardTime = 0;
    lastCardUID = "";
}
