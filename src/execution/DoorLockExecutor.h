#ifndef DOORLOCKEXECUTOR_H
#define DOORLOCKEXECUTOR_H

#include "IActionExecutor.h"

/**
 * 门锁执行器
 * 控制实际的门锁硬件（未来使用）
 */
class DoorLockExecutor : public IActionExecutor {
private:
    int doorLockPin;
    int ledPin;
    
    /**
     * LED闪烁函数（用于状态指示）
     * @param times 闪烁次数
     * @param delayMs 每次闪烁的延迟时间
     */
    void blinkLED(int times, int delayMs);
    
public:
    /**
     * 构造函数
     * @param lockPin 门锁控制引脚
     * @param statusLedPin 状态指示LED引脚
     */
    DoorLockExecutor(int lockPin, int statusLedPin);
    
    /**
     * 初始化门锁执行器
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * 执行成功动作（开门）
     */
    void executeSuccessAction() override;
    
    /**
     * 执行失败动作（警告）
     */
    void executeFailureAction() override;
    
    /**
     * 执行注册成功动作
     */
    void executeRegistrationSuccessAction() override;
    
    /**
     * 执行删除成功动作
     */
    void executeDeletionSuccessAction() override;
    
    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    const char* getName() const override;
};

#endif // DOORLOCKEXECUTOR_H
