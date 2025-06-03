#include "ServoExecutor.h"

ServoExecutor::ServoExecutor(int pin) 
    : servoPin(pin), actionStartTime(0), doorIsOpen(false), actionInProgress(false) {
}

bool ServoExecutor::initialize() {
    pinMode(servoPin, OUTPUT);
    
    // 初始化为关门状态
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;
    actionInProgress = false;
    
    Serial.print("Servo initialized on pin ");
    Serial.println(servoPin);
    
    return true;
}

void ServoExecutor::openDoor() {
    if (!actionInProgress) {
        Serial.println("Servo: Opening door");
        setServoAngle(DOOR_OPEN_ANGLE);
        doorIsOpen = true;
        actionInProgress = true;
        actionStartTime = millis();
    }
}

void ServoExecutor::closeDoor() {
    Serial.println("Servo: Closing door");
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;
    actionInProgress = false;
}

void ServoExecutor::handleServo() {
    // 自动关门逻辑
    if (doorIsOpen && actionInProgress) {
        if (millis() - actionStartTime >= DOOR_OPEN_DURATION) {
            closeDoor();
        }
    }
}

bool ServoExecutor::isDoorOpen() const {
    return doorIsOpen;
}

bool ServoExecutor::isActionInProgress() const {
    return actionInProgress;
}

void ServoExecutor::setServoAngle(int angle) {
    // 简化的PWM控制（实际项目中可能需要使用Servo库）
    // 这里只是示例实现
    int pulseWidth = map(angle, 0, 180, 1000, 2000); // 1ms到2ms脉宽
    
    // 发送PWM信号（简化版本）
    for (int i = 0; i < 10; i++) {
        digitalWrite(servoPin, HIGH);
        delayMicroseconds(pulseWidth);
        digitalWrite(servoPin, LOW);
        delayMicroseconds(20000 - pulseWidth); // 20ms周期
    }
    
    Serial.print("Servo angle set to: ");
    Serial.println(angle);
}
