#include "ServoExecutor.h"

ServoExecutor::ServoExecutor(int pin)
    : servoPin(pin), isExecuting_(false), taskHandle(nullptr), doorIsOpen(false) {
}

ServoExecutor::~ServoExecutor() {
    stopExecution();
}

bool ServoExecutor::initialize() {
    pinMode(servoPin, OUTPUT);

    // 初始化为关门状态
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;

    Serial.print("Servo Executor initialized on pin ");
    Serial.println(servoPin);

    return true;
}

void ServoExecutor::executeSuccessAction() {
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Servo Executor: Starting success action (async) - Opening door");
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        servoTaskFunction,
        "ServoTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

void ServoExecutor::executeFailureAction() {
    // 失败时不执行任何动作
    Serial.println("Servo Executor: Failure action - No door operation");
}

bool ServoExecutor::isExecuting() const {
    return isExecuting_;
}

void ServoExecutor::stopExecution() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    isExecuting_ = false;
    // 确保门关闭
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;
    Serial.println("Servo Executor: Execution stopped");
}

const char* ServoExecutor::getName() const {
    return "Servo Executor";
}

// 静态任务函数
void ServoExecutor::servoTaskFunction(void* parameter) {
    ServoExecutor* executor = static_cast<ServoExecutor*>(parameter);
    executor->performOpenDoorSequence();

    // 任务完成，清理状态
    executor->isExecuting_ = false;
    executor->taskHandle = nullptr;

    Serial.println("Servo Executor: Action completed");

    // 删除任务
    vTaskDelete(nullptr);
}

void ServoExecutor::performOpenDoorSequence() {
    // 开门
    Serial.println("Servo: Opening door");
    setServoAngle(DOOR_OPEN_ANGLE);
    doorIsOpen = true;

    // 等待指定时间
    vTaskDelay(pdMS_TO_TICKS(DOOR_OPEN_DURATION));

    // 自动关门
    Serial.println("Servo: Auto-closing door");
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;
}

// 兼容性方法
void ServoExecutor::openDoor() {
    if (!isExecuting_) {
        Serial.println("Servo: Opening door (compatibility mode)");
        setServoAngle(DOOR_OPEN_ANGLE);
        doorIsOpen = true;
    }
}

void ServoExecutor::closeDoor() {
    Serial.println("Servo: Closing door (compatibility mode)");
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;
}

bool ServoExecutor::isDoorOpen() const {
    return doorIsOpen;
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
