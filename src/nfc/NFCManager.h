#ifndef NFCMANAGER_H
#define NFCMANAGER_H

#include <Adafruit_PN532.h>
#include <Arduino.h>

/**
 * NFC管理器
 * 封装PN532的底层操作，处理IRQ逻辑和卡片检测
 * 解决startPassiveDetection()和IRQ引脚逻辑混合的问题
 */
class NFCManager {
public:
    // 卡片检测结果枚举
    enum CardDetectionResult {
        NO_CARD,           // 没有检测到卡片
        CARD_DETECTED,     // 检测到新卡片
        CARD_PERSISTENT    // 卡片持续在场
    };

private:
    Adafruit_PN532* nfc;
    int irqPin;
    int resetPin;
    
    // 卡片检测状态
    enum DetectionState {
        STATE_IDLE,        // 空闲状态
        STATE_DETECTING,   // 检测中（等待IRQ）
        STATE_CARD_PRESENT // 卡片在场
    };
    
    DetectionState currentState;
    int irqCurr, irqPrev;
    unsigned long lastDetectionTime;
    
    // 卡片持续在场的延迟时间（毫秒）
    static const unsigned long CARD_PERSISTENCE_DELAY = 500;

public:
    /**
     * 构造函数
     * @param irq IRQ引脚
     * @param reset 复位引脚
     */
    NFCManager(int irq, int reset);
    
    /**
     * 初始化NFC管理器
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 检测卡片状态
     * 封装了startPassiveDetection()和IRQ逻辑
     * @return 卡片检测结果
     */
    CardDetectionResult detectCard();
    
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
     * 重置NFC状态
     */
    void reset();
    
    /**
     * 获取IRQ引脚状态（用于调试）
     * @return IRQ引脚状态
     */
    bool getIRQState() const;

private:
    /**
     * 启动被动目标检测
     * @return 是否立即检测到卡片
     */
    bool startPassiveDetection();
    
    /**
     * 检查IRQ引脚状态变化
     * @return 是否检测到下降沿
     */
    bool checkIRQFallingEdge();
};

#endif // NFCMANAGER_H
