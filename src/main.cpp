// Door Access System using ESP32, PN532, and MIFARE Classic
// Features:
// 1. Automatic card authentication in main loop
// 2. Anti-replay protection to prevent repeated door opening
// 3. Register blank card via Serial commands
// 4. List and delete cards via Serial commands

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <Adafruit_PN532.h>

// =============================================================================
// 配置
// =============================================================================
// PN532 I2C pins
#define PN532_IRQ   34
#define PN532_RESET 5

// LED pin
#define LED_PIN 2

// 门锁控制引脚
#define DOOR_LOCK_PIN 12

// MIFARE Classic settings
#define SECTOR_TRAILER_BLOCK 7
#define AUTH_BLOCK 4
#define KEY_SIZE 6
#define TRAILER_SIZE 16

// 防重放时间（同一张卡在成功认证后需要等待的时间）
#define CARD_COOLDOWN_MS 3000

// 文件系统
const char* CARD_FILE = "/cards.json";


// =============================================================================
// 函数声明
// =============================================================================
void printWelcomeMessage();

// =============================================================================
// 全局变量
// =============================================================================
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
StaticJsonDocument<1024> cardDatabase;
uint8_t defaultKey[KEY_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

// 门禁系统状态
enum SystemState {
  STATE_IDLE,
  STATE_DETECTING,
  STATE_CARD_PRESENT
};

SystemState currentState = STATE_IDLE;
unsigned long lastCardTime = 0;
String lastCardUID = "";
bool readerDisabled = false;
unsigned long timeLastCardRead = 0;
int irqCurr = HIGH;
int irqPrev = HIGH;

// =============================================================================
// 工具函数
// =============================================================================
String uidToString(uint8_t* uid, uint8_t len) {
  String result;
  for (uint8_t i = 0; i < len; i++) {
    if (uid[i] < 0x10) result += "0";
    result += String(uid[i], HEX);
  }
  result.toUpperCase();
  return result;
}

void generateRandomKey(uint8_t* key) {
  for (int i = 0; i < KEY_SIZE; i++) {
    key[i] = random(0, 256);
  }
}

String keyToHexString(uint8_t* key) {
  char keyHex[13] = {0};
  for (int i = 0; i < KEY_SIZE; i++) {
    sprintf(&keyHex[i * 2], "%02X", key[i]);
  }
  return String(keyHex);
}

void hexStringToKey(const String& hexString, uint8_t* key) {
  for (int i = 0; i < KEY_SIZE; i++) {
    key[i] = strtoul(hexString.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
  }
}

// =============================================================================
// LED 和门锁控制
// =============================================================================
void blinkLED(int times = 3, int delayMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
}

void unlockDoor() {
  Serial.println("Unlocking door...");
  //digitalWrite(DOOR_LOCK_PIN, HIGH);
  blinkLED(2, 200); // 快速闪烁2次表示门已开

  // 保持门锁开启状态3秒
  delay(3000);

  // 重新锁门
  //digitalWrite(DOOR_LOCK_PIN, LOW);
  Serial.println("Door locked");
}

// =============================================================================
// 文件系统操作
// =============================================================================
void saveCards() {
  File file = SPIFFS.open(CARD_FILE, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open card file for writing");
    return;
  }
  serializeJsonPretty(cardDatabase, file);
  file.close();
}

void loadCards() {
  if (SPIFFS.exists(CARD_FILE)) {
    File file = SPIFFS.open(CARD_FILE, FILE_READ);
    if (file) {
      DeserializationError err = deserializeJson(cardDatabase, file);
      if (err) {
        Serial.println("Card database corrupt, resetting...");
        cardDatabase.to<JsonArray>();
        saveCards();
      }
      file.close();
      return;
    }
  }

  // 文件不存在或打开失败，创建新数据库
  cardDatabase.to<JsonArray>();
  saveCards();
}

// =============================================================================
// NFC 操作
// =============================================================================
bool readCardUID(uint8_t* uid, uint8_t* uidLen) {
  return nfc.readPassiveTargetID(PN532_MIFARE_ISO14443A, uid, uidLen);
}

bool authenticateBlock(uint8_t* uid, uint8_t uidLen, uint8_t blockNumber, uint8_t* key) {
  return nfc.mifareclassic_AuthenticateBlock(uid, uidLen, blockNumber, 0, key);
}

bool writeSectorTrailer(uint8_t* newKey) {
  uint8_t trailer[TRAILER_SIZE];

  // Set new KeyA
  memcpy(trailer, newKey, KEY_SIZE);

  // Set default access bits
  memcpy(trailer + 6, "\xFF\x07\x80\x69", 4);

  // Keep default KeyB
  memcpy(trailer + 10, defaultKey, KEY_SIZE);

  return nfc.mifareclassic_WriteDataBlock(SECTOR_TRAILER_BLOCK, trailer);
}

// =============================================================================
// 卡片管理
// =============================================================================
bool findCardByUID(const String& uid, String& keyHex) {
  JsonArray cards = cardDatabase.as<JsonArray>();
  for (JsonObject card : cards) {
    if (card["uid"] == uid) {
      keyHex = card["key"].as<String>();
      return true;
    }
  }
  return false;
}

bool isCardRegistered(const String& uid) {
  JsonArray cards = cardDatabase.as<JsonArray>();
  for (JsonObject card : cards) {
    if (card["uid"] == uid) {
      return true;
    }
  }
  return false;
}

bool addCardToDatabase(const String& uid, const String& keyHex) {
  JsonArray cards = cardDatabase.as<JsonArray>();

  // 检查卡片是否已存在
  for (JsonObject card : cards) {
    if (card["uid"] == uid) {
      return false; // 卡片已存在
    }
  }

  // 添加新卡片
  JsonObject newCard = cards.createNestedObject();
  newCard["uid"] = uid;
  newCard["key"] = keyHex;
  saveCards();
  return true;
}

bool removeCardFromDatabase(const String& uid) {
  JsonArray cards = cardDatabase.as<JsonArray>();
  for (size_t i = 0; i < cards.size(); i++) {
    if (cards[i]["uid"] == uid) {
      cards.remove(i);
      saveCards();
      return true;
    }
  }
  return false;
}

// =============================================================================
// 自动认证逻辑
// =============================================================================
void handleCardAuthentication(uint8_t* uid, uint8_t uidLen) {
  String uidString = uidToString(uid, uidLen);

  // 检查是否在冷却期内（同一张卡连续认证）
  if (uidString == lastCardUID && (millis() - lastCardTime) < CARD_COOLDOWN_MS) {
    Serial.println("Card in cooldown, ignored.");
    return;
  }

  Serial.print("Card detected: ");
  Serial.println(uidString);

  // 在数据库中查找卡片
  String keyHex;
  if (!findCardByUID(uidString, keyHex)) {
    Serial.println("Card not registered");
    return;
  }

  // 获取存储的密钥并认证
  uint8_t key[KEY_SIZE];
  hexStringToKey(keyHex, key);

  if (authenticateBlock(uid, uidLen, AUTH_BLOCK, key)) {
    Serial.println("Authentication successful");
    unlockDoor(); // 开门并闪烁LED

    // 更新最后认证的卡片和时间
    lastCardUID = uidString;
    lastCardTime = millis();
  } else {
    Serial.println("Authentication failed");
    blinkLED(1, 500); // 长闪一次表示失败
  }
}

void startNFCListening() {
  // 重置IRQ状态
  irqPrev = irqCurr = HIGH;

  // 启动被动检测
  if (!nfc.startPassiveTargetIDDetection(PN532_MIFARE_ISO14443A)) {
    // 返回false：没有立即检测到卡片，进入被动监听状态
    currentState = STATE_DETECTING;
    Serial.println("Waiting for card...");
  } else {
    // 返回true：卡片在极短时间内就被读到（卡片未移开）
    currentState = STATE_CARD_PRESENT;
    Serial.println("Card already present, ignoring...");

    // 直接读取卡片但不处理认证
    uint8_t uid[7] = {0};
    uint8_t uidLength = 0;
    if (nfc.readDetectedPassiveTargetID(uid, &uidLength)) {
      String uidString = uidToString(uid, uidLength);
      Serial.print("Ignored card: ");
      Serial.println(uidString);
    }
    delay(100);
  }
}

// =============================================================================
// 命令处理
// =============================================================================
void handleRegisterCommand() {
  Serial.println("-- Tap blank card to register --");

  uint8_t uid[7], uidLen;
  if (!readCardUID(uid, &uidLen)) {
    Serial.println("No card detected");
    return;
  }

  String uidString = uidToString(uid, uidLen);
  Serial.print("UID: ");
  Serial.println(uidString);

  // 检查卡片是否已注册
  if (isCardRegistered(uidString)) {
    Serial.println("Card already registered");
    return;
  }

  // 使用默认密钥认证
  if (!authenticateBlock(uid, uidLen, SECTOR_TRAILER_BLOCK, defaultKey)) {
    Serial.println("Authentication with default key failed");
    return;
  }

  // 生成并写入新密钥
  uint8_t newKey[KEY_SIZE];
  generateRandomKey(newKey);

  if (!writeSectorTrailer(newKey)) {
    Serial.println("Failed to write sector trailer");
    return;
  }

  // 添加到数据库
  String keyHex = keyToHexString(newKey);
  if (addCardToDatabase(uidString, keyHex)) {
    Serial.println("Card registered successfully");
    blinkLED(3, 100); // 闪烁3次表示成功
  } else {
    Serial.println("Failed to save card to database");
  }
}

void handleListCommand() {
  Serial.println("-- Registered Cards --");
  JsonArray cards = cardDatabase.as<JsonArray>();

  if (cards.size() == 0) {
    Serial.println("No cards registered");
    return;
  }

  for (JsonObject card : cards) {
    Serial.print(card["uid"].as<const char*>());
    Serial.print(" : ");
    Serial.println(card["key"].as<const char*>());
  }
}

void handleDeleteCommand(const String& uid) {
  if (uid.length() == 0) {
    Serial.println("Usage: del <UID>");
    return;
  }

  if (removeCardFromDatabase(uid)) {
    Serial.println("Deleted " + uid);
    blinkLED(2, 200); // 闪烁2次表示删除成功
  } else {
    Serial.println("Card not found: " + uid);
  }
}



void processSerialCommand() {
  String command = Serial.readStringUntil('\n');
  command.trim();

  if (command.equalsIgnoreCase("reg")) {
    handleRegisterCommand();
  }
  else if (command.equalsIgnoreCase("list")) {
    handleListCommand();
  }
  else if (command.startsWith("del")) {
    String uid = command.substring(3);
    uid.trim();
    handleDeleteCommand(uid);
  }
  else if (command.equalsIgnoreCase("help")) {
    printWelcomeMessage();
  }
  else {
    Serial.println("Unknown command. Type 'help' for available commands.");
  }
}

// =============================================================================
// 初始化
// =============================================================================
bool initializeFileSystem() {
  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS Mount Failed");
    return false;
  }
  loadCards();
  return true;
}

bool initializeNFC() {
  nfc.begin();
  uint32_t version = nfc.getFirmwareVersion();
  if (!version) {
    Serial.println("PN532 not found");
    return false;
  }

  Serial.print("Found PN532 with firmware version: 0x");
  Serial.println(version, HEX);

  nfc.SAMConfig();

  // 配置IRQ引脚
  pinMode(PN532_IRQ, INPUT_PULLUP);
  irqPrev = irqCurr = digitalRead(PN532_IRQ);

  return true;
}

void initializeLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void initializeDoorLock() {
  //pinMode(DOOR_LOCK_PIN, OUTPUT);
  //digitalWrite(DOOR_LOCK_PIN, LOW); // 初始状态为锁定
}

void printWelcomeMessage() {
  Serial.println("\n=================================");
  Serial.println("    Door Access System Ready    ");
  Serial.println("=================================");
  Serial.println("Commands:");
  Serial.println("  reg       - Register new card");
  Serial.println("  list      - List all cards");
  Serial.println("  del <UID> - Delete card");
  Serial.println("  help      - Show this help");
  Serial.println("=================================");
}

// =============================================================================
// 主函数
// =============================================================================
void setup() {
  initializeLED();
  initializeDoorLock();

  Serial.begin(115200);
  for (int i = 0; i < 3; i++) {
    blinkLED(1, 100);
    delay(100);
  }

  if (!initializeFileSystem()) {
    Serial.println("Failed to initialize file system");
    while (true) delay(1000);
  }

  if (!initializeNFC()) {
    Serial.println("Failed to initialize NFC reader");
    while (true) delay(1000);
  }

  printWelcomeMessage();

  // 启动第一次NFC监听
  startNFCListening();
}

void loop() {
  // 处理串口命令
  if (Serial.available()) {
    processSerialCommand();
  }

  // 门禁系统状态机
  switch (currentState) {
    case STATE_IDLE:
      // 空闲状态，启动NFC监听
      startNFCListening();
      break;

    case STATE_DETECTING:
      // 检测IRQ引脚状态
      irqCurr = digitalRead(PN532_IRQ);

      // 当IRQ引脚从高变低时，表示卡片靠近
      if (irqCurr == LOW && irqPrev == HIGH) {
        uint8_t uid[7] = {0};
        uint8_t uidLength = 0;

        // 读取卡片UID
        if (nfc.readDetectedPassiveTargetID(uid, &uidLength)) {
          handleCardAuthentication(uid, uidLength);
        }

        // 重置状态，准备下一次检测
        currentState = STATE_IDLE;
      }

      irqPrev = irqCurr;
      break;

    case STATE_CARD_PRESENT:
      // 卡片持续存在状态，不做任何处理
      // 等待卡片移开后再进行下一次检测
      if (digitalRead(PN532_IRQ) == HIGH) {
        currentState = STATE_IDLE;
      }
      break;
  }

  // 小延迟防止CPU过度使用
  delay(10);
}