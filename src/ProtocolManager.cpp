#include "ProtocolManager.h"

ProtocolManager::ProtocolManager()
    : BaseObject("Protocol", PROTOCOL_TASK_PRIORITY, PROTOCOL_TASK_STACK)
    , _inQueue(nullptr)
    , _btOutQueue(nullptr)
    , _jsonMutex(nullptr)
    , _cmdCallback(nullptr)
    , _processedCount(0)
    , _errorCount(0)
{}

ProtocolManager::~ProtocolManager() {
    if (_inQueue) vQueueDelete(_inQueue);
    if (_jsonMutex) vSemaphoreDelete(_jsonMutex);
}

bool ProtocolManager::start() {
    _inQueue = xQueueCreate(PROTOCOL_QUEUE_DEPTH, sizeof(ProtocolInMessage));
    _jsonMutex = xSemaphoreCreateMutex();
    if (!_inQueue || !_jsonMutex) {
        DBG_PRINTLN("[PROTOCOL] Resource creation failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    return BaseObject::start();
}

void ProtocolManager::stop() {
    DBG_PRINTLN("[PROTOCOL] Stopping...");
    BaseObject::stop();
    if (_inQueue) { vQueueDelete(_inQueue); _inQueue = nullptr; }
    if (_jsonMutex) { vSemaphoreDelete(_jsonMutex); _jsonMutex = nullptr; }
}

void ProtocolManager::process() {
    _processIncoming();
}

void ProtocolManager::onCommand(Command cmd) {
    if (cmd == Command::CMD_OTA_START) DBG_PRINTLN("[PROTOCOL] OTA requested");
    else BaseObject::onCommand(cmd);
}

bool ProtocolManager::sendResponse(Command cmd, ResponseStatus status, const char* data) {
    if (!_btOutQueue) return false;
    JsonDocument doc;
    doc["cmd"] = static_cast<uint8_t>(cmd);
    doc["status"] = static_cast<uint8_t>(status);
    if (data) doc["data"] = data;
    char buf[256];
    size_t len = serializeJson(doc, buf);
    if (len > 0 && len < sizeof(buf)) {
        _pushToPeer(buf);
        return true;
    }
    _errorCount++;
    return false;
}

void ProtocolManager::setCommandCallback(ProtocolCommandCallback callback) {
    _cmdCallback = callback;
}

void ProtocolManager::_processIncoming() {
    ProtocolInMessage msg;
    if (xQueueReceive(_inQueue, &msg, 0) != pdTRUE) return;
    if (xSemaphoreTake(_jsonMutex, 10/portTICK_PERIOD_MS) != pdTRUE) return;
    JsonDocument doc;
    if (!deserializeJson(doc, msg.payload) && doc["cmd"].is<uint8_t>()) {
        Command cmd = static_cast<Command>(doc["cmd"].as<uint8_t>());
        DBG_PRINTF("[PROTOCOL] CMD: 0x%02X", static_cast<uint8_t>(cmd));
        if (_cmdCallback) _cmdCallback(cmd, msg.payload);
        _processedCount++;
    } else {
        _errorCount++;
    }
    xSemaphoreGive(_jsonMutex);
}

void ProtocolManager::_pushToPeer(const char* jsonStr) {
    BtOutMessage outMsg;
    size_t len = strlen(jsonStr);
    if (len >= sizeof(outMsg.data)) return;
    outMsg.len = len;
    memcpy(outMsg.data, jsonStr, len + 1);
    xQueueOverwrite(_btOutQueue, &outMsg);
}