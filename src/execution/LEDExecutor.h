#ifndef LEDEXECUTOR_H
#define LEDEXECUTOR_H

#include "../interfaces/IActionExecutor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * LED执行器
 * 使用LED闪烁来表示不同的动作状态
 * 重构后支持异步执行，只提供成功/失败两种模式
 */
class LEDExecutor : public IActionExecutor {
private:
    int ledPin;

    // 异步执行状态
    bool isExecuting_;
    TaskHandle_t taskHandle;

    // 执行模式
    enum ExecutionMode {
        MODE_NONE,
        MODE_SUCCESS,    // 快速闪烁2次
        MODE_FAILURE     // 慢速闪烁3次
    };

    ExecutionMode currentMode;

    // 静态任务函数
    static void ledTaskFunction(void* parameter);

    // 实际的LED控制逻辑
    void performSuccessPattern();
    void performFailurePattern();

public:
    /**
     * 构造函数
     * @param pin LED引脚号
     */
    LEDExecutor(int pin);

    /**
     * 析构函数
     */
    ~LEDExecutor();

    /**
     * 初始化LED执行器
     * @return 初始化是否成功
     */
    bool initialize() override;

    /**
     * 执行成功动作（异步）
     * 快速闪烁2次表示成功
     */
    void executeSuccessAction() override;

    /**
     * 执行失败动作（异步）
     * 慢速闪烁3次表示失败
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
     * 开启LED
     */
    void turnOn();

    /**
     * 关闭LED
     */
    void turnOff();
};

#endif // LEDEXECUTOR_H
