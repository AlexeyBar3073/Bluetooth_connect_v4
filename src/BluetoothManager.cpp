/*
================================================================================
BLUETOOTHMANAGER.CPP — РЕАЛИЗАЦИЯ МЕНЕДЖЕРА BLUETOOTH
================================================================================
*/

#include "BluetoothManager.h"
#include "DataRouter.h"


// ============================================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================================

BluetoothManager::BluetoothManager(const char* name, UBaseType_t priority, 
                                   uint32_t stackSize, BaseType_t core)
    : BaseObject(name, priority, stackSize, core)
    , _outQueue(nullptr)
    , _isConnected(false)
    , _lastBlinkTime(0)
    , _blinkCount(0)
    , _ledState(false)
    , _blinkSequenceActive(false)
{
    pinMode(BT_LED_PIN, OUTPUT);
    _setLed(false);
}

BluetoothManager::~BluetoothManager() {
    if (_outQueue) {
        vQueueDelete(_outQueue);
        _outQueue = nullptr;
    }
    _setLed(false);
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ
// ============================================================================

bool BluetoothManager::init(const char* deviceName) {
    DBG_PRINTF("[BT] Initializing '%s'...", deviceName);
    
    if (!_bt.begin(deviceName)) {
        DBG_PRINTLN("[BT] ERROR: Hardware init failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    DBG_PRINTF("[BT] Hardware OK: '%s'", deviceName);
    
    _outQueue = xQueueCreate(BT_OUT_QUEUE_DEPTH, sizeof(BtOutMessage));
    if (!_outQueue) {
        DBG_PRINTLN("[BT] ERROR: Output queue creation failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    DBG_PRINTF("[BT] Output queue created (depth=%d, msg_size=%u)", 
               BT_OUT_QUEUE_DEPTH, (unsigned)sizeof(BtOutMessage));
    
    pinMode(BT_LED_PIN, OUTPUT);
    _setLed(false);
    
    return BaseObject::start();
}

void BluetoothManager::stop() {
    DBG_PRINTLN("[BT] Stopping...");
    BaseObject::stop();
    if (_outQueue) {
        vQueueDelete(_outQueue);
        _outQueue = nullptr;
    }
    _setLed(false);
}

// ============================================================================
// ОТПРАВКА ДАННЫХ В PROTOCOL
// ============================================================================

void BluetoothManager::sendToProtocol(const char* json) {
    ProtocolInMessage msg;
    uint16_t len = strlen(json);
    if (len >= sizeof(msg.payload)) len = sizeof(msg.payload) - 1;
    
    msg.len = len;
    memcpy(msg.payload, json, len);
    msg.payload[len] = '\0';
    
    if (!DataRouter::publish(Topic::MSG_INCOMING, &msg)) {
        // DBG_PRINTLN("[BT] Error: Router rejected incoming msg");
    }
}

// ============================================================================
// ХУКИ БАЗОВОГО КЛАССА
// ============================================================================

void BluetoothManager::process() {
    bool current = _bt.hasClient();
    if (current != _isConnected) {
        _handleConnectionChange(current);
    }
    
    _updateLed();
    _processTx();
    _processRx();
}

void BluetoothManager::onCommand(Command cmd) {
    if (cmd == Command::CMD_OTA_START) {
        DBG_PRINTLN("[BT] OTA requested");
    } else {
        BaseObject::onCommand(cmd);
    }
}

// ============================================================================
// ОБРАБОТКА ДАННЫХ
// ============================================================================

void BluetoothManager::_processTx() {
    BtOutMessage msg;
    if (xQueueReceive(_outQueue, &msg, 0) == pdTRUE) {
        _bt.write((uint8_t*)msg.data, msg.len);
    }
}

void BluetoothManager::_processRx() {
    static char buf[256];
    static uint16_t pos = 0;
    
    while (_bt.available()) {
        char c = _bt.read();
        if (c == '\r') continue;
        
        if (c == '\n') {
            if (pos > 0) {
                buf[pos] = '\0';
                sendToProtocol(buf);
                pos = 0;
            }
        } else {
            if (pos < sizeof(buf) - 1) {
                buf[pos++] = c;
            } else {
                DBG_PRINTLN("[BT] RX buffer overflow");
                pos = 0;
            }
        }
    }
}

// ============================================================================
// УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ
// ============================================================================

void BluetoothManager::_handleConnectionChange(bool currentState) {
    _isConnected = currentState;
    
    if (_isConnected) {
        DBG_PRINTLN("[BT] Connected");
        _blinkSequenceActive = false;
        _blinkCount = 0;
        _setLed(true);
    } else {
        DBG_PRINTLN("[BT] Disconnected");
        _blinkSequenceActive = true;
        _blinkCount = 0;
        _lastBlinkTime = millis();
        _setLed(false);
    }
}

// ============================================================================
// УПРАВЛЕНИЕ LED
// ============================================================================

void BluetoothManager::_updateLed() {
    if (_isConnected) {
        return;  // При подключении LED горит постоянно
    }
    
    if (!_blinkSequenceActive) {
        _setLed(false);
        return;
    }
    
    uint32_t now = millis();
    if (now - _lastBlinkTime >= BT_LED_BLINK_INTERVAL) {
        _lastBlinkTime = now;
        
        if (_blinkCount < BT_LED_BLINK_COUNT) {
            _ledState = !_ledState;
            _setLed(_ledState);
            if (_ledState) {
                _blinkCount++;
            }
        } else {
            _blinkSequenceActive = false;
            _setLed(false);
        }
    }
}

void BluetoothManager::_setLed(bool state) {
    _ledState = state;
    digitalWrite(BT_LED_PIN, state ? HIGH : LOW);
}