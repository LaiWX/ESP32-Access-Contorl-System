#include "NFCCardManagerImpl.h"
#include <vector>

NFCCardManagerImpl::NFCCardManagerImpl(NFCCoordinator* coordinator, CardDatabase* db,
                                       FileSystemManager* fsManager, DoorAccessExecutor* executor)
    : nfcCoordinator(coordinator), cardDatabase(db), fileSystemManager(fsManager), doorExecutor(executor),
      currentState(NFC_IDLE), currentOperation(OP_NONE),
      operationCompleted(false), operationSuccess(false), operationStartTime(0),
      lastOperationTime(0) {
}

bool NFCCardManagerImpl::registerNew() {
    if (currentState != NFC_IDLE) {
        Serial.println("Card Manager: Operation already in progress");
        return false;
    }

    Serial.println("Card Manager: Tap new card to register (10s timeout)");

    // 请求进入管理模式
    if (!nfcCoordinator->requestManagementMode()) {
        Serial.println("Card Manager: Failed to enter management mode");
        return false;
    }

    // 进入注册状态
    currentState = NFC_REGISTERING;
    operationStartTime = millis();
    currentOperation = OP_REGISTER;

    return true;
}

bool NFCCardManagerImpl::deleteItem(const String& uid) {
    if (uid.length() == 0) {
        Serial.println("Usage: del <UID>");
        return false;
    }

    if (cardDatabase->removeCard(uid)) {
        if (fileSystemManager->saveCards()) {
            Serial.println("Deleted " + uid);
            doorExecutor->executeDeletionSuccessAction();
            return true;
        } else {
            Serial.println("Failed to save changes to file system");
            return false;
        }
    } else {
        Serial.println("Card not found: " + uid);
        return false;
    }
}

bool NFCCardManagerImpl::eraseAndDeleteItem(const String& uid) {
    if (uid.length() == 0) {
        Serial.println("Usage: erase <UID>");
        return false;
    }

    if (currentState != NFC_IDLE) {
        Serial.println("Card Manager: Operation already in progress");
        return false;
    }

    // 检查卡片是否存在于数据库中
    if (!cardDatabase->isCardRegistered(uid)) {
        Serial.println("Card not found in database: " + uid);
        return false;
    }

    Serial.println("Card Manager: Tap card " + uid + " to erase (10s timeout)");

    // 请求进入管理模式
    if (!nfcCoordinator->requestManagementMode()) {
        Serial.println("Card Manager: Failed to enter management mode");
        return false;
    }

    // 进入擦除状态
    currentState = NFC_ERASING;
    operationStartTime = millis();
    targetUID = uid;
    currentOperation = OP_ERASE;

    return true;
}

void NFCCardManagerImpl::listRegisteredItems() {
    Serial.println("=== Registered Cards ===");
    JsonArray cards = cardDatabase->getCards();

    if (cards.size() == 0) {
        Serial.println("No cards registered");
    } else {
        for (size_t i = 0; i < cards.size(); i++) {
            JsonObject card = cards[i];
            Serial.print(i + 1);
            Serial.print(". ");
            Serial.println(card["uid"].as<String>());
        }
    }
    Serial.println("========================");
}

bool NFCCardManagerImpl::hasOngoingOperation() {
    return currentState != NFC_IDLE;
}

void NFCCardManagerImpl::handleOperations() {
    if (currentState == NFC_IDLE) {
        return;
    }

    // 检查超时
    if (millis() - operationStartTime > OPERATION_TIMEOUT) {
        handleOperationTimeout();
        return;
    }

    // 处理卡片检测
    handleCardDetection();

    // 处理具体操作
    if (currentState == NFC_REGISTERING) {
        processRegistration();
    } else if (currentState == NFC_ERASING) {
        processErasure();
    }

    // 检查操作是否完成
    if (operationCompleted) {
        if (operationSuccess) {
            // 保存到文件系统
            if (fileSystemManager->saveCards()) {
                if (currentOperation == OP_REGISTER) {
                    doorExecutor->executeRegistrationSuccessAction();
                } else if (currentOperation == OP_ERASE) {
                    // 从数据库删除卡片
                    if (cardDatabase->removeCard(targetUID)) {
                        Serial.println("Card " + targetUID + " deleted from database");
                    }
                    doorExecutor->executeDeletionSuccessAction();
                }
            } else {
                Serial.println("Failed to save changes to file system");
            }
        }
        
        // 重置状态
        resetOperationState();
    }
}

void NFCCardManagerImpl::reset() {
    resetOperationState();
    lastOperationTime = 0;
    lastCardUID = "";
}

bool NFCCardManagerImpl::startOperationListening() {
    // NFC协调器已经处理了被动检测
    Serial.println("Card Manager: Waiting for card...");
    currentState = NFC_DETECTING;
    return true;
}

void NFCCardManagerImpl::handleOperationTimeout() {
    Serial.println("Card Manager: Operation timeout");
    resetOperationState();
}

void NFCCardManagerImpl::handleCardDetection() {
    if (currentState == NFC_DETECTING) {
        // 使用NFC协调器检查卡片
        if (nfcCoordinator->hasCardDetectedForManagement()) {
            Serial.println("Card Manager: Card detected via coordinator");
            currentState = NFC_CARD_PRESENT;
        }
    }
}

