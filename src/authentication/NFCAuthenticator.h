#ifndef NFCAUTHENTICATOR_H
#define NFCAUTHENTICATOR_H

#include "IAuthenticator.h"
#include "../data/CardDatabase.h"
#include "../utils/Utils.h"
#include <Adafruit_PN532.h>

/**
 * NFC认证器
 * 使用PN532模块进行MIFARE Classic卡片认证
 */
class NFCAuthenticator : public IAuthenticator {
private:
    Adafruit_PN532* nfc;
    CardDatabase* cardDatabase;
    
    // 硬件配置
    int irqPin;
    int resetPin;
    
    // MIFARE Classic 配置
    static const uint8_t SECTOR_TRAILER_BLOCK = 7;
    static const uint8_t AUTH_BLOCK = 4;
    static const uint8_t TRAILER_SIZE = 16;
    
    // 状态管理
    enum NFCState {
        NFC_IDLE,
        NFC_DETECTING,
        NFC_CARD_PRESENT
    };
    
    NFCState currentState;
    unsigned long lastCardTime;
    String lastCardUID;
    int irqCurr, irqPrev;
    
    // 防重放时间（毫秒）
    static const unsigned long CARD_COOLDOWN_MS = 3000;
    
    // 默认密钥
    uint8_t defaultKey[Utils::KEY_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    
    /**
     * 读取卡片UID
     */
    bool readCardUID(uint8_t* uid, uint8_t* uidLen);
    
    /**
     * 认证指定块
     */
    bool authenticateBlock(uint8_t* uid, uint8_t uidLen, uint8_t blockNumber, uint8_t* key);
    
    /**
     * 写入扇区尾部
     */
    bool writeSectorTrailer(uint8_t* newKey);
    
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
     * @param nfcModule PN532模块指针
     * @param db 卡片数据库指针
     * @param irq IRQ引脚
     * @param reset 复位引脚
     */
    NFCAuthenticator(Adafruit_PN532* nfcModule, CardDatabase* db, int irq, int reset);
    
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
    
    /**
     * 注册新卡片
     * @return 注册是否成功
     */
    bool registerNewCard();
};

#endif // NFCAUTHENTICATOR_H
