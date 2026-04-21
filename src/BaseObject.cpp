/*
================================================================================
BASEOBJECT.CPP — РЕАЛИЗАЦИЯ БАЗОВОГО КЛАССА
================================================================================
*/

#include "BaseObject.h"

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
        _state = ObjectState::STATE_ERROR;
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
    if (_state == ObjectState::STATE_ERROR) return false;

    BaseType_t res = xTaskCreatePinnedToCore(
        taskWrapper,
        _name,
        _stackSize,
        this,
        _priority,
        &_taskHandle,
        _core
    );

    if (res == pdPASS) {
        _state = ObjectState::STATE_RUNNING;
        return true;
    }
    _state = ObjectState::STATE_ERROR;
    return false;
}

void BaseObject::stop() {
    if (_state == ObjectState::STATE_RUNNING && _taskHandle) {
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        _state = ObjectState::STATE_STOPPED;
    }
}

bool BaseObject::isRunning() const { return _state == ObjectState::STATE_RUNNING; }
ObjectState BaseObject::getState() const { return _state; }
const char* BaseObject::getName() const { return _name; }

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
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}

void BaseObject::onCommand(Command cmd) {
    // По умолчанию: игнор
}

void BaseObject::taskWrapper(void* pvParameters) {
    BaseObject* obj = static_cast<BaseObject*>(pvParameters);
    if (obj) {
        obj->taskLoop();
    }
    vTaskDelete(nullptr);
}