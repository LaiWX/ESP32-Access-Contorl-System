#include "FileSystemManager.h"

const char* FileSystemManager::CARD_FILE = "/cards.json";

FileSystemManager::FileSystemManager(CardDatabase* db) : cardDatabase(db) {
}

bool FileSystemManager::initialize() {
    if (!SPIFFS.begin(true)) {
        Serial.println("SPIFFS Mount Failed");
        return false;
    }
    return loadCards();
}

bool FileSystemManager::saveCards() {
    File file = SPIFFS.open(CARD_FILE, FILE_WRITE);
    if (!file) {
        Serial.println("Failed to open card file for writing");
        return false;
    }
    serializeJsonPretty(cardDatabase->getDatabase(), file);
    file.close();
    return true;
}

bool FileSystemManager::loadCards() {
    if (SPIFFS.exists(CARD_FILE)) {
        File file = SPIFFS.open(CARD_FILE, FILE_READ);
        if (file) {
            JsonDocument tempDoc;
            DeserializationError err = deserializeJson(tempDoc, file);
            if (err) {
                Serial.println("Card database corrupt, resetting...");
                cardDatabase->initialize();
                file.close();
                return saveCards();
            }
            cardDatabase->loadFromJson(tempDoc);
            file.close();
            return true;
        }
    }
    
    // 文件不存在或打开失败，创建新数据库
    cardDatabase->initialize();
    return saveCards();
}
