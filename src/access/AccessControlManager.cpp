#include "AccessControlManager.h"

AccessControlManager::AccessControlManager(DoorAccessExecutor* executor)
    : doorExecutor(executor), lastSuccessTime(0) {
}

AccessControlManager::~AccessControlManager() {
    // 注意：不要在这里删除认证器和管理操作，因为它们可能在其他地方管理
}

void AccessControlManager::addAuthenticator(IAuthenticator* authenticator) {
    if (authenticator != nullptr) {
        authenticators.push_back(authenticator);
        Serial.print("Added authenticator: ");
        Serial.println(authenticator->getName());
    }
}

void AccessControlManager::addManagementOperation(const String& type, IManagementOperation* operation) {
    if (operation != nullptr) {
        managementOperations[type] = operation;
        Serial.print("Added management operation: ");
        Serial.print(type);
        Serial.print(" (");
        Serial.print(operation->getName());
        Serial.println(")");
    }
}

bool AccessControlManager::initialize() {
    Serial.println("Initializing Access Control Manager...");
    
    bool allSuccess = true;
    
    // 初始化门禁执行器
    if (doorExecutor && !doorExecutor->initialize()) {
        Serial.println("Failed to initialize door executor");
        allSuccess = false;
    }
    
    // 初始化所有认证器
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
        Serial.println("All components initialized successfully");
    }
    
    return allSuccess;
}

void AccessControlManager::handleAuthentication() {
    // 处理支持异步操作的认证器
    for (auto* auth : authenticators) {
        if (auth->supportsAsyncOperations() && auth->hasCompletedOperation()) {
            bool success = auth->getOperationResult();
            Serial.print("Async operation completed from ");
            Serial.print(auth->getName());
            Serial.print(": ");
            Serial.println(success ? "Success" : "Failed");
            auth->clearOperationFlag();
        }
    }

    // 遍历所有认证器，检查是否有认证请求
    for (auto* auth : authenticators) {
        if (auth->hasAuthenticationRequest()) {
            Serial.print("Authentication request from: ");
            Serial.println(auth->getName());

            if (auth->authenticate()) {
                // 检查管理器级别的冷却期
                unsigned long currentTime = millis();
                if (currentTime - lastSuccessTime < MANAGER_COOLDOWN_MS) {
                    Serial.println("Authentication successful but in cooldown - IGNORED");
                    // 重置冷却期（同一张卡或不同卡都重置）
                    lastSuccessTime = currentTime;
                    return;
                }

                Serial.println("Authentication successful - OPENING DOOR");
                doorExecutor->executeSuccessAction();
                lastSuccessTime = currentTime;
            } else {
                Serial.println("Authentication failed - ACCESS DENIED");
                doorExecutor->executeFailureAction();
            }

            // 处理完一个认证请求后就返回，避免同时处理多个
            return;
        }
    }
}

void AccessControlManager::handleManagementOperations() {
    // 处理所有管理操作
    for (auto& pair : managementOperations) {
        IManagementOperation* operation = pair.second;
        if (operation) {
            operation->handleOperations();
        }
    }
    
    // 处理门禁执行器的时序
    if (doorExecutor) {
        doorExecutor->handleActions();
    }
}

bool AccessControlManager::executeManagementCommand(const String& command) {
    String type, action, param;
    
    if (!parseManagementCommand(command, type, action, param)) {
        Serial.println("Invalid command format. Use: type:action[:param]");
        Serial.println("Examples: card:register, card:delete:ABC123, card:list");
        return false;
    }
    
    // 查找对应的管理操作
    auto it = managementOperations.find(type);
    if (it == managementOperations.end()) {
        Serial.println("Unknown management type: " + type);
        listAvailableManagementTypes();
        return false;
    }
    
    IManagementOperation* operation = it->second;
    
    // 执行对应的动作
    if (action == "register") {
        return operation->registerNew();
    } else if (action == "delete") {
        if (param.length() == 0) {
            Serial.println("Delete command requires parameter: " + type + ":delete:<id>");
            return false;
        }
        return operation->deleteItem(param);
    } else if (action == "erase") {
        if (param.length() == 0) {
            Serial.println("Erase command requires parameter: " + type + ":erase:<id>");
            return false;
        }
        return operation->eraseAndDeleteItem(param);
    } else if (action == "list") {
        operation->listRegisteredItems();
        return true;
    } else if (action == "reset") {
        operation->reset();
        return true;
    } else {
        Serial.println("Unknown action: " + action);
        Serial.println("Available actions: register, delete, erase, list, reset");
        return false;
    }
}

void AccessControlManager::resetAll() {
    for (auto* auth : authenticators) {
        auth->reset();
    }
    
    for (auto& pair : managementOperations) {
        if (pair.second) {
            pair.second->reset();
        }
    }
    
    Serial.println("All components reset");
}

void AccessControlManager::listAvailableManagementTypes() {
    Serial.println("Available management types:");
    for (auto& pair : managementOperations) {
        Serial.print("- ");
        Serial.print(pair.first);
        Serial.print(" (");
        Serial.print(pair.second->getName());
        Serial.println(")");
    }
}

bool AccessControlManager::parseManagementCommand(const String& command, String& type, String& action, String& param) {
    int firstColon = command.indexOf(':');
    if (firstColon == -1) {
        return false;
    }
    
    type = command.substring(0, firstColon);
    
    int secondColon = command.indexOf(':', firstColon + 1);
    if (secondColon == -1) {
        action = command.substring(firstColon + 1);
        param = "";
    } else {
        action = command.substring(firstColon + 1, secondColon);
        param = command.substring(secondColon + 1);
    }
    
    type.trim();
    action.trim();
    param.trim();
    
    return type.length() > 0 && action.length() > 0;
}
