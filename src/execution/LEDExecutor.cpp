#include "LEDExecutor.h"

LEDExecutor::LEDExecutor(int pin)
    : ledPin(pin), isExecuting_(false), taskHandle(nullptr), currentMode(MODE_NONE) {
}

LEDExecutor::~LEDExecutor() {
    stopExecution();
}

bool LEDExecutor::initialize() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    Serial.print("LED Executor initialized on pin ");
    Serial.println(ledPin);
    return true;
}

void LEDExecutor::executeSuccessAction() {
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("LED Executor: Starting success action (async)");
    currentMode = MODE_SUCCESS;
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        ledTaskFunction,
        "LEDTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

void LEDExecutor::executeFailureAction() {
    if (isExecuting_) {
        stopExecution();
    }

    Serial.println("LED Executor: Starting failure action (async)");
    currentMode = MODE_FAILURE;
    isExecuting_ = true;

    // 创建FreeRTOS任务
    xTaskCreate(
        ledTaskFunction,
        "LEDTask",
        2048,
        this,
        1,
        &taskHandle
    );
}

bool LEDExecutor::isExecuting() const {
    return isExecuting_;
}

void LEDExecutor::stopExecution() {
    if (taskHandle != nullptr) {
        vTaskDelete(taskHandle);
        taskHandle = nullptr;
    }
    isExecuting_ = false;
    currentMode = MODE_NONE;
    digitalWrite(ledPin, LOW);
    Serial.println("LED Executor: Execution stopped");
}

const char* LEDExecutor::getName() const {
    return "LED Executor";
}

// 静态任务函数
void LEDExecutor::ledTaskFunction(void* parameter) {
    LEDExecutor* executor = static_cast<LEDExecutor*>(parameter);

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
    digitalWrite(executor->ledPin, LOW);

    Serial.println("LED Executor: Action completed");

    // 删除任务
    vTaskDelete(nullptr);
}

void LEDExecutor::performSuccessPattern() {
    // 快速闪烁2次表示成功
    for (int i = 0; i < 2; i++) {
        digitalWrite(ledPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(200));
        digitalWrite(ledPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

void LEDExecutor::performFailurePattern() {
    // 慢速闪烁3次表示失败
    for (int i = 0; i < 3; i++) {
        digitalWrite(ledPin, HIGH);
        vTaskDelay(pdMS_TO_TICKS(500));
        digitalWrite(ledPin, LOW);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// 兼容性方法
void LEDExecutor::turnOn() {
    stopExecution();
    digitalWrite(ledPin, HIGH);
}

void LEDExecutor::turnOff() {
    stopExecution();
    digitalWrite(ledPin, LOW);
}
