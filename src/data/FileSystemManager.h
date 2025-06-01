#ifndef FILESYSTEMMANAGER_H
#define FILESYSTEMMANAGER_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "CardDatabase.h"

/**
 * 文件系统管理类
 * 负责文件系统的初始化和数据的持久化存储
 */
class FileSystemManager {
private:
    static const char* CARD_FILE;
    CardDatabase* cardDatabase;
    
public:
    /**
     * 构造函数
     * @param db 卡片数据库指针
     */
    FileSystemManager(CardDatabase* db);
    
    /**
     * 初始化文件系统
     * @return 初始化是否成功
     */
    bool initialize();
    
    /**
     * 保存卡片数据库到文件
     * @return 保存是否成功
     */
    bool saveCards();
    
    /**
     * 从文件加载卡片数据库
     * @return 加载是否成功
     */
    bool loadCards();
};

#endif // FILESYSTEMMANAGER_H
