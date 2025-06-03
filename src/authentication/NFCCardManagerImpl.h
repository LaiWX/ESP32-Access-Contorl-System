#ifndef NFCCARDMANAGERIMPL_H
#define NFCCARDMANAGERIMPL_H

#include "../management/IManagementOperation.h"
#include "../data/CardDatabase.h"
#include "../data/FileSystemManager.h"
#include "../execution/DoorAccessExecutor.h"
#include "../utils/Utils.h"
#include "../nfc/NFCCoordinator.h"

/**
 * NFC卡片管理器实现
 * 专门负责NFC卡片的注册、删除、擦除等管理操作
 * 与认证功能完全分离
 */
class NFCCardManagerImpl : public IManagementOperation {
public:
    // 操作类型枚举
    enum OperationType {
        OP_NONE,
        OP_REGISTER,
        OP_ERASE
    };

private:
    NFCCoordinator* nfcCoordinator;
    CardDatabase* cardDatabase;
    FileSystemManager* fileSystemManager;
    DoorAccessExecutor* doorExecutor;
    
    // MIFARE Classic 配置
    static const uint8_t SECTOR_TRAILER_BLOCK = 7;
    static const uint8_t AUTH_BLOCK = 4;
    static const uint8_t TRAILER_SIZE = 16;

    // 状态管理
    enum NFCState {
        NFC_IDLE,
        NFC_DETECTING,
        NFC_CARD_PRESENT,
        NFC_REGISTERING,
        NFC_ERASING
    };

    NFCState currentState;
    OperationType currentOperation;
    
    // 操作状态
    bool operationCompleted;
    bool operationSuccess;
    unsigned long operationStartTime;
    String targetUID;
    
    // 超时设置
    static const unsigned long OPERATION_TIMEOUT = 10000; // 10秒
    
    // 冷却机制
    unsigned long lastOperationTime;
    String lastCardUID;
    static const unsigned long COOLDOWN_TIME = 1000; // 1秒冷却时间
    static const unsigned long SAME_CARD_DELAY = 100; // 同卡片延迟

    // 内部方法
    bool startOperationListening();
    void handleOperationTimeout();
    void handleCardDetection();
    void processRegistration();
    void processErasure();
    bool authenticateCard(uint8_t* uid, uint8_t uidLength, uint8_t* key);
    bool writeKeyToCard(uint8_t* uid, uint8_t uidLength, uint8_t* newKey);
    bool eraseKeyFromCard(uint8_t* uid, uint8_t uidLength);
    void generateRandomKey(uint8_t* key);
    void resetOperationState();

public:
    /**
     * 构造函数
     * @param coordinator NFC协调器指针
     * @param db 卡片数据库指针
     * @param fsManager 文件系统管理器指针
     * @param executor 门禁执行器指针
     */
    NFCCardManagerImpl(NFCCoordinator* coordinator, CardDatabase* db,
                       FileSystemManager* fsManager, DoorAccessExecutor* executor);

    // IManagementOperation接口实现
    bool registerNew() override;
    bool deleteItem(const String& id) override;
    bool eraseAndDeleteItem(const String& id) override;
    void listRegisteredItems() override;
    bool hasOngoingOperation() override;
    void handleOperations() override;
    void reset() override;
    const char* getName() const override;

    // 兼容性方法（保持向后兼容）
    bool registerNewCard() { return registerNew(); }
    bool deleteCard(const String& uid) { return deleteItem(uid); }
    bool eraseAndDeleteCard(const String& uid) { return eraseAndDeleteItem(uid); }
    void listRegisteredCards() { listRegisteredItems(); }
};

#endif // NFCCARDMANAGERIMPL_H
