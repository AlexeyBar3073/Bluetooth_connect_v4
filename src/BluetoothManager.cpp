#include "BluetoothManager.h"

BluetoothManager::BluetoothManager() 
    : BaseObject("BT_Manager", 5, 4096, 0)
    , _outQueue(nullptr)
    , _rxTarget(nullptr)
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
    if (_outQueue) vQueueDelete(_outQueue);
    _setLed(false);
}

bool BluetoothManager::init(const char* deviceName) {
    if (!_bt.begin(deviceName)) {
        DBG_PRINTLN("[BT] HW init failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    _outQueue = xQueueCreate(BT_OUT_QUEUE_DEPTH, sizeof(BtOutMessage));
    if (!_outQueue) {
        DBG_PRINTLN("[BT] Queue creation failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
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

void BluetoothManager::process() {
    bool current = _bt.hasClient();
    if (current != _isConnected) _handleConnectionChange(current);
    _updateLed();
    _processTx();
    _processRx();
}

void BluetoothManager::onCommand(Command cmd) {
    if (cmd == Command::CMD_OTA_START) DBG_PRINTLN("[BT] OTA requested");
    else BaseObject::onCommand(cmd);
}

void BluetoothManager::_processTx() {
    BtOutMessage msg;
    if (xQueueReceive(_outQueue, &msg, 0) == pdTRUE) {
        _bt.write((uint8_t*)msg.data, msg.len);
    }
}

void BluetoothManager::_processRx() {
    static char buf[128];
    static uint16_t pos = 0;
    while (_bt.available()) {
        char c = _bt.read();
        if (c == '\r') continue;
        if (c == '\n') {
            if (_rxTarget && pos > 0) {
                ProtocolInMessage pMsg;
                uint16_t len = (pos < sizeof(pMsg.payload)-1) ? pos : (sizeof(pMsg.payload)-1);
                pMsg.len = len;
                memcpy(pMsg.payload, buf, len);
                pMsg.payload[len] = '\0';
                xQueueOverwrite(_rxTarget, &pMsg);
            }
            pos = 0;
        } else {
            if (pos < sizeof(buf)-1) buf[pos++] = c;
            else pos = 0;
        }
    }
}

void BluetoothManager::_handleConnectionChange(bool currentState) {
    _isConnected = currentState;
    if (_isConnected) {
        DBG_PRINTLN("[BT] Connected");
        _blinkSequenceActive = false; _blinkCount = 0; _setLed(true);
    } else {
        DBG_PRINTLN("[BT] Disconnected");
        _blinkSequenceActive = true; _blinkCount = 0; _lastBlinkTime = millis(); _setLed(false);
    }
}

void BluetoothManager::_updateLed() {
    if (_isConnected) return;
    if (!_blinkSequenceActive) { _setLed(false); return; }
    uint32_t now = millis();
    if (now - _lastBlinkTime >= BT_LED_BLINK_INTERVAL) {
        _lastBlinkTime = now;
        if (_blinkCount < BT_LED_BLINK_COUNT) {
            _ledState = !_ledState; _setLed(_ledState);
            if (_ledState) _blinkCount++;
        } else { _blinkSequenceActive = false; _setLed(false); }
    }
}

void BluetoothManager::_setLed(bool state) {
    _ledState = state;
    digitalWrite(BT_LED_PIN, state ? HIGH : LOW);
}