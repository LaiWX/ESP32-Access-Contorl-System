#include "ServoExecutor.h"
#include <Arduino.h>
#include "driver/ledc.h"

ServoExecutor::ServoExecutor(int pin)
    : servoPin(pin), isExecuting_(false), taskHandle(nullptr), doorIsOpen(false) {
}

ServoExecutor::~ServoExecutor() {
    stopExecution();
}

bool ServoExecutor::initialize() {
    // 配置LEDC定时器
    ledc_timer_config_t timer_config = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = static_cast<ledc_timer_bit_t>(PWM_BIT),
        .timer_num = LEDC_TIMER_0,
        .freq_hz = PWM_FREQ,
        .clk_cfg = LEDC_AUTO_CLK
    };
    esp_err_t timer_result = ledc_timer_config(&timer_config);

    // 配置LEDC通道
    ledc_channel_config_t channel_config = {
        .gpio_num = servoPin,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = static_cast<ledc_channel_t>(PWM_CHANNEL),
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = LEDC_TIMER_0,
        .duty = 0,
        .hpoint = 0
    };
    esp_err_t channel_result = ledc_channel_config(&channel_config);

    if (timer_result != ESP_OK || channel_result != ESP_OK) {
        Serial.println("Servo Executor: PWM configuration failed");
        return false;
    }

    // 初始化为关门状态
    setServoAngle(DOOR_CLOSED_ANGLE);
    doorIsOpen = false;

    Serial.print("Servo Executor initialized on pin ");
    Serial.print(servoPin);
    Serial.print(" with PWM channel ");
    Serial.println(PWM_CHANNEL);

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
    // 使用精确的PWM值映射角度
    uint32_t duty;

    if (angle == 0) {
        // 0°关门位置：0.53ms脉宽
        duty = PWM_0_DEGREE;
    } else if (angle == 180) {
        // 180°开门位置：2.53ms脉宽
        duty = PWM_180_DEGREE;
    } else {
        // 线性插值其他角度
        duty = map(angle, 0, 180, PWM_0_DEGREE, PWM_180_DEGREE);
    }

    // 设置PWM占空比
    esp_err_t result = ledc_set_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(PWM_CHANNEL), duty);
    if (result == ESP_OK) {
        ledc_update_duty(LEDC_LOW_SPEED_MODE, static_cast<ledc_channel_t>(PWM_CHANNEL));
    }

    Serial.print("Servo angle set to: ");
    Serial.print(angle);
    Serial.print("° (PWM duty: ");
    Serial.print(duty);
    Serial.print("/");
    Serial.print(PWM_MAX);
    Serial.print(", pulse width: ");
    Serial.print(duty * STEP_TIME / 1000.0, 2);
    Serial.println("ms)");
}
