/*
================================================================================
PROTOCOLMANAGER.CPP — РЕАЛИЗАЦИЯ МЕНЕДЖЕРА ПРОТОКОЛА
================================================================================
*/

#include "ProtocolManager.h"
#include "DataRouter.h"


// ============================================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================================

ProtocolManager::ProtocolManager(const char* name, UBaseType_t priority, 
                                 uint32_t stackSize, BaseType_t core)
    : BaseObject(name, priority, stackSize, core)
    , _inQueue(nullptr)
    , _jsonMutex(nullptr)
    , _cmdCallback(nullptr)
    , _processedCount(0)
    , _errorCount(0)
{}

ProtocolManager::~ProtocolManager() {
    if (_inQueue) {
        vQueueDelete(_inQueue);
        _inQueue = nullptr;
    }
    if (_jsonMutex) {
        vSemaphoreDelete(_jsonMutex);
        _jsonMutex = nullptr;
    }
}

// ============================================================================
// УПРАВЛЕНИЕ ЖИЗНЕННЫМ ЦИКЛОМ
// ============================================================================

bool ProtocolManager::start() {
    DBG_PRINTLN("[PROTOCOL] Starting...");
    
    _inQueue = xQueueCreate(PROTOCOL_QUEUE_DEPTH, sizeof(ProtocolInMessage));
    if (!_inQueue) {
        DBG_PRINTLN("[PROTOCOL] ERROR: Input queue creation failed");
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    DBG_PRINTF("[PROTOCOL] Input queue created (depth=%d, msg_size=%u)", 
               PROTOCOL_QUEUE_DEPTH, (unsigned)sizeof(ProtocolInMessage));
    
    _jsonMutex = xSemaphoreCreateMutex();
    if (!_jsonMutex) {
        DBG_PRINTLN("[PROTOCOL] ERROR: Mutex creation failed");
        vQueueDelete(_inQueue);
        _inQueue = nullptr;
        _state = ObjectState::STATE_ERROR;
        return false;
    }
    DBG_PRINTLN("[PROTOCOL] JSON mutex created");
    
    return BaseObject::start();
}

void ProtocolManager::stop() {
    DBG_PRINTLN("[PROTOCOL] Stopping...");
    BaseObject::stop();
    if (_inQueue) {
        vQueueDelete(_inQueue);
        _inQueue = nullptr;
    }
    if (_jsonMutex) {
        vSemaphoreDelete(_jsonMutex);
        _jsonMutex = nullptr;
    }
}

// ============================================================================
// ОТПРАВКА ОТВЕТОВ
// ============================================================================

void ProtocolManager::sendResponse(Command cmd, ResponseStatus status, const char* data) {
    JsonDocument doc;
    doc["cmd"] = static_cast<uint8_t>(cmd);
    doc["status"] = static_cast<uint8_t>(status);
    if (data) {
        doc["data"] = data;
    }
    
    char buf[512];
    size_t len = serializeJson(doc, buf, sizeof(buf));
    if (len > 0 && len < sizeof(buf)) {
        buf[len] = '\0';
        _sendToBluetooth(buf);
    } else {
        _errorCount++;
        DBG_PRINTLN("[PROTOCOL] ERROR: Response too large");
    }
}

// ============================================================================
// ХУКИ БАЗОВОГО КЛАССА
// ============================================================================

void ProtocolManager::process() {
    //--- Обработка очереди входящих сообщений
    _readIncomingQueue(); 
}

//--- Обработчик команд
void ProtocolManager::onCommand(Command cmd) {
   
}

// ============================================================================
// ОБРАБОТЧИК СООБЩЕНИЙ
// ============================================================================

void ProtocolManager::onMessage(const char *msg)
{
    //--- Идентификатор входящего сообщения
    static int msgId = 0;

    //--- Идентификатор команды
    Command cmd = Command::CMD_NONE;

    //--- Десериализуем входящее сообщение
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, msg);
    if (error) {
        DBG_PRINTF("[PROTOCOL] ERROR: Failed to parse JSON: %s", error.c_str());
        return;
    }

    //--- Получаем идентификатор входящего сообщения
    msgId = (int)doc["msg_id"];

    //--- Готовоим объект для ответа
    JsonDocument resp;
    //--- Если идентификатор входящей команды не равен 0, тогда отвечаем
    if (msgId != 0)
    {
        resp["ack_id"] = msgId;
        msgId = 0;
    }

    //--- Получаем идентификатор команды
    const char *cmdStr = doc["command"];
    cmd = (Command)_stringToCommand(cmdStr);
    if (cmd != Command::CMD_NONE)
    {
        onCommand(cmd); // Вызываем обработчик
    }
}

// ============================================================================
// ВНУТРЕННИЕ МЕТОДЫ
// ============================================================================

void ProtocolManager::_readIncomingQueue() {
    ProtocolInMessage msg;
    
    // Неблокирующее чтение
    if (xQueueReceive(_inQueue, &msg, 0) != pdTRUE) {
        return;
    }
    
    // Захватываем мьютекс только на время копирования
    if (xSemaphoreTake(_jsonMutex, 10 / portTICK_PERIOD_MS) != pdTRUE) {
        return;
    }
    
    char localBuffer[sizeof(msg.payload)];
    strncpy(localBuffer, msg.payload, sizeof(localBuffer) - 1);
    localBuffer[sizeof(localBuffer) - 1] = '\0';
    
    // Освобождаем мьютекс — очередь свободна
    xSemaphoreGive(_jsonMutex);
    
    // Вызываем обработчик
    onMessage(localBuffer);
}

void ProtocolManager::_sendToBluetooth(const char* json) {
    BtOutMessage msg;
    uint16_t len = strlen(json);
    if (len >= sizeof(msg.data)) len = sizeof(msg.data) - 1;
    
    msg.len = len;
    memcpy(msg.data, json, len);
    msg.data[len] = '\0';
    
    DataRouter::publish(Topic::MSG_OUTGOING, &msg);
}

Command ProtocolManager::_stringToCommand(const char* str) {
    if (!str) return Command::CMD_NONE;
    
    for (const auto& item : COMMAND_TABLE) {
        if (strcmp(str, item.str) == 0) {
            return item.cmd;
        }
    }
    return Command::CMD_NONE;
}