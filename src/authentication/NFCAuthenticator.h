#ifndef NFCAUTHENTICATOR_H
#define NFCAUTHENTICATOR_H

#include "../interfaces/IAuthenticator.h"
#include "../data/CardDatabase.h"
#include "../utils/Utils.h"
#include "../nfc/NFCManager.h"

/**
 * NFC认证器
 * 使用PN532模块进行MIFARE Classic卡片认证
 */
class NFCAuthenticator : public IAuthenticator {
private:
    NFCManager* nfcManager;
    CardDatabase* cardDatabase;
    
    // MIFARE Classic 配置
    static const uint8_t SECTOR_TRAILER_BLOCK = 7;
    static const uint8_t AUTH_BLOCK = 4;
    static const uint8_t TRAILER_SIZE = 16;

    // 冷却机制
    unsigned long lastCardTime;
    String lastCardUID;

    // 防重放时间（毫秒）
    static const unsigned long CARD_COOLDOWN_MS = 1000;
    
    /**
     * 读取卡片UID
     */
    bool readCardUID(uint8_t* uid, uint8_t* uidLen);

    /**
     * 认证指定块
     */
    bool authenticateBlock(uint8_t* uid, uint8_t uidLen, uint8_t blockNumber, uint8_t* key);

    /**
     * 启动NFC监听
     */
    void startNFCListening();

    /**
     * 处理卡片认证
     */
    bool handleCardAuthentication(uint8_t* uid, uint8_t uidLength);

public:
    /**
     * 构造函数
     * @param manager NFC管理器指针
     * @param db 卡片数据库指针
     */
    NFCAuthenticator(NFCManager* manager, CardDatabase* db);
    
    /**
     * 初始化NFC认证器
     * @return 初始化是否成功
     */
    bool initialize() override;
    
    /**
     * 检查是否有认证请求
     * @return 是否检测到认证请求
     */
    bool hasAuthenticationRequest() override;
    
    /**
     * 执行认证
     * @return 认证是否成功
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

#endif // NFCAUTHENTICATOR_H
