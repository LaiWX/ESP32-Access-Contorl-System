#ifndef AUTHENTICATIONMANAGER_H
#define AUTHENTICATIONMANAGER_H

#include "IAuthenticator.h"
#include "NFCAuthenticator.h"
#include "ManualTriggerAuthenticator.h"
#include "../execution/IActionExecutor.h"
#include "../data/CardDatabase.h"
#include "../data/FileSystemManager.h"
#include <vector>

/**
 * 认证管理器
 * 统一管理多种认证方式，处理认证请求和执行相应动作
 */
class AuthenticationManager {
private:
    std::vector<IAuthenticator*> authenticators;
    IActionExecutor* actionExecutor;
    CardDatabase* cardDatabase;
    FileSystemManager* fileSystemManager;
    NFCAuthenticator* nfcAuth; // 保留NFC认证器的直接引用用于注册功能
    
public:
    /**
     * 构造函数
     * @param executor 动作执行器
     * @param db 卡片数据库
     * @param fsManager 文件系统管理器
     */
    AuthenticationManager(IActionExecutor* executor, CardDatabase* db, FileSystemManager* fsManager);
    
    /**
     * 析构函数
     */
    ~AuthenticationManager();
    
    /**
     * 添加认证器
     * @param authenticator 认证器指针
     */
    void addAuthenticator(IAuthenticator* authenticator);
    
    /**
     * 设置NFC认证器（用于注册功能）
     * @param nfcAuthenticator NFC认证器指针
     */
    void setNFCAuthenticator(NFCAuthenticator* nfcAuthenticator);
    
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
     * 注册新卡片
     * @return 注册是否成功
     */
    bool registerNewCard();
    
    /**
     * 列出所有已注册的卡片
     */
    void listRegisteredCards();
    
    /**
     * 删除指定卡片
     * @param uid 卡片UID
     * @return 删除是否成功
     */
    bool deleteCard(const String& uid);

    /**
     * 擦除并删除指定卡片
     * @param uid 卡片UID
     * @return 擦除删除是否成功
     */
    bool eraseAndDeleteCard(const String& uid);
    
    /**
     * 重置所有认证器
     */
    void resetAll();
};

#endif // AUTHENTICATIONMANAGER_H
