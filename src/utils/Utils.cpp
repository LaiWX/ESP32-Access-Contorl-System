#include "Utils.h"

String Utils::uidToString(uint8_t* uid, uint8_t len) {
    String result;
    for (uint8_t i = 0; i < len; i++) {
        if (uid[i] < 0x10) result += "0";
        result += String(uid[i], HEX);
    }
    result.toUpperCase();
    return result;
}

void Utils::generateRandomKey(uint8_t* key) {
    for (int i = 0; i < KEY_SIZE; i++) {
        key[i] = random(0, 256);
    }
}

String Utils::keyToHexString(uint8_t* key) {
    char keyHex[13] = {0};
    for (int i = 0; i < KEY_SIZE; i++) {
        sprintf(&keyHex[i * 2], "%02X", key[i]);
    }
    return String(keyHex);
}

void Utils::hexStringToKey(const String& hexString, uint8_t* key) {
    for (int i = 0; i < KEY_SIZE; i++) {
        key[i] = strtoul(hexString.substring(i * 2, i * 2 + 2).c_str(), NULL, 16);
    }
}
