#ifndef IACTIONEXECUTOR_H
#define IACTIONEXECUTOR_H

#include <Arduino.h>

/**
 * 动作执行器接口基类
 * 定义所有执行动作的通用接口
 * 重构后只提供成功/失败两个基本动作，支持异步执行
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
     * 执行成功动作
     * 异步执行，立即返回
     */
    virtual void executeSuccessAction() = 0;

    /**
     * 执行失败动作
     * 异步执行，立即返回
     */
    virtual void executeFailureAction() = 0;

    /**
     * 检查执行器是否正在执行动作
     * @return 是否正在执行动作
     */
    virtual bool isExecuting() const = 0;

    /**
     * 停止当前执行的动作
     */
    virtual void stopExecution() = 0;

    /**
     * 获取执行器名称
     * @return 执行器名称
     */
    virtual const char* getName() const = 0;
};

#endif // IACTIONEXECUTOR_H
