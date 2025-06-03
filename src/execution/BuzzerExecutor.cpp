#include "BuzzerExecutor.h"

BuzzerExecutor::BuzzerExecutor(int pin) 
    : buzzerPin(pin), lastBeepTime(0), beepCount(0), targetBeepCount(0), 
      isBeeping(false), currentMode(BEEP_NONE) {
}

bool BuzzerExecutor::initialize() {
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);
    
    Serial.print("Buzzer initialized on pin ");
    Serial.println(buzzerPin);
    
    return true;
}

void BuzzerExecutor::beepSuccess() {
    currentMode = BEEP_SUCCESS;
    targetBeepCount = 1;
    beepCount = 0;
    lastBeepTime = millis();
    isBeeping = true;
    digitalWrite(buzzerPin, HIGH);
    Serial.println("Buzzer: Success beep");
}

void BuzzerExecutor::beepFailure() {
    currentMode = BEEP_FAILURE;
    targetBeepCount = 3;
    beepCount = 0;
    lastBeepTime = millis();
    isBeeping = true;
    digitalWrite(buzzerPin, HIGH);
    Serial.println("Buzzer: Failure beep");
}

void BuzzerExecutor::beepRegister() {
    currentMode = BEEP_REGISTER;
    targetBeepCount = 2;
    beepCount = 0;
    lastBeepTime = millis();
    isBeeping = true;
    digitalWrite(buzzerPin, HIGH);
    Serial.println("Buzzer: Register beep");
}

void BuzzerExecutor::beepDelete() {
    currentMode = BEEP_DELETE;
    targetBeepCount = 1;
    beepCount = 0;
    lastBeepTime = millis();
    isBeeping = true;
    digitalWrite(buzzerPin, HIGH);
    Serial.println("Buzzer: Delete beep");
}

void BuzzerExecutor::handleBeeping() {
    if (!isBeeping || currentMode == BEEP_NONE) {
        return;
    }
    
    unsigned long currentTime = millis();
    unsigned long beepDuration;
    
    // 确定蜂鸣持续时间
    switch (currentMode) {
        case BEEP_SUCCESS:
            beepDuration = LONG_BEEP;
            break;
        case BEEP_FAILURE:
            beepDuration = SHORT_BEEP;
            break;
        case BEEP_REGISTER:
        case BEEP_DELETE:
            beepDuration = MEDIUM_BEEP;
            break;
        default:
            beepDuration = SHORT_BEEP;
            break;
    }
    
    // 检查是否需要停止当前蜂鸣
    if (digitalRead(buzzerPin) == HIGH && (currentTime - lastBeepTime >= beepDuration)) {
        digitalWrite(buzzerPin, LOW);
        beepCount++;
        lastBeepTime = currentTime;
        
        // 检查是否完成所有蜂鸣
        if (beepCount >= targetBeepCount) {
            stopBeeping();
        }
    }
    
    // 检查是否需要开始下一次蜂鸣
    if (digitalRead(buzzerPin) == LOW && beepCount < targetBeepCount && 
        (currentTime - lastBeepTime >= BEEP_INTERVAL)) {
        digitalWrite(buzzerPin, HIGH);
        lastBeepTime = currentTime;
    }
}

void BuzzerExecutor::stopBeeping() {
    digitalWrite(buzzerPin, LOW);
    isBeeping = false;
    currentMode = BEEP_NONE;
    beepCount = 0;
    targetBeepCount = 0;
}

bool BuzzerExecutor::isActive() const {
    return isBeeping;
}
