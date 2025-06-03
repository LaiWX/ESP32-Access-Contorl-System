#include "LEDExecutor.h"

LEDExecutor::LEDExecutor(int pin) : ledPin(pin), isBlinking(false), blinkCount(0),
    targetBlinkCount(0), lastBlinkTime(0), blinkInterval(0), ledState(false) {
}

bool LEDExecutor::initialize() {
    pinMode(ledPin, OUTPUT);
    digitalWrite(ledPin, LOW);
    return true;
}

void LEDExecutor::blinkLED(int times, int delayMs) {
    for (int i = 0; i < times; i++) {
        digitalWrite(ledPin, HIGH);
        delay(delayMs);
        digitalWrite(ledPin, LOW);
        delay(delayMs);
    }
}

void LEDExecutor::executeSuccessAction() {
    Serial.println("Executing success action (LED)");
    blinkLED(2, 200); // 快速闪烁2次表示成功

    Serial.println("Action completed");
}

void LEDExecutor::executeFailureAction() {
    Serial.println("Executing failure action (LED)");
    blinkLED(1, 500); // 长闪1次表示失败
}

void LEDExecutor::executeRegistrationSuccessAction() {
    Serial.println("Executing registration success action (LED)");
    blinkLED(3, 100); // 闪烁3次表示注册成功
}

void LEDExecutor::executeDeletionSuccessAction() {
    Serial.println("Executing deletion success action (LED)");
    blinkLED(2, 200); // 闪烁2次表示删除成功
}

const char* LEDExecutor::getName() const {
    return "LED Executor";
}

void LEDExecutor::turnOn() {
    stopBlinking(); // 停止任何正在进行的闪烁
    digitalWrite(ledPin, HIGH);
    ledState = true;
}

void LEDExecutor::turnOff() {
    stopBlinking(); // 停止任何正在进行的闪烁
    digitalWrite(ledPin, LOW);
    ledState = false;
}

void LEDExecutor::blink(int times, unsigned long intervalMs) {
    targetBlinkCount = times * 2; // 每次闪烁包括开和关
    blinkCount = 0;
    blinkInterval = intervalMs;
    lastBlinkTime = millis();
    isBlinking = true;

    // 立即开始第一次闪烁
    digitalWrite(ledPin, HIGH);
    ledState = true;
}

void LEDExecutor::handleBlinking() {
    if (!isBlinking) {
        return;
    }

    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= blinkInterval) {
        // 切换LED状态
        ledState = !ledState;
        digitalWrite(ledPin, ledState ? HIGH : LOW);

        blinkCount++;
        lastBlinkTime = currentTime;

        // 检查是否完成所有闪烁
        if (blinkCount >= targetBlinkCount) {
            stopBlinking();
        }
    }
}

void LEDExecutor::stopBlinking() {
    isBlinking = false;
    blinkCount = 0;
    targetBlinkCount = 0;
    digitalWrite(ledPin, LOW);
    ledState = false;
}

bool LEDExecutor::isBlinkingActive() const {
    return isBlinking;
}
