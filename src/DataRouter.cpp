#include "DataRouter.h"

void DataRouter::init() {
    _incomingQueue = nullptr;
    _outgoingQueue = nullptr;
    _commandQueue  = nullptr;
}

void DataRouter::registerQueue(Topic topic, QueueHandle_t queue) {
    switch (topic) {
        case Topic::MSG_INCOMING: _incomingQueue = queue; break;
        case Topic::MSG_OUTGOING: _outgoingQueue = queue; break;
        case Topic::SYSTEM_CMD:   _commandQueue  = queue; break;
    }
}

bool DataRouter::publish(Topic topic, const void* data) {
    QueueHandle_t target = nullptr;
    size_t size = 0;
    switch (topic) {
        case Topic::MSG_INCOMING:
            target = _incomingQueue;
            size = sizeof(ProtocolInMessage);
            break;
        case Topic::MSG_OUTGOING:
            target = _outgoingQueue;
            size = sizeof(BtOutMessage);
            break;
        case Topic::SYSTEM_CMD:
            target = _commandQueue;
            size = sizeof(Command);
            break;
    }
    if (target && data) {
        // Используем xQueueSend. Для команд и сообщений обычно хватает 0 ожидания.
        return xQueueSend(target, data, 0) == pdTRUE;
    }
    return false;
}