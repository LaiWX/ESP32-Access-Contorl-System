#ifndef IACTIONEXECUTOR_H
#define IACTIONEXECUTOR_H

#include <Arduino.h>

/**
 * 动作执行器接口基类
 * 定义所有执行动作的通用接口
 */
class IActionExecutor {
public:
    virtual ~IActionExecutor() = default;
    
    /**
     * 初始化执行器
     * @return 初始化是否成功
     */
    virtual bool initialize() = 0;
    
    /**
     * 执行成功动作（如开门）
     */
    virtual void executeSuccessAction() = 0;
    
    /**
     * 执行失败动作（如警告）
     */
    virtual void executeFailureAction() = 0;
    
    /**
     * 执行注册成功动作
     */
    virtual void executeRegistrationSuccessAction() = 0;
    
    /**
     * 执行删除成功动作
     */
    virtual void executeDeletionSuccessAction() = 0;
    
    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    virtual const char* getName() const = 0;
};

#endif // IACTIONEXECUTOR_H
