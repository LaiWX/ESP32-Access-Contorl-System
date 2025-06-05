#include "DoorAccessExecutor.h"
#include "LEDExecutor.h"
#include "BuzzerExecutor.h"
#include "ServoExecutor.h"

DoorAccessExecutor::DoorAccessExecutor(LEDExecutor* led, BuzzerExecutor* buzzer, ServoExecutor* servo)
    : ledExecutor(led), buzzerExecutor(buzzer), servoExecutor(servo),
      doorCloseTaskHandle(nullptr), doorCloseTaskActive(false) {
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

    // 停止之前的关门任务（如果存在）
    if (doorCloseTaskHandle != nullptr) {
        vTaskDelete(doorCloseTaskHandle);
        doorCloseTaskHandle = nullptr;
        doorCloseTaskActive = false;
    }

    // 协调LED和蜂鸣器执行成功动作
    if (ledExecutor) {
        ledExecutor->executeSuccessAction();
    }

    if (buzzerExecutor) {
        buzzerExecutor->executeSuccessAction();
    }

    // 舵机只执行开门动作（不自动关门）
    if (servoExecutor) {
        servoExecutor->executeOpenDoorAction();
    }

    // 启动定时关门任务
    doorCloseTaskActive = true;
    xTaskCreate(
        doorCloseTaskFunction,
        "DoorCloseTask",
        2048,
        this,
        1,
        &doorCloseTaskHandle
    );
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

    if (doorCloseTaskActive) {
        anyExecuting = true;
    }

    return anyExecuting;
}

void DoorAccessExecutor::stopExecution() {
    Serial.println("Door Access Executor: Stopping all executions");

    // 停止定时关门任务
    if (doorCloseTaskHandle != nullptr) {
        vTaskDelete(doorCloseTaskHandle);
        doorCloseTaskHandle = nullptr;
        doorCloseTaskActive = false;
    }

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

// 静态任务函数 - 定时关门
void DoorAccessExecutor::doorCloseTaskFunction(void* parameter) {
    DoorAccessExecutor* executor = static_cast<DoorAccessExecutor*>(parameter);

    Serial.println("Door Access Executor: Door close timer started");

    // 等待指定时间
    vTaskDelay(pdMS_TO_TICKS(DOOR_OPEN_DURATION));

    Serial.println("Door Access Executor: Auto-closing door with sound");

    // 执行关门动作
    if (executor->servoExecutor) {
        executor->servoExecutor->executeCloseDoorAction();
    }

    // 播放关门声音
    if (executor->buzzerExecutor) {
        executor->buzzerExecutor->executeDoorCloseAction();
    }

    // 清理任务状态
    executor->doorCloseTaskActive = false;
    executor->doorCloseTaskHandle = nullptr;

    Serial.println("Door Access Executor: Door close sequence completed");

    // 删除任务
    vTaskDelete(nullptr);
}
