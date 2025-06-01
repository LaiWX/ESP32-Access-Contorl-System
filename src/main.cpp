// Door Access System using ESP32, PN532, and MIFARE Classic
// Features:
// 1. Register blank card: generate random key, write to sector trailer (block 7), record mapping in SPIFFS JSON
// 2. Authenticate card: read UID, lookup key, perform Crypto1 authentication on sector1 block4, blink LED on success
// 3. List and delete cards: manage registered cards via Serial commands

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

// MIFARE Classic settings
#define SECTOR_TRAILER_BLOCK 7
#define AUTH_BLOCK 4
#define KEY_SIZE 6
#define TRAILER_SIZE 16

// 文件系统
const char* CARD_FILE = "/cards.json";

// =============================================================================
// 全局变量
// =============================================================================
Adafruit_PN532 nfc(PN532_IRQ, PN532_RESET);
StaticJsonDocument<1024> cardDatabase;
uint8_t defaultKey[KEY_SIZE] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };

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
// LED 控制
// =============================================================================
void blinkLED(int times = 3, int delayMs = 100) {
  for (int i = 0; i < times; i++) {
    digitalWrite(LED_PIN, HIGH);
    delay(delayMs);
    digitalWrite(LED_PIN, LOW);
    delay(delayMs);
  }
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
  File file = SPIFFS.open(CARD_FILE, FILE_READ);
  if (!file) {
    // Create empty array if file doesn't exist
    cardDatabase.to<JsonArray>();
    saveCards();
  } else {
    DeserializationError err = deserializeJson(cardDatabase, file);
    if (err) {
      // Reset if file is corrupt
      Serial.println("Card database corrupt, resetting...");
      cardDatabase.to<JsonArray>();
      saveCards();
    }
    file.close();
  }
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

  // Check if card is already registered
  if (isCardRegistered(uidString)) {
    Serial.println("Card already registered");
    return;
  }

  // Authenticate with default key
  if (!authenticateBlock(uid, uidLen, SECTOR_TRAILER_BLOCK, defaultKey)) {
    Serial.println("Authentication with default key failed");
    return;
  }

  // Generate and write new key
  uint8_t newKey[KEY_SIZE];
  generateRandomKey(newKey);
  
  if (!writeSectorTrailer(newKey)) {
    Serial.println("Failed to write sector trailer");
    return;
  }

  // Add to database
  String keyHex = keyToHexString(newKey);
  if (addCardToDatabase(uidString, keyHex)) {
    Serial.println("Card registered successfully");
  } else {
    Serial.println("Failed to save card to database");
  }
}

void handleAuthenticateCommand() {
  Serial.println("-- Tap card to authenticate --");
  
  uint8_t uid[7], uidLen;
  if (!readCardUID(uid, &uidLen)) {
    Serial.println("No card detected");
    return;
  }

  String uidString = uidToString(uid, uidLen);
  Serial.print("UID: ");
  Serial.println(uidString);

  // Find card in database
  String keyHex;
  if (!findCardByUID(uidString, keyHex)) {
    Serial.println("Card not registered");
    return;
  }

  // Get stored key and authenticate
  uint8_t key[KEY_SIZE];
  hexStringToKey(keyHex, key);

  if (authenticateBlock(uid, uidLen, AUTH_BLOCK, key)) {
    Serial.println("Authentication successful");
    blinkLED();
  } else {
    Serial.println("Authentication failed");
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
  else if (command.equalsIgnoreCase("auth")) {
    handleAuthenticateCommand();
  } 
  else if (command.equalsIgnoreCase("list")) {
    handleListCommand();
  } 
  else if (command.startsWith("del")) {
    String uid = command.substring(3);
    uid.trim();
    handleDeleteCommand(uid);
  } 
  else {
    Serial.println("Unknown command");
    Serial.println("Available commands: reg, auth, list, del <UID>");
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
  return true;
}

void initializeLED() {
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void printWelcomeMessage() {
  Serial.println("=================================");
  Serial.println("    Door Access System Ready    ");
  Serial.println("=================================");
  Serial.println("Commands:");
  Serial.println("  reg       - Register new card");
  Serial.println("  auth      - Authenticate card");
  Serial.println("  list      - List all cards");
  Serial.println("  del <UID> - Delete card");
  Serial.println("=================================");
}

// =============================================================================
// 主函数
// =============================================================================
void setup() {
  initializeLED();

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
}

void loop() {
  if (Serial.available()) {
    processSerialCommand();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(10);
}
