#include "NFCManager.h"

NFCManager::NFCManager(int irq, int reset)
    : nfc(nullptr), irqPin(irq), resetPin(reset), currentState(STATE_IDLE),
      irqCurr(HIGH), irqPrev(HIGH), lastDetectionTime(0) {
}

bool NFCManager::initialize() {
    nfc = new Adafruit_PN532(irqPin, resetPin);

    pinMode(irqPin, INPUT_PULLUP);
    
    Serial.println("NFC Manager: Initializing...");
    
    // 初始化PN532
    nfc->begin();
    
    uint32_t versionData = nfc->getFirmwareVersion();
    if (!versionData) {
        Serial.println("NFC Manager: PN532 not found");
        return false;
    }
    
    Serial.print("NFC Manager: Found chip PN5");
    Serial.println((versionData >> 24) & 0xFF, HEX);
    
    // 配置PN532为读取RFID标签
    nfc->SAMConfig();
    
    Serial.println("NFC Manager: Initialized successfully");
    return true;
}

NFCManager::CardDetectionResult NFCManager::detectCard() {
    switch (currentState) {
        case STATE_IDLE:
        case STATE_CARD_PRESENT:
            // 启动被动检测
            if (startPassiveDetection()) {
                // 立即检测到卡片，立即读走卡片UID
                uint8_t uid[7] = {0};
                uint8_t uidLength = 0;
                readCardUID(uid, &uidLength);

                Serial.println("NFC Manager: Card detected immediately");
                currentState = STATE_CARD_PRESENT;
                lastDetectionTime = millis();
                
                // 延迟一段时间，避免立即重复检测
                delay(CARD_PERSISTENCE_DELAY);
                return CARD_PERSISTENT;
            } else {
                // 进入检测模式，等待IRQ
                currentState = STATE_DETECTING;
                return NO_CARD;
            }
            
        case STATE_DETECTING:
            // 检查IRQ引脚下降沿
            if (checkIRQFallingEdge()) {
                Serial.println("NFC Manager: Card detected via IRQ");
                currentState = STATE_CARD_PRESENT;
                lastDetectionTime = millis();
                return CARD_DETECTED;
            }
            return NO_CARD;

        default:
            return NO_CARD;
    }
}

bool NFCManager::readCardUID(uint8_t* uid, uint8_t* uidLength) {
    return nfc->readDetectedPassiveTargetID(uid, uidLength);
}

bool NFCManager::authenticateBlock(uint8_t* uid, uint8_t uidLength, uint8_t blockNumber, uint8_t* key) {
    return nfc->mifareclassic_AuthenticateBlock(uid, uidLength, blockNumber, 0, key);
}

bool NFCManager::writeDataBlock(uint8_t blockNumber, uint8_t* data) {
    return nfc->mifareclassic_WriteDataBlock(blockNumber, data);
}

void NFCManager::reset() {
    currentState = STATE_IDLE;
    irqCurr = irqPrev = HIGH;
    lastDetectionTime = 0;
    Serial.println("NFC Manager: Reset completed");
}

bool NFCManager::getIRQState() const {
    return digitalRead(irqPin) == LOW;
}

bool NFCManager::startPassiveDetection() {
    // 根据PN532基本原则：
    // 返回true表示立即检测到卡片，不需要IRQ
    // 返回false表示进入轮询模式，需要通过IRQ判断
    return nfc->startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A);
}

bool NFCManager::checkIRQFallingEdge() {
    irqCurr = digitalRead(irqPin);
    bool fallingEdge = (irqCurr == LOW && irqPrev == HIGH);
    irqPrev = irqCurr;
    return fallingEdge;
}
