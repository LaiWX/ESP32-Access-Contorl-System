#include "BuzzerExecutor.h"

BuzzerExecutor::BuzzerExecutor(int pin)
    : buzzerPin(pin), isExecuting_(false), taskHandle(nullptr), currentMode(MODE_NONE) {
}

BuzzerExecutor::~BuzzerExecutor() {
    stopExecution();
}

bool BuzzerExecutor::initialize() {
    pinMode(buzzerPin, OUTPUT);
    digitalWrite(buzzerPin, LOW);

    Serial.print("Buzzer Executor initialized on pin ");
    Serial.println(buzzerPin);

    return true;
}

void BuzzerExecutor::executeSuccessAction() {
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Buzzer Executor: Starting success action (async)");
    currentMode = MODE_SUCCESS;
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        buzzerTaskFunction,
        "BuzzerTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

void BuzzerExecutor::executeDoorCloseAction() {
    // 松开舵机时的反馈
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Buzzer Executor: Starting success action (async)");
    currentMode = MODE_DOOR_CLOSE;
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        buzzerTaskFunction,
        "BuzzerTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

void BuzzerExecutor::executeFailureAction() {
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Buzzer Executor: Starting failure action (async)");
    currentMode = MODE_FAILURE;
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        buzzerTaskFunction,
        "BuzzerTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

bool BuzzerExecutor::isExecuting() const {
    return isExecuting_;
}

void BuzzerExecutor::stopExecution() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    isExecuting_ = false;
    currentMode = MODE_NONE;
    digitalWrite(buzzerPin, LOW);
    Serial.println("Buzzer Executor: Execution stopped");
}

const char* BuzzerExecutor::getName() const {
    return "Buzzer Executor";
}

// 静态任务函数
void BuzzerExecutor::buzzerTaskFunction(void* parameter) {
    BuzzerExecutor* executor = static_cast<BuzzerExecutor*>(parameter);

    switch (executor->currentMode) {
        case MODE_SUCCESS:
            executor->performSuccessPattern();
            break;
        case MODE_FAILURE:
            executor->performFailurePattern();
            break;
        case MODE_DOOR_CLOSE:
            executor->performDoorClosePattern();
            break;
        default:
            break;
    }

    // 任务完成，清理状态
    executor->isExecuting_ = false;
    executor->currentMode = MODE_NONE;
    executor->taskHandle = nullptr;
    digitalWrite(executor->buzzerPin, LOW);

    Serial.println("Buzzer Executor: Action completed");

    // 删除任务
    vTaskDelete(nullptr);
}

void BuzzerExecutor::performSuccessPattern() {
    // 升调表达成功
    tone(buzzerPin, 784, 100);  // 784HZ
    vTaskDelay(100 / portTICK_PERIOD_MS);
    tone(buzzerPin, 880, 100);  // 880HZ
    vTaskDelay(100 / portTICK_PERIOD_MS);
    tone(buzzerPin, 980, 100);  // 980HZ
}

void BuzzerExecutor::performDoorClosePattern() {
    // 降调表达关门
    tone(buzzerPin, 980, 100);  // 980HZ
    vTaskDelay(100 / portTICK_PERIOD_MS);
    tone(buzzerPin, 880, 100);  // 880HZ
    vTaskDelay(100 / portTICK_PERIOD_MS);
    tone(buzzerPin, 784, 100);  // 784HZ
}

void BuzzerExecutor::performFailurePattern() {
    // 低音重复表达失败
    tone(buzzerPin, 262, 150);
    vTaskDelay(150 / portTICK_PERIOD_MS);
    tone(buzzerPin, 262, 150);
}

bool BuzzerExecutor::isActive() const {
    return isExecuting_;
}
