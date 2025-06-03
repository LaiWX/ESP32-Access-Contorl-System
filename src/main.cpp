// Door Access System using ESP32, PN532, and MIFARE Classic
// Modularized version with authentication and execution components
// Features:
// 1. NFC authentication with automatic card detection
// 2. Manual trigger authentication via pin falling edge
// 3. Modular design with abstract interfaces
// 4. Anti-replay protection and cooldown mechanisms

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// 模块化组件
#include "authentication/AuthenticationManager.h"
#include "authentication/NFCAuthenticator.h"
#include "authentication/ManualTriggerAuthenticator.h"
#include "execution/LEDExecutor.h"
#include "execution/DoorLockExecutor.h"
#include "data/CardDatabase.h"
#include "data/FileSystemManager.h"
#include "utils/Utils.h"

// =============================================================================
// 硬件配置
// =============================================================================
// PN532 I2C pins
#define PN532_IRQ   4
#define PN532_RESET 5

// LED pin
#define LED_PIN 2

// 门锁控制引脚
#define DOOR_LOCK_PIN 12

// 手动触发引脚
#define MANUAL_TRIGGER_PIN 13

// =============================================================================
// 全局对象
// =============================================================================
// 硬件对象
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);

// 数据管理
CardDatabase cardDatabase;
FileSystemManager fileSystemManager(&cardDatabase);

// 执行器（可以在这里切换使用LED或门锁执行器）
LEDExecutor ledExecutor(LED_PIN);
// DoorLockExecutor doorLockExecutor(DOOR_LOCK_PIN, LED_PIN);

// 认证器
NFCAuthenticator nfcAuth(&nfc, &cardDatabase, PN532_IRQ, PN532_RESET);
ManualTriggerAuthenticator manualAuth(MANUAL_TRIGGER_PIN);

// 认证管理器
AuthenticationManager authManager(&ledExecutor, &cardDatabase, &fileSystemManager);

// =============================================================================
// 函数声明
// =============================================================================
void printWelcomeMessage();

// =============================================================================
// 串口命令处理
// =============================================================================
void processSerialCommand() {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("reg")) {
        authManager.registerNewCard();
    }
    else if (command.equalsIgnoreCase("list")) {
        authManager.listRegisteredCards();
    }
    else if (command.startsWith("del ")) {
        String uid = command.substring(4);
        uid.trim();
        authManager.deleteCard(uid);
    }
    else if (command.startsWith("erase ")) {
        String uid = command.substring(6);
        uid.trim();
        authManager.eraseAndDeleteCard(uid);
    }
    else if (command.equalsIgnoreCase("help")) {
        printWelcomeMessage();
    }
    else if (command.equalsIgnoreCase("reset")) {
        authManager.resetAll();
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
    }
}

void printWelcomeMessage() {
    Serial.println("\n=================================");
    Serial.println("    Door Access System Ready    ");
    Serial.println("=================================");
    Serial.println("Commands:");
    Serial.println("  reg         - Register new card (with timeout)");
    Serial.println("  list        - List all cards");
    Serial.println("  del <UID>   - Delete card");
    Serial.println("  erase <UID> - Erase card key and delete");
    Serial.println("  reset       - Reset all authenticators");
    Serial.println("  help        - Show this help");
    Serial.println("=================================");
    Serial.println("Authentication methods:");
    Serial.println("  - NFC card authentication");
    Serial.println("  - Manual trigger (pin " + String(MANUAL_TRIGGER_PIN) + ")");
    Serial.println("=================================");
}

// =============================================================================
// 初始化函数
// =============================================================================
bool initializeSystem() {
    Serial.println("Initializing Door Access System...");

    // 初始化文件系统
    if (!fileSystemManager.initialize()) {
        Serial.println("Failed to initialize file system");
        return false;
    }
    Serial.println("File system initialized");

    // 添加认证器到管理器
    authManager.addAuthenticator(&nfcAuth);
    authManager.addAuthenticator(&manualAuth);
    authManager.setNFCAuthenticator(&nfcAuth);

    // 初始化执行器
    if (!ledExecutor.initialize()) {
        Serial.println("Failed to initialize LED executor");
        return false;
    }
    Serial.println("LED executor initialized");

    // 初始化认证管理器
    if (!authManager.initialize()) {
        Serial.println("Failed to initialize authentication manager");
        return false;
    }

    Serial.println("System initialization completed successfully");
    return true;
}

// =============================================================================
// 主函数
// =============================================================================
void setup() {
    Serial.begin(115200);

    // 启动指示
    pinMode(LED_PIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_PIN, HIGH);
        delay(100);
        digitalWrite(LED_PIN, LOW);
        delay(100);
    }

    // 初始化系统
    if (!initializeSystem()) {
        Serial.println("System initialization failed!");
        while (true) {
            digitalWrite(LED_PIN, HIGH);
            delay(500);
            digitalWrite(LED_PIN, LOW);
            delay(500);
        }
    }

    printWelcomeMessage();
}

void loop() {
    // 处理串口命令
    if (Serial.available()) {
        processSerialCommand();
    }

    // 处理认证请求
    authManager.handleAuthentication();

    // 小延迟防止CPU过度使用
    delay(10);
}