void NFCCardManagerImpl::processRegistration() {
    if (currentState != NFC_CARD_PRESENT) {
        return;
    }

    uint8_t uid[7];
    uint8_t uidLength;
    
    // 读取卡片UID
    if (!nfcCoordinator->readCardUID(uid, &uidLength)) {
        Serial.println("Card Manager: Failed to read card UID");
        resetOperationState();
        return;
    }

    String uidString = Utils::uidToString(uid, uidLength);
    Serial.println("Card Manager: Registering card: " + uidString);

    // 检查卡片是否已注册
    if (cardDatabase->isCardRegistered(uidString)) {
        Serial.println("Card Manager: Card already registered");
        operationCompleted = true;
        operationSuccess = false;
        return;
    }

    // 生成新密钥
    uint8_t newKey[6];
    generateRandomKey(newKey);

    // 写入密钥到卡片
    if (!writeKeyToCard(uid, uidLength, newKey)) {
        Serial.println("Card Manager: Failed to write key to card");
        operationCompleted = true;
        operationSuccess = false;
        return;
    }

    // 添加到数据库
    String keyHex = Utils::keyToHexString(newKey);
    if (cardDatabase->addCard(uidString, keyHex)) {
        Serial.println("Card Manager: Card registered successfully");
        operationCompleted = true;
        operationSuccess = true;
    } else {
        Serial.println("Card Manager: Failed to save card to database");
        operationCompleted = true;
        operationSuccess = false;
    }
}

void NFCCardManagerImpl::processErasure() {
    if (currentState != NFC_CARD_PRESENT) {
        return;
    }

    uint8_t uid[7];
    uint8_t uidLength;
    
    // 读取卡片UID
    if (!nfcCoordinator->readCardUID(uid, &uidLength)) {
        Serial.println("Card Manager: Failed to read card UID");
        resetOperationState();
        return;
    }

    String uidString = Utils::uidToString(uid, uidLength);

    // 检查是否是目标卡片
    if (uidString != targetUID) {
        Serial.println("Card Manager: Wrong card. Expected: " + targetUID + ", Got: " + uidString);
        resetOperationState();
        return;
    }

    Serial.println("Card Manager: Erasing card: " + uidString);

    // 擦除卡片密钥
    if (eraseKeyFromCard(uid, uidLength)) {
        Serial.println("Card Manager: Card erased successfully");
        operationCompleted = true;
        operationSuccess = true;
    } else {
        Serial.println("Card Manager: Failed to erase card");
        operationCompleted = true;
        operationSuccess = false;
    }
}

bool NFCCardManagerImpl::authenticateCard(uint8_t* uid, uint8_t uidLength, uint8_t* key) {
    // 使用密钥认证扇区
    if (!nfcCoordinator->authenticateBlock(uid, uidLength, AUTH_BLOCK, key)) {
        return false;
    }
    return true;
}

bool NFCCardManagerImpl::writeKeyToCard(uint8_t* uid, uint8_t uidLength, uint8_t* newKey) {
    // 使用默认密钥尝试认证
    uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    if (!authenticateCard(uid, uidLength, defaultKey)) {
        Serial.println("Card Manager: Failed to authenticate with default key");
        return false;
    }

    // 准备扇区尾部数据
    uint8_t trailerData[TRAILER_SIZE];
    memcpy(trailerData, newKey, 6);      // Key A
    trailerData[6] = 0xFF;               // Access bits byte 1
    trailerData[7] = 0x07;               // Access bits byte 2
    trailerData[8] = 0x80;               // Access bits byte 3
    trailerData[9] = 0x69;               // GPB
    memcpy(trailerData + 10, newKey, 6); // Key B

    // 写入扇区尾部
    if (!nfcCoordinator->writeDataBlock(SECTOR_TRAILER_BLOCK, trailerData)) {
        Serial.println("Card Manager: Failed to write sector trailer");
        return false;
    }

    return true;
}

bool NFCCardManagerImpl::eraseKeyFromCard(uint8_t* uid, uint8_t uidLength) {
    // 获取卡片的当前密钥
    String uidString = Utils::uidToString(uid, uidLength);
    String keyHex;
    if (!cardDatabase->findCardByUID(uidString, keyHex)) {
        Serial.println("Card Manager: Card not found in database");
        return false;
    }



    uint8_t currentKey[6];
    Utils::hexStringToKey(keyHex, currentKey);

    if (!authenticateCard(uid, uidLength, currentKey)) {
        Serial.println("Card Manager: Failed to authenticate with stored key");
        return false;
    }

    // 恢复为默认密钥
    uint8_t defaultKey[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    uint8_t trailerData[TRAILER_SIZE];
    memcpy(trailerData, defaultKey, 6);   // Key A
    trailerData[6] = 0xFF;                // Access bits byte 1
    trailerData[7] = 0x07;                // Access bits byte 2
    trailerData[8] = 0x80;                // Access bits byte 3
    trailerData[9] = 0x69;                // GPB
    memcpy(trailerData + 10, defaultKey, 6); // Key B

    // 写入扇区尾部
    if (!nfcCoordinator->writeDataBlock(SECTOR_TRAILER_BLOCK, trailerData)) {
        Serial.println("Card Manager: Failed to restore default key");
        return false;
    }

    return true;
}

void NFCCardManagerImpl::generateRandomKey(uint8_t* key) {
    for (int i = 0; i < 6; i++) {
        key[i] = random(0, 256);
    }
}

void NFCCardManagerImpl::resetOperationState() {
    currentState = NFC_IDLE;
    currentOperation = OP_NONE;
    operationCompleted = false;
    operationSuccess = false;
    operationStartTime = 0;
    targetUID = "";

    // 退出管理模式
    nfcCoordinator->exitManagementMode();
}

const char* NFCCardManagerImpl::getName() const {
    return "NFC Card Manager";
}
