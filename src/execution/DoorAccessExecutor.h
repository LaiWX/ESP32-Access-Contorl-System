#ifndef DOORACCESSEXECUTOR_H
#define DOORACCESSEXECUTOR_H

#include "../interfaces/IActionExecutor.h"

// 前向声明
class LEDExecutor;
class BuzzerExecutor;
class ServoExecutor;

/**
 * 门禁主执行器
 * 重构后作为简单的协调器，不管理时序
 * 只负责协调各个执行器的动作，具体时序由各执行器自主管理
 */
class DoorAccessExecutor : public IActionExecutor {
private:
    LEDExecutor* ledExecutor;
    BuzzerExecutor* buzzerExecutor;
    ServoExecutor* servoExecutor;

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
     * 执行成功动作
     * 协调LED、蜂鸣器、舵机的成功动作
     */
    void executeSuccessAction() override;

    /**
     * 执行失败动作
     * 协调LED、蜂鸣器的失败动作
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
     * 获取舵机执行器（供管理操作使用）
     * @return 舵机执行器指针
     */
    ServoExecutor* getServoExecutor() const;

    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    const char* getName() const override;
};

#endif // DOORACCESSEXECUTOR_H
