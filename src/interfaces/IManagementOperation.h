#ifndef IMANAGEMENTOPERATION_H
#define IMANAGEMENTOPERATION_H

#include <Arduino.h>

/**
 * 管理操作接口
 * 定义所有管理操作的通用接口（卡片、指纹、蓝牙等）
 */
class IManagementOperation {
public:
    virtual ~IManagementOperation() = default;
    
    /**
     * 注册新项目
     * @return 注册是否成功启动
     */
    virtual bool registerNew() = 0;
    
    /**
     * 删除指定项目（仅从数据库删除）
     * @param id 项目ID
     * @return 删除是否成功
     */
    virtual bool deleteItem(const String& id) = 0;
    
    /**
     * 擦除并删除指定项目（物理擦除+数据库删除）
     * @param id 项目ID
     * @return 擦除删除是否成功启动
     */
    virtual bool eraseAndDeleteItem(const String& id) = 0;
    
    /**
     * 列出所有已注册的项目
     */
    virtual void listRegisteredItems() = 0;
    
    /**
     * 检查是否有管理操作正在进行
     * @return 是否有操作正在进行
     */
    virtual bool hasOngoingOperation() = 0;

    /**
     * 检查操作是否已完成（成功或失败）
     * @return 是否有操作刚刚完成
     */
    virtual bool hasCompletedOperation() = 0;
    
    /**
     * 处理管理操作（在主循环中调用）
     */
    virtual void handleOperations() = 0;
    
    /**
     * 重置管理器状态
     */
    virtual void reset() = 0;
    
    /**
     * 获取管理器名称
     * @return 管理器名称
     */
    virtual const char* getName() const = 0;
};

#endif // IMANAGEMENTOPERATION_H
