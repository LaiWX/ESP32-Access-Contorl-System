#include "DoorLockExecutor.h"

DoorLockExecutor::DoorLockExecutor(int lockPin, int statusLedPin) 
    : doorLockPin(lockPin), ledPin(statusLedPin) {
}

bool DoorLockExecutor::initialize() {
    // 初始化门锁控制引脚（当前注释掉，未来启用）
    // pinMode(doorLockPin, OUTPUT);
    // digitalWrite(doorLockPin, LOW); // 初始状态为锁定
    
    // 初始化状态LED
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    
    return true;
}

void DoorLockExecutor::blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(ledPin, HIGH);
        delay(delayMs);
        digitalWrite(ledPin, LOW);
        delay(delayMs);
    }
}

void DoorLockExecutor::executeSuccessAction() {
    Serial.println("Unlocking door...");
    
    // 开启门锁（当前注释掉）
    // digitalWrite(doorLockPin, HIGH);
    
    // LED指示开门成功
    blinkLED(2, 200);
    
    // 保持门锁开启状态3秒
    delay(3000);
    
    // 重新锁门
    // digitalWrite(doorLockPin, LOW);
    Serial.println("Door locked");
}

void DoorLockExecutor::executeFailureAction() {
    Serial.println("Access denied");
    blinkLED(1, 500); // 长闪1次表示失败
}

void DoorLockExecutor::executeRegistrationSuccessAction() {
    Serial.println("Card registration successful");
    blinkLED(3, 100); // 闪烁3次表示注册成功
}

void DoorLockExecutor::executeDeletionSuccessAction() {
    Serial.println("Card deletion successful");
    blinkLED(2, 200); // 闪烁2次表示删除成功
}

const char* DoorLockExecutor::getName() const {
    return "Door Lock Executor";
}
