#ifndef SERVOEXECUTOR_H
#define SERVOEXECUTOR_H

#include <Arduino.h>

/**
 * 舵机执行器
 * 控制门锁舵机的开关动作
 */
class ServoExecutor {
private:
    int servoPin;
    unsigned long actionStartTime;
    bool doorIsOpen;
    bool actionInProgress;
    
    // 舵机角度
    static const int DOOR_CLOSED_ANGLE = 0;   // 关门角度
    static const int DOOR_OPEN_ANGLE = 90;    // 开门角度
    
    // 动作持续时间（毫秒）
    static const unsigned long DOOR_OPEN_DURATION = 3000;  // 3秒后自动关门

public:
    /**
     * 构造函数
     * @param pin 舵机控制引脚
     */
    ServoExecutor(int pin);
    
    /**
     * 初始化舵机
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 开门动作
     */
    void openDoor();
    
    /**
     * 关门动作
     */
    void closeDoor();
    
    /**
     * 处理舵机时序（在主循环中调用）
     * 自动关门逻辑
     */
    void handleServo();
    
    /**
     * 检查门是否开启
     * @return 门是否开启
     */
    bool isDoorOpen() const;
    
    /**
     * 检查是否有动作正在进行
     * @return 是否有动作正在进行
     */
    bool isActionInProgress() const;

private:
    /**
     * 设置舵机角度
     * @param angle 目标角度
     */
    void setServoAngle(int angle);
};

#endif // SERVOEXECUTOR_H
