#include "DoorAccessExecutor.h"
#include "LEDExecutor.h"
#include "BuzzerExecutor.h"
#include "ServoExecutor.h"

DoorAccessExecutor::DoorAccessExecutor(LEDExecutor* led, BuzzerExecutor* buzzer, ServoExecutor* servo)
    : ledExecutor(led), buzzerExecutor(buzzer), servoExecutor(servo) {
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
    Serial.println("Door Access Executor: Executing success action (OPEN DOOR)");

    // 协调所有执行器执行成功动作
    if (ledExecutor) {
        ledExecutor->executeSuccessAction();
    }

    if (buzzerExecutor) {
        buzzerExecutor->executeSuccessAction();
    }

    if (servoExecutor) {
        servoExecutor->executeSuccessAction();
    }
}

void DoorAccessExecutor::executeFailureAction() {
    Serial.println("Door Access Executor: Executing failure action (ACCESS DENIED)");

    // 协调LED和蜂鸣器执行失败动作（不开门）
    if (ledExecutor) {
        ledExecutor->executeFailureAction();
    }

    if (buzzerExecutor) {
        buzzerExecutor->executeFailureAction();
    }

    // 舵机不执行失败动作（不开门）
    if (servoExecutor) {
        servoExecutor->executeFailureAction();
    }
}

bool DoorAccessExecutor::isExecuting() const {
    bool anyExecuting = false;

    if (ledExecutor && ledExecutor->isExecuting()) {
        anyExecuting = true;
    }

    if (buzzerExecutor && buzzerExecutor->isExecuting()) {
        anyExecuting = true;
    }

    if (servoExecutor && servoExecutor->isExecuting()) {
        anyExecuting = true;
    }

    return anyExecuting;
}

void DoorAccessExecutor::stopExecution() {
    Serial.println("Door Access Executor: Stopping all executions");

    if (ledExecutor) {
        ledExecutor->stopExecution();
    }

    if (buzzerExecutor) {
        buzzerExecutor->stopExecution();
    }

    if (servoExecutor) {
        servoExecutor->stopExecution();
    }
}

LEDExecutor* DoorAccessExecutor::getLEDExecutor() const {
    return ledExecutor;
}

BuzzerExecutor* DoorAccessExecutor::getBuzzerExecutor() const {
    return buzzerExecutor;
}

ServoExecutor* DoorAccessExecutor::getServoExecutor() const {
    return servoExecutor;
}

const char* DoorAccessExecutor::getName() const {
    return "Door Access Executor";
}
