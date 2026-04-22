/*
================================================================================
BASEOBJECT.CPP — РЕАЛИЗАЦИЯ БАЗОВОГО КЛАССА
================================================================================
*/

#include "BaseObject.h"
#include "debug.h"

// ============================================================================
// КОНСТРУКТОР / ДЕСТРУКТОР
// ============================================================================

BaseObject::BaseObject(const char* name, UBaseType_t priority, uint32_t stackSize, BaseType_t core)
    : _taskHandle(nullptr)
    , _name(name)
    , _state(ObjectState::STATE_STOPPED)
    , _priority(priority)
    , _stackSize(stackSize)
    , _core(core)
    , _cmdQueue(nullptr)
{
    _cmdQueue = xQueueCreate(BASE_CMD_QUEUE_DEPTH, sizeof(BASE_CMD_TYPE));
    if (!_cmdQueue) {
        DBG_PRINTF("[%s] ERROR: Failed to create command queue", _name);
        _state = ObjectState::STATE_ERROR;
    } else {
        DBG_PRINTF("[%s] Created (queue depth=%d)", _name, BASE_CMD_QUEUE_DEPTH);
    }
}

BaseObject::~BaseObject() {
    if (_state == ObjectState::STATE_RUNNING) {
        stop();
    }
    if (_cmdQueue) {
        vQueueDelete(_cmdQueue);
        _cmdQueue = nullptr;
    }
}

// ============================================================================
// УПРАВЛЕНИЕ ЖИЗНЕННЫМ ЦИКЛОМ
// ============================================================================

bool BaseObject::start() {
    if (_state == ObjectState::STATE_RUNNING) return true;

    // Сначала ставим флаг, что мы должны бежать
    _state = ObjectState::STATE_RUNNING; 

    BaseType_t res = xTaskCreatePinnedToCore(
        taskWrapper,
        _name,
        _stackSize,
        this,
        _priority,
        &_taskHandle,
        _core
    );

    if (res != pdPASS) {
        _state = ObjectState::STATE_ERROR; // Если не взлетело — сбрасываем в ошибку
        DBG_PRINTF("[%s] ERROR: Task creation failed", _name);
        return false;
    }
    
    DBG_PRINTF("[%s] Started (Core %d)", _name, _core);
    return true;
}

void BaseObject::stop() {
    if (_state == ObjectState::STATE_RUNNING && _taskHandle) {
        DBG_PRINTF("[%s] Stopping...", _name);
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        _state = ObjectState::STATE_STOPPED;
        DBG_PRINTF("[%s] Stopped", _name);
    }
}

// ============================================================================
// ИНФОРМАЦИЯ О СОСТОЯНИИ
// ============================================================================

bool BaseObject::isRunning() const { 
    return _state == ObjectState::STATE_RUNNING; 
}

ObjectState BaseObject::getState() const { 
    return _state; 
}

const char* BaseObject::getName() const { 
    return _name; 
}

// ============================================================================
// ОЧЕРЕДЬ КОМАНД
// ============================================================================

bool BaseObject::sendCommand(Command cmd) {
    if (!_cmdQueue) return false;
    BASE_CMD_TYPE val = static_cast<BASE_CMD_TYPE>(cmd);
    return xQueueSend(_cmdQueue, &val, 0) == pdTRUE;
}

// ============================================================================
// ЦИКЛ ЗАДАЧИ
// ============================================================================

void BaseObject::taskLoop() {
    while (_state == ObjectState::STATE_RUNNING) {
        // 1. Проверка очереди команд
        BASE_CMD_TYPE cmdVal;
        if (xQueueReceive(_cmdQueue, &cmdVal, 0) == pdTRUE) {
            onCommand(static_cast<Command>(cmdVal));
        }
        
        // 2. Бизнес-логика наследника
        process();
        
        // 3. Передача управления планировщику
        vTaskDelay(1);
    }
}

// ============================================================================
// ХУКИ ДЛЯ ПЕРЕОПРЕДЕЛЕНИЯ (реализация по умолчанию)
// ============================================================================

void BaseObject::onCommand(Command cmd) {
    // По умолчанию — игнорируем команду
    (void)cmd;
}

// ============================================================================
// СТАТИЧЕСКАЯ ОБЁРТКА ДЛЯ ЗАДАЧИ FREERTOS
// ============================================================================

void BaseObject::taskWrapper(void* pvParameters) {
    BaseObject* obj = static_cast<BaseObject*>(pvParameters);
    if (obj) {
        obj->taskLoop();
    }
    vTaskDelete(nullptr);
}