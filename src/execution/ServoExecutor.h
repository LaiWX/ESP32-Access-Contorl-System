#ifndef SERVOEXECUTOR_H
#define SERVOEXECUTOR_H

#include "../interfaces/IActionExecutor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * 舵机执行器
 * 控制门锁舵机的开关动作
 * 重构后支持异步执行，只提供成功/失败两种模式
 */
class ServoExecutor : public IActionExecutor {
private:
    int servoPin;

    // 异步执行状态
    bool isExecuting_;
    TaskHandle_t taskHandle;
    bool doorIsOpen;

    // PWM配置常量（基于调试demo参数）
    static constexpr int PWM_CHANNEL = 0;
    static constexpr int PWM_FREQ = 50;        // 50Hz → 20ms周期
    static constexpr int PWM_BIT = 12;         // 分辨率12位
    static constexpr int PWM_MAX = (1 << PWM_BIT); // 4096

    // 每 step 代表的时间（微秒）
    static constexpr float STEP_TIME = 20000.0 / PWM_MAX; // ≈ 4.88us

    // 定义 0度 (0.53ms)，180度 (2.53ms) 所对应的 PWM 值
    static constexpr int PWM_0_DEGREE = static_cast<int>(530.0 / STEP_TIME);    // 0°开门
    static constexpr int PWM_180_DEGREE = static_cast<int>(2530.0 / STEP_TIME); // 180°关门

    // 舵机角度
    static const int DOOR_CLOSED_ANGLE = 180;   // 关门角度
    static const int DOOR_OPEN_ANGLE = 0;    // 开门角度

    // 动作持续时间（毫秒）
    static const unsigned long DOOR_OPEN_DURATION = 3000;  // 3秒后自动关门

    // 静态任务函数
    static void servoTaskFunction(void* parameter);

    // 实际的舵机控制逻辑
    void performOpenDoorSequence();

public:
    /**
     * 构造函数
     * @param pin 舵机控制引脚
     */
    ServoExecutor(int pin);

    /**
     * 析构函数
     */
    ~ServoExecutor();

    /**
     * 初始化舵机
     * @return 初始化是否成功
     */
    bool initialize() override;

    /**
     * 执行成功动作（异步）
     * 开门并在3秒后自动关门（保持原有行为）
     */
    void executeSuccessAction() override;

    /**
     * 执行开门动作（异步）
     * 只开门，不自动关门
     */
    void executeOpenDoorAction();

    /**
     * 执行关门动作（同步）
     * 立即关门
     */
    void executeCloseDoorAction();

    /**
     * 执行失败动作（异步）
     * 失败时不执行任何动作
     */
    void executeFailureAction() override;

    /**
     * 检查是否正在执行动作
     * @return 是否正在执行
     */
    bool isExecuting() const override;

    /**
     * 停止当前执行的动作
     */
    void stopExecution() override;

    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    const char* getName() const override;

    // 兼容性方法（保留用于向后兼容）
    /**
     * 开门动作
     */
    void openDoor();

    /**
     * 关门动作
     */
    void closeDoor();

    /**
     * 检查门是否开启
     * @return 门是否开启
     */
    bool isDoorOpen() const;

private:
    /**
     * 设置舵机角度
     * @param angle 目标角度
     */
    void setServoAngle(int angle);
};

#endif // SERVOEXECUTOR_H
