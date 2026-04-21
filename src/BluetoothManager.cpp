/*
================================================================================
BLUETOOTH MANAGER IMPLEMENTATION
================================================================================
*/

#include "BluetoothManager.h"

BluetoothManager::BluetoothManager() 
    : SystemObject("BT_Manager", 5, 4096) // Имя, Приоритет, Стек
    , _isConnected(false)
    , _lastBlinkTime(0)
    , _blinkCount(0)
    , _ledState(false)
    , _blinkSequenceActive(false)
{
    _mutex = xSemaphoreCreateMutex();
    pinMode(BT_LED_PIN, OUTPUT);
    _setLed(false);
}

BluetoothManager::~BluetoothManager() {
    if (_mutex) vSemaphoreDelete(_mutex);
    _setLed(false);
}

bool BluetoothManager::start(const char* deviceName) {
    // Инициализация Bluetooth перед запуском задачи
    if (!_bt.begin(deviceName)) {
        Serial.println("[BT] Hardware init failed");
        _state = STATE_ERROR;
        return false;
    }
    Serial.printf("[BT] Initialized: %s\n", deviceName);
    
    // Запуск задачи базового класса
    return SystemObject::start();
}

void BluetoothManager::taskLoop() {
    // Бесконечный цикл задачи
    while (_state == STATE_RUNNING) {
        // 1. Проверка соединения
        bool currentConnection = _bt.hasClient();
        if (currentConnection != _isConnected) {
            _handleConnectionChange(currentConnection);
        }

        // 2. Обновление LED (всегда работает, даже без соединения)
        _updateLed();

        // 3. Небольшая задержка для стабильности
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}

void BluetoothManager::_handleConnectionChange(bool currentState) {
    _isConnected = currentState;
    
    if (_isConnected) {
        Serial.println("[BT] Connected");
        _blinkSequenceActive = false;
        _blinkCount = 0;
        _setLed(true); // Горит постоянно
    } else {
        Serial.println("[BT] Disconnected");
        _blinkSequenceActive = true;
        _blinkCount = 0;
        _lastBlinkTime = millis();
        _setLed(false);
    }
}

void BluetoothManager::_updateLed() {
    if (_isConnected) {
        return; // Если подключено - LED горит, логика не нужна
    }

    if (!_blinkSequenceActive) {
        _setLed(false);
        return;
    }

    uint32_t now = millis();
    if (now - _lastBlinkTime >= BLINK_INTERVAL_MS) {
        _lastBlinkTime = now;

        if (_blinkCount < BLINK_COUNT_TARGET) {
            _ledState = !_ledState;
            _setLed(_ledState);
            
            // Считаем только включения для подсчета "вспышек"
            if (_ledState) {
                _blinkCount++;
            }
        } else {
            // Лимит исчерпан
            _blinkSequenceActive = false;
            _setLed(false); // Погаснуть окончательно
        }
    }
}

void BluetoothManager::_setLed(bool state) {
    _ledState = state;
    digitalWrite(BT_LED_PIN, state ? HIGH : LOW);
}

// --- Потокобезопасные методы доступа ---

int BluetoothManager::available() {
    // В простой реализации можно без мьютекса, т.к. read атомарен
    return _bt.available();
}

int BluetoothManager::read() {
    return _bt.read();
}

size_t BluetoothManager::write(const uint8_t* buffer, size_t size) {
    // Защита записи от одновременного доступа
    if (xSemaphoreTake(_mutex, portMAX_DELAY) == pdTRUE) {
        size_t sent = _bt.write(buffer, size);
        xSemaphoreGive(_mutex);
        return sent;
    }
    return 0;
}

bool BluetoothManager::isConnected() {
    return _isConnected;
}