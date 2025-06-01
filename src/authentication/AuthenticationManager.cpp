#include "AuthenticationManager.h"

AuthenticationManager::AuthenticationManager(IActionExecutor* executor, CardDatabase* db, FileSystemManager* fsManager)
    : actionExecutor(executor), cardDatabase(db), fileSystemManager(fsManager), nfcAuth(nullptr) {
}

AuthenticationManager::~AuthenticationManager() {
    // 注意：不要在这里删除认证器，因为它们可能在其他地方管理
}

void AuthenticationManager::addAuthenticator(IAuthenticator* authenticator) {
    if (authenticator != nullptr) {
        authenticators.push_back(authenticator);
        Serial.print("Added authenticator: ");
        Serial.println(authenticator->getName());
    }
}

void AuthenticationManager::setNFCAuthenticator(NFCAuthenticator* nfcAuthenticator) {
    nfcAuth = nfcAuthenticator;
}

bool AuthenticationManager::initialize() {
    Serial.println("Initializing Authentication Manager...");
    
    bool allSuccess = true;
    for (auto* auth : authenticators) {
        if (!auth->initialize()) {
            Serial.print("Failed to initialize: ");
            Serial.println(auth->getName());
            allSuccess = false;
        } else {
            Serial.print("Initialized: ");
            Serial.println(auth->getName());
        }
    }
    
    if (allSuccess) {
        Serial.println("All authenticators initialized successfully");
    }
    
    return allSuccess;
}

void AuthenticationManager::handleAuthentication() {
    // 检查NFC认证器的操作完成状态
    if (nfcAuth != nullptr && nfcAuth->isOperationCompleted()) {
        bool success = nfcAuth->getOperationResult();
        NFCAuthenticator::OperationType opType = nfcAuth->getCurrentOperation();

        if (success) {
            if (opType == NFCAuthenticator::OP_ERASE) {
                // 擦除操作成功，从数据库删除卡片
                String uid = nfcAuth->getTargetUID();
                if (cardDatabase->removeCard(uid)) {
                    Serial.println("Card " + uid + " deleted from database");
                }
            }

            // 保存到文件系统
            if (fileSystemManager->saveCards()) {
                if (opType == NFCAuthenticator::OP_REGISTER) {
                    actionExecutor->executeRegistrationSuccessAction();
                } else if (opType == NFCAuthenticator::OP_ERASE) {
                    actionExecutor->executeDeletionSuccessAction();
                }
            } else {
                Serial.println("Failed to save changes to file system");
            }
        }
        nfcAuth->clearOperationFlag();
    }

    // 遍历所有认证器，检查是否有认证请求
    for (auto* auth : authenticators) {
        if (auth->hasAuthenticationRequest()) {
            Serial.print("Authentication request from: ");
            Serial.println(auth->getName());

            if (auth->authenticate()) {
                Serial.println("Authentication successful");
                actionExecutor->executeSuccessAction();
            } else {
                Serial.println("Authentication failed");
                actionExecutor->executeFailureAction();
            }

            // 处理完一个认证请求后就返回，避免同时处理多个
            return;
        }
    }
}

bool AuthenticationManager::registerNewCard() {
    if (nfcAuth == nullptr) {
        Serial.println("NFC authenticator not available for registration");
        return false;
    }

    // 启动非阻塞注册流程
    return nfcAuth->registerNewCard();
}

void AuthenticationManager::listRegisteredCards() {
    Serial.println("-- Registered Cards --");
    JsonArray cards = cardDatabase->getCards();
    
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

bool AuthenticationManager::deleteCard(const String& uid) {
    if (uid.length() == 0) {
        Serial.println("Usage: del <UID>");
        return false;
    }

    if (cardDatabase->removeCard(uid)) {
        if (fileSystemManager->saveCards()) {
            Serial.println("Deleted " + uid);
            actionExecutor->executeDeletionSuccessAction();
            return true;
        } else {
            Serial.println("Failed to save changes to file system");
            return false;
        }
    } else {
        Serial.println("Card not found: " + uid);
        return false;
    }
}

bool AuthenticationManager::eraseAndDeleteCard(const String& uid) {
    if (uid.length() == 0) {
        Serial.println("Usage: erase <UID>");
        return false;
    }

    if (nfcAuth == nullptr) {
        Serial.println("NFC authenticator not available for erase operation");
        return false;
    }

    // 检查卡片是否存在于数据库中
    if (!cardDatabase->isCardRegistered(uid)) {
        Serial.println("Card not found in database: " + uid);
        return false;
    }

    // 启动非阻塞擦除流程
    return nfcAuth->eraseCard(uid);
}

void AuthenticationManager::resetAll() {
    for (auto* auth : authenticators) {
        auth->reset();
    }
    Serial.println("All authenticators reset");
}
