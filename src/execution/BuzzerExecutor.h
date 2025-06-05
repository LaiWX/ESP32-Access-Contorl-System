#ifndef BUZZEREXECUTOR_H
#define BUZZEREXECUTOR_H

#include "../interfaces/IActionExecutor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * 蜂鸣器执行器
 * 控制蜂鸣器的响应模式
 * 重构后支持异步执行，只提供成功/失败两种模式
 */
class BuzzerExecutor : public IActionExecutor {
private:
    int buzzerPin;

    // 异步执行状态
    bool isExecuting_;
    TaskHandle_t taskHandle;

    // 执行模式
    enum ExecutionMode {
        MODE_NONE,
        MODE_SUCCESS,    // 单次长响
        MODE_FAILURE     // 三次短响
    };

    ExecutionMode currentMode;

    // 静态任务函数
    static void buzzerTaskFunction(void* parameter);

    // 实际的蜂鸣器控制逻辑
    void performSuccessPattern();
    void performFailurePattern();

public:
    /**
     * 构造函数
     * @param pin 蜂鸣器引脚
     */
    BuzzerExecutor(int pin);

    /**
     * 析构函数
     */
    ~BuzzerExecutor();

    /**
     * 初始化蜂鸣器
     * @return 初始化是否成功
     */
    bool initialize() override;

    /**
     * 执行成功动作（异步）
     * 单次长响表示成功
     */
    void executeSuccessAction() override;

    /**
     * 执行失败动作（异步）
     * 三次短响表示失败
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
     * 成功响应（单次长响）
     */
    void beepDoorOpen();

    /**
     * 失败响应（三次短响）
     */
    void beepFailure();

    /**
     * 注册成功响应（两次中响）
     */
    void beepRegister();

    /**
     * 删除成功响应（一次中响）
     */
    void beepDelete();

    /**
     * 检查是否正在蜂鸣
     * @return 是否正在蜂鸣
     */
    bool isActive() const;
};

#endif // BUZZEREXECUTOR_H
