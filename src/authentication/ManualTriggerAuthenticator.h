#ifndef MANUALTRIGGERAUTHENTICATOR_H
#define MANUALTRIGGERAUTHENTICATOR_H

#include "IAuthenticator.h"

/**
 * 手动触发认证器
 * 通过检测引脚的下降沿来触发开门动作
 */
class ManualTriggerAuthenticator : public IAuthenticator {
private:
    int triggerPin;
    int lastPinState;
    unsigned long lastTriggerTime;
    
    // 防抖动时间（毫秒）
    static const unsigned long DEBOUNCE_DELAY = 50;
    
    // 冷却时间（毫秒）
    static const unsigned long COOLDOWN_TIME = 1000;
    
public:
    /**
     * 构造函数
     * @param pin 触发引脚号
     */
    ManualTriggerAuthenticator(int pin);
    
    /**
     * 初始化手动触发认证器
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * 检查是否有认证请求（检测下降沿）
     * @return 是否检测到触发信号
     */
    bool hasAuthenticationRequest() override;
    
    /**
     * 执行认证（手动触发总是成功）
     * @return 认证结果（总是true）
     */
    bool authenticate() override;
    
    /**
     * 获取认证器名称
     * @return 认证器名称
     */
    const char* getName() const override;
    
    /**
     * 重置认证器状态
     */
    void reset() override;
};

#endif // MANUALTRIGGERAUTHENTICATOR_H
