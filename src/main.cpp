// Improved Door Access System using ESP32, PN532, and MIFARE Classic
// Features improved OOP architecture with state machine coordinator
// Key improvements:
// 1. State machine coordinator ensures mutual exclusion between authentication and management
// 2. Improved NFC wrapper that handles startPassiveDetection() and IRQ logic properly
// 3. Better separation of concerns following OOP principles

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_PN532.h>

// 改进的模块化组件
#include "system/SystemCoordinator.h"
#include "authentication/NFCAuthenticator.h"
#include "authentication/ManualTriggerAuthenticator.h"
#include "card_management/NFCCardManager.h"
#include "execution/DoorAccessExecutor.h"
#include "execution/LEDExecutor.h"
#include "execution/BuzzerExecutor.h"
#include "execution/ServoExecutor.h"
#include "nfc/NFCManager.h"
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

// NFC管理器（新的封装层）
NFCManager nfcManager(&nfc, PN532_IRQ, PN532_RESET);

// 执行器
LEDExecutor ledExecutor(LED_PIN);
BuzzerExecutor buzzerExecutor(BUZZER_PIN);
ServoExecutor servoExecutor(SERVO_PIN);
DoorAccessExecutor doorExecutor(&ledExecutor, &buzzerExecutor, &servoExecutor);

// 认证器（使用新的NFCManager）
NFCAuthenticator nfcAuth(&nfcManager, &cardDatabase);
ManualTriggerAuthenticator manualAuth(MANUAL_TRIGGER_PIN);

// 卡片管理器（使用新的NFCManager）
NFCCardManager cardManager(&nfcManager, &cardDatabase, &fileSystemManager, &doorExecutor);

// 系统协调器（新的状态机协调器）
SystemCoordinator systemCoordinator(&doorExecutor);

// =============================================================================
// 串口主界面
// =============================================================================
void printWelcomeMessage() {
    Serial.println("\n=================================");
    Serial.println("  Improved Door Access System   ");
    Serial.println("=================================");
    Serial.println("Architecture improvements:");
    Serial.println("  - State machine coordinator");
    Serial.println("  - Mutual exclusion between auth/mgmt");
    Serial.println("  - Improved NFC wrapper");
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
        systemCoordinator.resetAll();
    }
    else if (command.equalsIgnoreCase("help")) {
        printWelcomeMessage();
    }
    else if (command.indexOf(':') != -1) {
        // 新的命令格式：type:action[:param]
        if (!systemCoordinator.requestManagementState(command)) {
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
    Serial.println("Initializing Improved Door Access System...");

    // 初始化NFC管理器
    if (!nfcManager.initialize()) {
        Serial.println("Failed to initialize NFC manager");
        return false;
    }
    Serial.println("NFC manager initialized");

    // 初始化文件系统
    if (!fileSystemManager.initialize()) {
        Serial.println("Failed to initialize file system");
        return false;
    }
    Serial.println("File system initialized");

    // 添加认证器到系统协调器
    systemCoordinator.addAuthenticator(&nfcAuth);
    systemCoordinator.addAuthenticator(&manualAuth);

    // 添加管理操作
    systemCoordinator.addManagementOperation("card", &cardManager);

    // 初始化系统协调器
    if (!systemCoordinator.initialize()) {
        Serial.println("Failed to initialize system coordinator");
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

    // 主循环由系统协调器处理（状态机）
    systemCoordinator.handleLoop();

    // 小延迟防止CPU过度使用
    delay(10);
}
