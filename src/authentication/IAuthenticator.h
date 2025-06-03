#ifndef IAUTHENTICATOR_H
#define IAUTHENTICATOR_H

#include <Arduino.h>

/**
 * 认证器接口基类
 * 定义所有认证方式的通用接口
 */
class IAuthenticator {
public:
    virtual ~IAuthenticator() = default;
    
    /**
     * 初始化认证器
     * @return 初始化是否成功
     */
    virtual bool initialize() = 0;
    
    /**
     * 检查是否有认证请求
     * @return 是否检测到认证请求
     */
    virtual bool hasAuthenticationRequest() = 0;
    
    /**
     * 执行认证
     * @return 认证是否成功
     */
    virtual bool authenticate() = 0;
    
    /**
     * 获取认证器名称
     * @return 认证器名称
     */
    virtual const char* getName() const = 0;
    
    /**
     * 重置认证器状态
     */
    virtual void reset() = 0;

    /**
     * 检查认证器是否支持异步操作
     * @return 是否支持异步操作
     */
    virtual bool supportsAsyncOperations() const { return false; }

    /**
     * 检查是否有已完成的异步操作（仅对支持异步操作的认证器有效）
     * @return 是否有已完成的操作
     */
    virtual bool hasCompletedOperation() const { return false; }

    /**
     * 获取最近完成操作的结果（仅对支持异步操作的认证器有效）
     * @return 操作结果
     */
    virtual bool getOperationResult() const { return false; }

    /**
     * 清除操作完成标志（仅对支持异步操作的认证器有效）
     */
    virtual void clearOperationFlag() {}
};

#endif // IAUTHENTICATOR_H
