#ifndef SYSTEMCOORDINATOR_H
#define SYSTEMCOORDINATOR_H

#include <Arduino.h>
#include <vector>
#include <map>
#include "../interfaces/IAuthenticator.h"
#include "../interfaces/IManagementOperation.h"
#include "../execution/DoorAccessExecutor.h"

/**
 * 系统协调器
 * 实现状态机，确保认证状态和管理状态互斥
 * 作为主循环的核心协调器
 */
class SystemCoordinator {
public:
    // 系统状态枚举
    enum SystemState {
        STATE_IDLE,           // 空闲状态
        STATE_AUTHENTICATION, // 认证状态
        STATE_MANAGEMENT      // 管理状态
    };

private:
    // 系统状态
    SystemState currentState;
    unsigned long stateStartTime;
    
    // 组件引用
    std::vector<IAuthenticator*> authenticators;
    std::map<String, IManagementOperation*> managementOperations;
    DoorAccessExecutor* doorExecutor;
    
    // 管理状态超时设置
    static const unsigned long MANAGEMENT_TIMEOUT_MS = 10000; // 10秒
    
    // 认证冷却机制
    unsigned long lastSuccessTime;
    static const unsigned long AUTH_COOLDOWN_MS = 2000; // 2秒

public:
    /**
     * 构造函数
     * @param executor 门禁执行器
     */
    SystemCoordinator(DoorAccessExecutor* executor);
    
    /**
     * 析构函数
     */
    ~SystemCoordinator();
    
    /**
     * 添加认证器
     * @param authenticator 认证器指针
     */
    void addAuthenticator(IAuthenticator* authenticator);
    
    /**
     * 添加管理操作
     * @param type 操作类型
     * @param operation 管理操作指针
     */
    void addManagementOperation(const String& type, IManagementOperation* operation);

    /**
     * 处理串口命令
     * @param command 命令字符串
     * @return 是否成功处理
     */
    bool handleCommand(const String& command);

    /**
     * 初始化系统协调器
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 主循环处理函数
     * 实现状态机逻辑
     */
    void handleLoop();

    /**
     * 强制退出管理状态
     */
    void exitManagementState();
    
    /**
     * 获取当前系统状态
     * @return 当前状态
     */
    SystemState getCurrentState() const;
    
    /**
     * 重置所有组件
     */
    void resetAll();
    
    /**
     * 执行管理命令
     * @param command 命令字符串
     * @return 执行是否成功
     */
    bool executeManagementCommand(const String& command);
    
    /**
     * 列出可用的管理操作类型
     */
    void listAvailableManagementTypes();

private:
    /**
     * 处理认证状态
     */
    void handleAuthenticationState();
    
    /**
     * 处理管理状态
     */
    void handleManagementState();
    
    /**
     * 处理空闲状态
     */
    void handleIdleState();
    
    /**
     * 检查管理状态超时
     */
    void checkManagementTimeout();
    
    /**
     * 解析管理命令
     * @param command 命令字符串
     * @param type 输出：操作类型
     * @param action 输出：动作
     * @param param 输出：参数
     * @return 解析是否成功
     */
    bool parseManagementCommand(const String& command, String& type, String& action, String& param);
    
    /**
     * 转换到指定状态
     * @param newState 新状态
     */
    void transitionToState(SystemState newState);
};

#endif // SYSTEMCOORDINATOR_H
