#ifndef LEDEXECUTOR_H
#define LEDEXECUTOR_H

#include "IActionExecutor.h"

/**
 * LED执行器
 * 使用LED闪烁来表示不同的动作状态（调试用）
 */
class LEDExecutor : public IActionExecutor {
private:
    int ledPin;

    // 非阻塞闪烁状态
    bool isBlinking;
    int blinkCount;
    int targetBlinkCount;
    unsigned long lastBlinkTime;
    unsigned long blinkInterval;
    bool ledState;

    /**
     * LED闪烁函数（阻塞版本）
     * @param times 闪烁次数
     * @param delayMs 每次闪烁的延迟时间
     */
    void blinkLED(int times, int delayMs);
    
public:
    /**
     * 构造函数
     * @param pin LED引脚号
     */
    LEDExecutor(int pin);
    
    /**
     * 初始化LED执行器
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * 执行成功动作（快速闪烁2次）
     */
    void executeSuccessAction() override;
    
    /**
     * 执行失败动作（长闪1次）
     */
    void executeFailureAction() override;
    
    /**
     * 执行注册成功动作（闪烁3次）
     */
    void executeRegistrationSuccessAction() override;
    
    /**
     * 执行删除成功动作（闪烁2次）
     */
    void executeDeletionSuccessAction() override;
    
    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    const char* getName() const override;

    /**
     * 开启LED
     */
    void turnOn();

    /**
     * 关闭LED
     */
    void turnOff();

    /**
     * 非阻塞闪烁
     * @param times 闪烁次数
     * @param intervalMs 闪烁间隔（毫秒）
     */
    void blink(int times, unsigned long intervalMs);

    /**
     * 处理非阻塞闪烁（在主循环中调用）
     */
    void handleBlinking();

    /**
     * 停止闪烁
     */
    void stopBlinking();

    /**
     * 检查是否正在闪烁
     * @return 是否正在闪烁
     */
    bool isBlinkingActive() const;
};

#endif // LEDEXECUTOR_H
