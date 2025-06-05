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
    // 单次长响表示成功
    digitalWrite(buzzerPin, HIGH);
    vTaskDelay(pdMS_TO_TICKS(500));
    digitalWrite(buzzerPin, LOW);
}

void BuzzerExecutor::performFailurePattern() {
    // 三次短响表示失败
    for (int i = 0; i < 3; i++) {
        digitalWrite(buzzerPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        digitalWrite(buzzerPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// 兼容性方法
void BuzzerExecutor::beepDoorOpen() {
    executeSuccessAction();
}

void BuzzerExecutor::beepFailure() {
    executeFailureAction();
}

void BuzzerExecutor::beepRegister() {
    // 两次中响表示注册成功
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Buzzer: Register beep (compatibility mode)");
    for (int i = 0; i < 2; i++) {
        digitalWrite(buzzerPin, HIGH);
        delay(300);
        digitalWrite(buzzerPin, LOW);
        delay(200);
    }
}

void BuzzerExecutor::beepDelete() {
    // 一次中响表示删除成功
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("Buzzer: Delete beep (compatibility mode)");
    digitalWrite(buzzerPin, HIGH);
    delay(300);
    digitalWrite(buzzerPin, LOW);
}

bool BuzzerExecutor::isActive() const {
    return isExecuting_;
}
