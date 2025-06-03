#ifndef ACCESSCONTROLMANAGER_H
#define ACCESSCONTROLMANAGER_H

#include "../authentication/IAuthenticator.h"
#include "../management/IManagementOperation.h"
#include "../execution/DoorAccessExecutor.h"
#include <vector>
#include <map>

/**
 * 门禁控制管理器
 * 统一管理认证和管理操作，遵循开闭原则
 */
class AccessControlManager {
private:
    std::vector<IAuthenticator*> authenticators;
    std::map<String, IManagementOperation*> managementOperations;
    DoorAccessExecutor* doorExecutor;

    // 管理器级别的冷却机制（防止短时间内重复开门）
    unsigned long lastSuccessTime;
    static const unsigned long MANAGER_COOLDOWN_MS = 2000; // 2秒冷却期

public:
    /**
     * 构造函数
     * @param executor 门禁执行器
     */
    AccessControlManager(DoorAccessExecutor* executor);
    
    /**
     * 析构函数
     */
    ~AccessControlManager();
    
    /**
     * 添加认证器
     * @param authenticator 认证器指针
     */
    void addAuthenticator(IAuthenticator* authenticator);
    
    /**
     * 添加管理操作
     * @param type 操作类型（如"card", "fingerprint", "bluetooth"）
     * @param operation 管理操作指针
     */
    void addManagementOperation(const String& type, IManagementOperation* operation);
    
    /**
     * 初始化所有认证器
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 处理认证请求（在主循环中调用）
     */
    void handleAuthentication();
    
    /**
     * 处理管理操作（在主循环中调用）
     */
    void handleManagementOperations();
    
    /**
     * 执行管理命令
     * @param command 命令字符串，格式："type:action:param"
     *                例如："card:register", "card:delete:ABC123", "fingerprint:list"
     * @return 命令是否执行成功
     */
    bool executeManagementCommand(const String& command);
    
    /**
     * 重置所有组件
     */
    void resetAll();
    
    /**
     * 获取可用的管理操作类型
     */
    void listAvailableManagementTypes();

private:
    /**
     * 解析管理命令
     * @param command 命令字符串
     * @param type 输出：操作类型
     * @param action 输出：动作
     * @param param 输出：参数
     * @return 解析是否成功
     */
    bool parseManagementCommand(const String& command, String& type, String& action, String& param);
};

#endif // ACCESSCONTROLMANAGER_H
