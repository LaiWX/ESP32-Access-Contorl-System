#include "CardDatabase.h"

void CardDatabase::initialize() {
    database.to<JsonArray>();
}

void CardDatabase::loadFromJson(const JsonDocument& data) {
    database.set(data);
}

JsonDocument& CardDatabase::getDatabase() {
    return database;
}

bool CardDatabase::findCardByUID(const String& uid, String& keyHex) {
    JsonArray cards = database.as<JsonArray>();
    for (JsonObject card : cards) {
        if (card["uid"] == uid) {
            keyHex = card["key"].as<String>();
            return true;
        }
    }
    return false;
}

bool CardDatabase::isCardRegistered(const String& uid) {
    JsonArray cards = database.as<JsonArray>();
    for (JsonObject card : cards) {
        if (card["uid"] == uid) {
            return true;
        }
    }
    return false;
}

bool CardDatabase::addCard(const String& uid, const String& keyHex) {
    JsonArray cards = database.as<JsonArray>();
    
    // 检查卡片是否已存在
    for (JsonObject card : cards) {
        if (card["uid"] == uid) {
            return false; // 卡片已存在
        }
    }
    
    // 添加新卡片
    JsonObject newCard = cards.add<JsonObject>();
    newCard["uid"] = uid;
    newCard["key"] = keyHex;
    return true;
}

bool CardDatabase::removeCard(const String& uid) {
    JsonArray cards = database.as<JsonArray>();
    for (size_t i = 0; i < cards.size(); i++) {
        if (cards[i]["uid"] == uid) {
            cards.remove(i);
            return true;
        }
    }
    return false;
}

JsonArray CardDatabase::getCards() {
    return database.as<JsonArray>();
}

size_t CardDatabase::getCardCount() {
    return database.as<JsonArray>().size();
}
