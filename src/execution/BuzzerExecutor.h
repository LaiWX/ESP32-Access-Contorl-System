#ifndef BUZZEREXECUTOR_H
#define BUZZEREXECUTOR_H

#include <Arduino.h>

/**
 * 蜂鸣器执行器
 * 控制蜂鸣器的各种响应模式
 */
class BuzzerExecutor {
private:
    int buzzerPin;
    unsigned long lastBeepTime;
    int beepCount;
    int targetBeepCount;
    bool isBeeping;
    
    // 蜂鸣器模式
    enum BeepMode {
        BEEP_NONE,
        BEEP_SUCCESS,    // 单次长响
        BEEP_FAILURE,    // 三次短响
        BEEP_REGISTER,   // 两次中响
        BEEP_DELETE      // 一次中响
    };
    
    BeepMode currentMode;
    
    // 时间常量（毫秒）
    static const unsigned long SHORT_BEEP = 100;
    static const unsigned long MEDIUM_BEEP = 300;
    static const unsigned long LONG_BEEP = 500;
    static const unsigned long BEEP_INTERVAL = 200;

public:
    /**
     * 构造函数
     * @param pin 蜂鸣器引脚
     */
    BuzzerExecutor(int pin);
    
    /**
     * 初始化蜂鸣器
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 成功响应（单次长响）
     */
    void beepSuccess();
    
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
     * 处理蜂鸣器时序（在主循环中调用）
     */
    void handleBeeping();
    
    /**
     * 停止蜂鸣
     */
    void stopBeeping();
    
    /**
     * 检查是否正在蜂鸣
     * @return 是否正在蜂鸣
     */
    bool isActive() const;
};

#endif // BUZZEREXECUTOR_H
