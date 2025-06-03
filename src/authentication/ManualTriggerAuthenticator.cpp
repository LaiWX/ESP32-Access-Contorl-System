#include "ManualTriggerAuthenticator.h"

ManualTriggerAuthenticator::ManualTriggerAuthenticator(int pin) 
    : triggerPin(pin), lastPinState(HIGH), lastTriggerTime(0) {
}

bool ManualTriggerAuthenticator::initialize() {
    pinMode(triggerPin, INPUT_PULLUP);
    lastPinState = digitalRead(triggerPin);
    lastTriggerTime = 0;
    
    Serial.print("Manual trigger initialized on pin ");
    Serial.println(triggerPin);
    
    return true;
}

bool ManualTriggerAuthenticator::hasAuthenticationRequest() {
    int currentPinState = digitalRead(triggerPin);
    unsigned long currentTime = millis();
    
    // 检测下降沿（从HIGH到LOW）
    if (lastPinState == HIGH && currentPinState == LOW) {
        // 防抖动检查
        if (currentTime - lastTriggerTime > DEBOUNCE_DELAY) {
            lastTriggerTime = currentTime;
            lastPinState = currentPinState;
            Serial.println("Manual trigger: Falling edge detected");
            return true;
        }
    }
    
    lastPinState = currentPinState;
    return false;
}

bool ManualTriggerAuthenticator::authenticate() {
    // 手动触发器（室内按钮）应该总是成功
    // 这是为了紧急情况或内部人员使用
    Serial.println("Manual trigger: Authentication successful (indoor button)");
    return true;
}

const char* ManualTriggerAuthenticator::getName() const {
    return "Manual Trigger Authenticator";
}

void ManualTriggerAuthenticator::reset() {
    lastPinState = digitalRead(triggerPin);
    lastTriggerTime = 0;
}
