#include "NFCCoordinator.h"

NFCCoordinator::NFCCoordinator(Adafruit_PN532* nfcModule, int irq, int reset)
    : nfc(nfcModule), irqPin(irq), resetPin(reset), currentMode(MODE_AUTHENTICATION),
      modeStartTime(0), currentState(STATE_IDLE), irqCurr(HIGH), irqPrev(HIGH) {
}

bool NFCCoordinator::initialize() {
    pinMode(irqPin, INPUT_PULLUP);
    
    Serial.println("NFC Coordinator: Initializing...");
    
    // 初始化PN532
    nfc->begin();
    
    uint32_t versiondata = nfc->getFirmwareVersion();
    if (!versiondata) {
        Serial.println("NFC Coordinator: PN532 not found");
        return false;
    }
    
    Serial.print("NFC Coordinator: Found chip PN5");
    Serial.println((versiondata >> 24) & 0xFF, HEX);
    
    // 配置PN532为读取RFID标签
    nfc->SAMConfig();
    
    Serial.println("NFC Coordinator: Initialized successfully");
    return true;
}

bool NFCCoordinator::requestManagementMode() {
    if (currentMode == MODE_MANAGEMENT) {
        return true; // 已经在管理模式
    }
    
    Serial.println("NFC Coordinator: Entering management mode");
    currentMode = MODE_MANAGEMENT;
    modeStartTime = millis();
    currentState = STATE_IDLE;
    
    return true;
}

void NFCCoordinator::exitManagementMode() {
    if (currentMode == MODE_MANAGEMENT) {
        Serial.println("NFC Coordinator: Exiting management mode");
        currentMode = MODE_AUTHENTICATION;
        currentState = STATE_IDLE;
    }
}

bool NFCCoordinator::hasCardDetected() {
    if (currentMode != MODE_AUTHENTICATION) {
        return false; // 只在认证模式下响应
    }
    
    return handleCardDetection();
}

bool NFCCoordinator::hasCardDetectedForManagement() {
    if (currentMode != MODE_MANAGEMENT) {
        return false; // 只在管理模式下响应
    }

    return handleCardDetection();
}

bool NFCCoordinator::isCardPersistentlyPresent() {
    if (currentMode != MODE_AUTHENTICATION) {
        return false; // 只在认证模式下有意义
    }

    return currentState == STATE_CARD_PRESENT;
}

bool NFCCoordinator::handleCardDetection() {
    switch (currentState) {
        case STATE_IDLE:
            // 启动被动检测
            if (startPassiveDetection()) {
                // 立即检测到卡片
                currentState = STATE_CARD_PRESENT;
                return true;
            } else {
                // 进入检测模式
                currentState = STATE_DETECTING;
                return false;
            }
            
        case STATE_DETECTING:
            // 检查IRQ引脚
            irqCurr = digitalRead(irqPin);
            if (irqCurr == LOW && irqPrev == HIGH) {
                irqPrev = irqCurr;
                currentState = STATE_CARD_PRESENT;
                return true;
            }
            irqPrev = irqCurr;
            return false;
            
        case STATE_CARD_PRESENT:
            // 卡片已存在，等待移开
            if (digitalRead(irqPin) == HIGH) {
                currentState = STATE_IDLE;
            }
            return false;
            
        default:
            return false;
    }
}

bool NFCCoordinator::startPassiveDetection() {
    // 根据PN532基本原则：
    // 返回true表示立即检测到卡片，不需要IRQ
    // 返回false表示进入轮询模式，需要通过IRQ判断
    return nfc->startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

bool NFCCoordinator::readCardUID(uint8_t* uid, uint8_t* uidLength) {
    return nfc->readDetectedPassiveTargetID(uid, uidLength);
}

bool NFCCoordinator::authenticateBlock(uint8_t* uid, uint8_t uidLength, uint8_t blockNumber, uint8_t* key) {
    return nfc->mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, key);
}

bool NFCCoordinator::writeDataBlock(uint8_t blockNumber, uint8_t* data) {
    return nfc->mifareclassic_WriteDataBlock(blockNumber, data);
}

NFCCoordinator::NFCMode NFCCoordinator::getCurrentMode() const {
    return currentMode;
}

void NFCCoordinator::handleNFC() {
    // 处理管理模式超时
    if (currentMode == MODE_MANAGEMENT) {
        handleManagementTimeout();
    }
}

void NFCCoordinator::handleManagementTimeout() {
    if (millis() - modeStartTime > MANAGEMENT_TIMEOUT) {
        Serial.println("NFC Coordinator: Management mode timeout, returning to authentication mode");
        exitManagementMode();
    }
}

void NFCCoordinator::reset() {
    currentMode = MODE_AUTHENTICATION;
    currentState = STATE_IDLE;
    irqCurr = irqPrev = HIGH;
    modeStartTime = 0;
}
