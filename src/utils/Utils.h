#ifndef UTILS_H
#define UTILS_H

#include <Arduino.h>

/**
 * 通用工具函数类
 */
class Utils {
public:
    /**
     * 将UID字节数组转换为字符串
     * @param uid UID字节数组
     * @param len 数组长度
     * @return UID字符串
     */
    static String uidToString(uint8_t* uid, uint8_t len);
    
    /**
     * 生成随机密钥
     * @param key 输出的密钥数组
     */
    static void generateRandomKey(uint8_t* key);
    
    /**
     * 将密钥转换为十六进制字符串
     * @param key 密钥数组
     * @return 十六进制字符串
     */
    static String keyToHexString(uint8_t* key);
    
    /**
     * 将十六进制字符串转换为密钥
     * @param hexString 十六进制字符串
     * @param key 输出的密钥数组
     */
    static void hexStringToKey(const String& hexString, uint8_t* key);
    
    // 常量定义
    static const int KEY_SIZE = 6;
};

#endif // UTILS_H
