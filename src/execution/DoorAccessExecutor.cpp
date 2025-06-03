#include "DoorAccessExecutor.h"
#include "LEDExecutor.h"
#include "BuzzerExecutor.h"
#include "ServoExecutor.h"

DoorAccessExecutor::DoorAccessExecutor(LEDExecutor* led, BuzzerExecutor* buzzer, ServoExecutor* servo)
    : ledExecutor(led), buzzerExecutor(buzzer), servoExecutor(servo),
      actionStartTime(0), actionInProgress(false) {
}

bool DoorAccessExecutor::initialize() {
    Serial.println("Initializing Door Access Executor...");
    
    bool allSuccess = true;
    
    if (ledExecutor && !ledExecutor->initialize()) {
        Serial.println("Failed to initialize LED executor");
        allSuccess = false;
    }
    
    if (buzzerExecutor && !buzzerExecutor->initialize()) {
        Serial.println("Failed to initialize Buzzer executor");
        allSuccess = false;
    }
    
    if (servoExecutor && !servoExecutor->initialize()) {
        Serial.println("Failed to initialize Servo executor");
        allSuccess = false;
    }
    
    if (allSuccess) {
        Serial.println("Door Access Executor initialized successfully");
    }
    
    return allSuccess;
}

void DoorAccessExecutor::executeSuccessAction() {
    Serial.println("Door Access: Executing success action (OPEN DOOR)");
    
    // 协调所有执行器
    if (ledExecutor) {
        ledExecutor->turnOn();
    }
    
    if (buzzerExecutor) {
        buzzerExecutor->beepSuccess();
    }
    
    if (servoExecutor) {
        servoExecutor->openDoor();
    }
    
    actionInProgress = true;
    actionStartTime = millis();
}

void DoorAccessExecutor::executeFailureAction() {
    Serial.println("Door Access: Executing failure action (ACCESS DENIED)");
    
    // 协调LED和蜂鸣器（不开门）
    if (ledExecutor) {
        ledExecutor->blink(3, 200); // 快速闪烁3次
    }
    
    if (buzzerExecutor) {
        buzzerExecutor->beepFailure();
    }
    
    // 不操作舵机
}

void DoorAccessExecutor::executeRegistrationSuccessAction() {
    Serial.println("Door Access: Executing registration success action");
    
    if (ledExecutor) {
        ledExecutor->blink(2, 500); // 慢速闪烁2次
    }
    
    if (buzzerExecutor) {
        buzzerExecutor->beepRegister();
    }
}

void DoorAccessExecutor::executeDeletionSuccessAction() {
    Serial.println("Door Access: Executing deletion success action");
    
    if (ledExecutor) {
        ledExecutor->blink(1, 1000); // 长闪烁1次
    }
    
    if (buzzerExecutor) {
        buzzerExecutor->beepDelete();
    }
}

void DoorAccessExecutor::handleActions() {
    // 处理各个执行器的时序
    if (ledExecutor) {
        ledExecutor->handleBlinking();
    }
    
    if (buzzerExecutor) {
        buzzerExecutor->handleBeeping();
    }
    
    if (servoExecutor) {
        servoExecutor->handleServo();
    }
    
    // 处理主动作的时序
    if (actionInProgress) {
        unsigned long elapsed = millis() - actionStartTime;
        
        // LED持续时间控制
        if (elapsed >= LED_DURATION && ledExecutor) {
            ledExecutor->turnOff();
        }
        
        // 检查动作是否完成
        if (elapsed >= DOOR_OPEN_DURATION) {
            actionInProgress = false;
        }
    }
}

LEDExecutor* DoorAccessExecutor::getLEDExecutor() const {
    return ledExecutor;
}

BuzzerExecutor* DoorAccessExecutor::getBuzzerExecutor() const {
    return buzzerExecutor;
}

const char* DoorAccessExecutor::getName() const {
    return "Door Access Executor";
}
