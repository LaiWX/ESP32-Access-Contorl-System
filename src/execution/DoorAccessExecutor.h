#ifndef DOORACCESSEXECUTOR_H
#define DOORACCESSEXECUTOR_H

#include "../interfaces/IActionExecutor.h"

// 前向声明
class LEDExecutor;
class BuzzerExecutor;
class ServoExecutor;

/**
 * 门禁主执行器
 * 统一管理门禁系统的所有执行动作
 * 对外只暴露简单的开门/拒绝接口
 */
class DoorAccessExecutor : public IActionExecutor {
private:
    LEDExecutor* ledExecutor;
    BuzzerExecutor* buzzerExecutor;
    ServoExecutor* servoExecutor;
    
    // 时序控制
    unsigned long actionStartTime;
    bool actionInProgress;
    
    // 动作持续时间（毫秒）
    static const unsigned long DOOR_OPEN_DURATION = 3000;  // 3秒
    static const unsigned long LED_DURATION = 2000;        // 2秒
    static const unsigned long BUZZER_DURATION = 500;      // 0.5秒

public:
    /**
     * 构造函数
     * @param led LED执行器
     * @param buzzer 蜂鸣器执行器
     * @param servo 舵机执行器
     */
    DoorAccessExecutor(LEDExecutor* led, BuzzerExecutor* buzzer, ServoExecutor* servo);
    
    /**
     * 初始化执行器
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * 执行开门动作
     * 协调LED、蜂鸣器、舵机的动作
     */
    void executeSuccessAction() override;
    
    /**
     * 执行拒绝动作
     * 协调LED、蜂鸣器的动作
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
     * 处理动作时序（在主循环中调用）
     */
    void handleActions();
    
    /**
     * 获取LED执行器（供管理操作使用）
     * @return LED执行器指针
     */
    LEDExecutor* getLEDExecutor() const;
    
    /**
     * 获取蜂鸣器执行器（供管理操作使用）
     * @return 蜂鸣器执行器指针
     */
    BuzzerExecutor* getBuzzerExecutor() const;

    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    const char* getName() const override;
};

#endif // DOORACCESSEXECUTOR_H
