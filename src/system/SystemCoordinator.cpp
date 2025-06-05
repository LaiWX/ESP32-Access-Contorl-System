#include "SystemCoordinator.h"

SystemCoordinator::SystemCoordinator(DoorAccessExecutor* executor)
    : currentState(STATE_IDLE), stateStartTime(0), doorExecutor(executor), lastSuccessTime(0) {
}

SystemCoordinator::~SystemCoordinator() {
    // 注意：不要在这里删除认证器和管理操作，因为它们可能在其他地方管理
}

void SystemCoordinator::addAuthenticator(IAuthenticator* authenticator) {
    if (authenticator != nullptr) {
        authenticators.push_back(authenticator);
        Serial.print("System Coordinator: Added authenticator: ");
        Serial.println(authenticator->getName());
    }
}

void SystemCoordinator::addManagementOperation(const String& type, IManagementOperation* operation) {
    if (operation != nullptr) {
        managementOperations[type] = operation;
        Serial.print("System Coordinator: Added management operation: ");
        Serial.print(type);
        Serial.print(" (");
        Serial.print(operation->getName());
        Serial.println(")");
    }
}

bool SystemCoordinator::initialize() {
    Serial.println("System Coordinator: Initializing...");
    
    bool allSuccess = true;
    
    // 初始化门禁执行器
    if (doorExecutor && !doorExecutor->initialize()) {
        Serial.println("System Coordinator: Failed to initialize door executor");
        allSuccess = false;
    }
    
    // 初始化所有认证器
    for (auto* auth : authenticators) {
        if (!auth->initialize()) {
            Serial.print("System Coordinator: Failed to initialize: ");
            Serial.println(auth->getName());
            allSuccess = false;
        } else {
            Serial.print("System Coordinator: Initialized: ");
            Serial.println(auth->getName());
        }
    }
    
    if (allSuccess) {
        Serial.println("System Coordinator: All components initialized successfully");
        transitionToState(STATE_AUTHENTICATION); // 默认进入认证状态
    }
    
    return allSuccess;
}

void SystemCoordinator::handleLoop() {
    switch (currentState) {
        case STATE_IDLE:
            handleIdleState();
            break;
            
        case STATE_AUTHENTICATION:
            handleAuthenticationState();
            break;
            
        case STATE_MANAGEMENT:
            handleManagementState();
            checkManagementTimeout();
            break;
    }

    // 重构后不再需要处理门禁执行器的时序
    // 各个执行器现在使用FreeRTOS任务自主管理时序
}

bool SystemCoordinator::handleCommand(const String& command) {
    if (command.equalsIgnoreCase("reset")) {
        resetAll();
        return true;
    }
    if (command.indexOf(':') != -1) {
        return executeManagementCommand(command);
    }
    return false;
}

void SystemCoordinator::exitManagementState() {
    if (currentState == STATE_MANAGEMENT) {
        Serial.println("System Coordinator: Exiting management state");
        transitionToState(STATE_AUTHENTICATION);
    }
}

SystemCoordinator::SystemState SystemCoordinator::getCurrentState() const {
    return currentState;
}

void SystemCoordinator::resetAll() {
    for (auto* auth : authenticators) {
        auth->reset();
    }
    
    for (auto& pair : managementOperations) {
        if (pair.second) {
            pair.second->reset();
        }
    }
    
    transitionToState(STATE_AUTHENTICATION);
    Serial.println("System Coordinator: All components reset");
}

void SystemCoordinator::listAvailableManagementTypes() {
    Serial.println("Available management types:");
    for (auto& pair : managementOperations) {
        Serial.print("- ");
        Serial.print(pair.first);
        Serial.print(" (");
        Serial.print(pair.second->getName());
        Serial.println(")");
    }
}

