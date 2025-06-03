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
#include "access/AccessControlManager.h"
#include "authentication/NFCAuthenticator.h"
#include "authentication/ManualTriggerAuthenticator.h"
#include "authentication/NFCCardManagerImpl.h"
#include "execution/DoorAccessExecutor.h"
#include "execution/LEDExecutor.h"
#include "execution/BuzzerExecutor.h"
#include "execution/ServoExecutor.h"
#include "nfc/NFCCoordinator.h"
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

// 蜂鸣器引脚
#define BUZZER_PIN 12

// 舵机引脚
#define SERVO_PIN 14

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

// NFC协调器
NFCCoordinator nfcCoordinator(&nfc, PN532_IRQ, PN532_RESET);

// 执行器
LEDExecutor ledExecutor(LED_PIN);
BuzzerExecutor buzzerExecutor(BUZZER_PIN);
ServoExecutor servoExecutor(SERVO_PIN);
DoorAccessExecutor doorExecutor(&ledExecutor, &buzzerExecutor, &servoExecutor);

// 认证器
NFCAuthenticator nfcAuth(&nfcCoordinator, &cardDatabase);
ManualTriggerAuthenticator manualAuth(MANUAL_TRIGGER_PIN);

// 卡片管理器
NFCCardManagerImpl cardManager(&nfcCoordinator, &cardDatabase, &fileSystemManager, &doorExecutor);

// 门禁控制管理器
AccessControlManager accessControl(&doorExecutor);

// =============================================================================
// 串口主界面
// =============================================================================
void printWelcomeMessage() {
    Serial.println("\n=================================");
    Serial.println("    Door Access System Ready    ");
    Serial.println("=================================");
    Serial.println("Commands:");
    Serial.println("  card:register       - Register new card");
    Serial.println("  card:list           - List all cards");
    Serial.println("  card:delete:<UID>   - Delete card");
    Serial.println("  card:erase:<UID>    - Erase card key and delete");
    Serial.println("  reset               - Reset all components");
    Serial.println("  help                - Show this help");
    Serial.println("=================================");
    Serial.println("Authentication methods:");
    Serial.println("  - NFC card authentication");
    Serial.println("  - Manual trigger (pin " + String(MANUAL_TRIGGER_PIN) + ")");
    Serial.println("=================================");
}

// =============================================================================
// 串口命令处理
// =============================================================================
void processSerialCommand() {
    String command = Serial.readStringUntil('\n');
    command.trim();

    if (command.equalsIgnoreCase("reset")) {
        accessControl.resetAll();
    }
    else if (command.equalsIgnoreCase("help")) {
        printWelcomeMessage();
    }
    else if (command.indexOf(':') != -1) {
        // 新的命令格式：type:action[:param]
        if (!accessControl.executeManagementCommand(command)) {
            Serial.println("Command failed. Type 'help' for available commands.");
        }
    }
    else {
        Serial.println("Unknown command. Type 'help' for available commands.");
        Serial.println("Use format: type:action[:param]");
        Serial.println("Example: card:register, card:delete:ABC123");
    }
}

// =============================================================================
// 初始化函数
// =============================================================================
bool initializeSystem() {
    Serial.println("Initializing Door Access System...");

    // 初始化NFC协调器
    if (!nfcCoordinator.initialize()) {
        Serial.println("Failed to initialize NFC coordinator");
        return false;
    }
    Serial.println("NFC coordinator initialized");

    // 初始化文件系统
    if (!fileSystemManager.initialize()) {
        Serial.println("Failed to initialize file system");
        return false;
    }
    Serial.println("File system initialized");

    // 添加认证器到门禁控制管理器
    accessControl.addAuthenticator(&nfcAuth);
    accessControl.addAuthenticator(&manualAuth);

    // 添加管理操作
    accessControl.addManagementOperation("card", &cardManager);

    // 初始化门禁控制管理器
    if (!accessControl.initialize()) {
        Serial.println("Failed to initialize access control manager");
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
    digitalWrite(LED_PIN, HIGH);
    delay(500);
    digitalWrite(LED_PIN, LOW);
    delay(100);

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

    // 处理NFC协调器
    nfcCoordinator.handleNFC();

    // 处理认证请求
    accessControl.handleAuthentication();

    // 处理管理操作
    accessControl.handleManagementOperations();

    // 小延迟防止CPU过度使用
    delay(10);
}