#ifndef CARDDATABASE_H
#define CARDDATABASE_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * 卡片数据库管理类
 * 负责卡片信息的存储、查询、添加和删除
 */
class CardDatabase {
private:
    JsonDocument database;
    
public:
    /**
     * 初始化数据库
     */
    void initialize();
    
    /**
     * 从文件加载数据库
     * @param data JSON数据
     */
    void loadFromJson(const JsonDocument& data);
    
    /**
     * 获取数据库JSON对象
     * @return JSON文档引用
     */
    JsonDocument& getDatabase();
    
    /**
     * 根据UID查找卡片
     * @param uid 卡片UID
     * @param keyHex 输出的密钥十六进制字符串
     * @return 是否找到卡片
     */
    bool findCardByUID(const String& uid, String& keyHex);
    
    /**
     * 检查卡片是否已注册
     * @param uid 卡片UID
     * @return 是否已注册
     */
    bool isCardRegistered(const String& uid);
    
    /**
     * 添加卡片到数据库
     * @param uid 卡片UID
     * @param keyHex 密钥十六进制字符串
     * @return 是否添加成功
     */
    bool addCard(const String& uid, const String& keyHex);
    
    /**
     * 从数据库删除卡片
     * @param uid 卡片UID
     * @return 是否删除成功
     */
    bool removeCard(const String& uid);
    
    /**
     * 获取所有已注册的卡片
     * @return 卡片数组
     */
    JsonArray getCards();
    
    /**
     * 获取已注册卡片数量
     * @return 卡片数量
     */
    size_t getCardCount();
};

#endif // CARDDATABASE_H