void SystemCoordinator::handleAuthenticationState() {
    // 处理支持异步操作的认证器
    for (auto* auth : authenticators) {
        if (auth->supportsAsyncOperations() && auth->hasCompletedOperation()) {
            bool success = auth->getOperationResult();
            Serial.print("System Coordinator: Async operation completed from ");
            Serial.print(auth->getName());
            Serial.print(": ");
            Serial.println(success ? "Success" : "Failed");
            auth->clearOperationFlag();
        }
    }

    // 遍历所有认证器，检查是否有认证请求
    for (auto* auth : authenticators) {
        if (auth->hasAuthenticationRequest()) {
            Serial.print("System Coordinator: Authentication request from: ");
            Serial.println(auth->getName());

            if (auth->authenticate()) {
                // 检查冷却期
                unsigned long currentTime = millis();
                if (currentTime - lastSuccessTime < AUTH_COOLDOWN_MS) {
                    Serial.println("System Coordinator: Authentication successful but in cooldown - IGNORED");
                    lastSuccessTime = currentTime;
                    return;
                }

                Serial.println("System Coordinator: Authentication successful - OPENING DOOR");
                doorExecutor->executeSuccessAction();
                lastSuccessTime = currentTime;
            } else {
                Serial.println("System Coordinator: Authentication failed - ACCESS DENIED");
                doorExecutor->executeFailureAction();
            }

            // 处理完一个认证请求后就返回，避免同时处理多个
            return;
        }
    }
}

void SystemCoordinator::handleManagementState() {
    // 处理所有管理操作
    for (auto& pair : managementOperations) {
        IManagementOperation* operation = pair.second;
        if (operation) {
            operation->handleOperations();

            // 检查操作是否刚刚完成
            if (operation->hasCompletedOperation()) {
                Serial.println("System Coordinator: Management operation completed, returning to authentication state");
                transitionToState(STATE_AUTHENTICATION);
                return; // 立即退出，避免处理其他操作
            }
        }
    }
}

void SystemCoordinator::handleIdleState() {
    // 空闲状态暂时不做任何处理
    // 可以在这里添加系统监控或维护任务
}

void SystemCoordinator::checkManagementTimeout() {
    if (millis() - stateStartTime > MANAGEMENT_TIMEOUT_MS) {
        Serial.println("System Coordinator: Management state timeout, returning to authentication state");
        transitionToState(STATE_AUTHENTICATION);
    }
}

bool SystemCoordinator::executeManagementCommand(const String& command) {
    String type, action, param;

    if (!parseManagementCommand(command, type, action, param)) {
        Serial.println("System Coordinator: Invalid command format. Use: type:action[:param]");
        Serial.println("Examples: card:register, card:delete:ABC123, card:list");
        return false;
    }

    if (currentState == STATE_MANAGEMENT) {
        Serial.println("System Coordinator: Already in management state");
    }

    if (action != "list") {
        Serial.println("System Coordinator: Entering management state");
        transitionToState(STATE_MANAGEMENT);
    }

    // 查找对应的管理操作
    auto it = managementOperations.find(type);
    if (it == managementOperations.end()) {
        Serial.println("System Coordinator: Unknown management type: " + type);
        listAvailableManagementTypes();
        return false;
    }

    IManagementOperation* operation = it->second;

    // 执行对应的动作
    if (action == "register") {
        return operation->registerNew();
    } else if (action == "delete") {
        if (param.length() == 0) {
            Serial.println("System Coordinator: Delete command requires parameter: " + type + ":delete:<id>");
            return false;
        }
        return operation->deleteItem(param);
    } else if (action == "erase") {
        if (param.length() == 0) {
            Serial.println("System Coordinator: Erase command requires parameter: " + type + ":erase:<id>");
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
        Serial.println("System Coordinator: Unknown action: " + action);
        Serial.println("Available actions: register, delete, erase, list, reset");
        return false;
    }
}

bool SystemCoordinator::parseManagementCommand(const String& command, String& type, String& action, String& param) {
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

void SystemCoordinator::transitionToState(SystemState newState) {
    if (currentState != newState) {
        Serial.print("System Coordinator: State transition: ");

        // 打印当前状态
        switch (currentState) {
            case STATE_IDLE: Serial.print("IDLE"); break;
            case STATE_AUTHENTICATION: Serial.print("AUTHENTICATION"); break;
            case STATE_MANAGEMENT: Serial.print("MANAGEMENT"); break;
        }

        Serial.print(" -> ");

        // 打印新状态
        switch (newState) {
            case STATE_IDLE: Serial.println("IDLE"); break;
            case STATE_AUTHENTICATION: Serial.println("AUTHENTICATION"); break;
            case STATE_MANAGEMENT: Serial.println("MANAGEMENT"); break;
        }

        currentState = newState;
        stateStartTime = millis();
    }
}
