#include "LEDExecutor.h"

LEDExecutor::LEDExecutor(int pin) : ledPin(pin) {
}

bool LEDExecutor::initialize() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    return true;
}

void LEDExecutor::blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(ledPin, HIGH);
        delay(delayMs);
        digitalWrite(ledPin, LOW);
        delay(delayMs);
    }
}

void LEDExecutor::executeSuccessAction() {
    Serial.println("Executing success action (LED)");
    blinkLED(2, 200); // 快速闪烁2次表示成功
    
    // 模拟门锁开启时间
    delay(3000);
    Serial.println("Action completed");
}

void LEDExecutor::executeFailureAction() {
    Serial.println("Executing failure action (LED)");
    blinkLED(1, 500); // 长闪1次表示失败
}

void LEDExecutor::executeRegistrationSuccessAction() {
    Serial.println("Executing registration success action (LED)");
    blinkLED(3, 100); // 闪烁3次表示注册成功
}

void LEDExecutor::executeDeletionSuccessAction() {
    Serial.println("Executing deletion success action (LED)");
    blinkLED(2, 200); // 闪烁2次表示删除成功
}

const char* LEDExecutor::getName() const {
    return "LED Executor";
}
