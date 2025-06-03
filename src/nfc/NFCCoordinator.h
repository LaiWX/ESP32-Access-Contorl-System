#ifndef NFCCOORDINATOR_H
#define NFCCOORDINATOR_H

#include <Adafruit_PN532.h>
#include <Arduino.h>

/**
 * NFC协调器
 * 统一管理PN532模块的访问，避免认证器和管理器之间的冲突
 */
class NFCCoordinator {
public:
    // NFC模式枚举
    enum NFCMode {
        MODE_IDLE,           // 空闲
        MODE_AUTHENTICATION, // 认证模式
        MODE_MANAGEMENT      // 管理模式（注册/擦除）
    };

private:
    Adafruit_PN532* nfc;
    int irqPin;
    int resetPin;
    
    NFCMode currentMode;
    unsigned long modeStartTime;
    
    // 状态管理
    enum NFCState {
        STATE_IDLE,
        STATE_DETECTING,
        STATE_CARD_PRESENT
    };
    
    NFCState currentState;
    int irqCurr, irqPrev;
    
    // 超时设置
    static const unsigned long MANAGEMENT_TIMEOUT = 10000; // 10秒管理模式超时

public:
    /**
     * 构造函数
     * @param nfcModule PN532模块指针
     * @param irq IRQ引脚
     * @param reset 复位引脚
     */
    NFCCoordinator(Adafruit_PN532* nfcModule, int irq, int reset);
    
    /**
     * 初始化NFC协调器
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 请求进入管理模式
     * @return 是否成功进入管理模式
     */
    bool requestManagementMode();
    
    /**
     * 退出管理模式，返回认证模式
     */
    void exitManagementMode();
    
    /**
     * 检查是否有卡片检测到（用于认证模式）
     * @return 是否检测到卡片
     */
    bool hasCardDetected();

    /**
     * 检查是否有卡片检测到（用于管理模式）
     * @return 是否检测到卡片
     */
    bool hasCardDetectedForManagement();

    /**
     * 检查是否有卡片持续在场被忽略
     * @return 是否有卡片持续在场
     */
    bool isCardPersistentlyPresent();
    
    /**
     * 读取卡片UID
     * @param uid 输出UID缓冲区
     * @param uidLength 输出UID长度
     * @return 读取是否成功
     */
    bool readCardUID(uint8_t* uid, uint8_t* uidLength);
    
    /**
     * 认证卡片块
     * @param uid 卡片UID
     * @param uidLength UID长度
     * @param blockNumber 块号
     * @param key 密钥
     * @return 认证是否成功
     */
    bool authenticateBlock(uint8_t* uid, uint8_t uidLength, uint8_t blockNumber, uint8_t* key);
    
    /**
     * 写入数据块
     * @param blockNumber 块号
     * @param data 数据
     * @return 写入是否成功
     */
    bool writeDataBlock(uint8_t blockNumber, uint8_t* data);
    
    /**
     * 获取当前模式
     * @return 当前模式
     */
    NFCMode getCurrentMode() const;
    
    /**
     * 处理NFC状态（在主循环中调用）
     */
    void handleNFC();
    
    /**
     * 重置NFC状态
     */
    void reset();

private:
    /**
     * 启动被动目标检测
     * @return 是否立即检测到卡片
     */
    bool startPassiveDetection();

    /**
     * 处理卡片检测逻辑
     * @return 是否检测到卡片
     */
    bool handleCardDetection();

    /**
     * 处理管理模式超时
     */
    void handleManagementTimeout();
};

#endif // NFCCOORDINATOR_H
