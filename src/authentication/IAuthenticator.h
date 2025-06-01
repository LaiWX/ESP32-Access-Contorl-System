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
};

#endif // IAUTHENTICATOR_H
