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
: _name(name)
, _priority(priority)
, _stackSize(stackSize)
, _core(core)
{
    _cmdQueue = xQueueCreate(BASE_CMD_QUEUE_DEPTH, sizeof(BASE_CMD_TYPE));
    if (!_cmdQueue) {
        DBG_PRINTF("[%s] ERROR: Failed to create command queue", _name);
        _state = ObjectState::STATE_ERROR;
        return;
    }

    // Задача создаётся в конструкторе, но не выполняет бизнес-логику до init()
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
        DBG_PRINTF("[%s] ERROR: Task creation failed", _name);
        _state = ObjectState::STATE_ERROR;
    } else {
        DBG_PRINTF("[%s] Created (Core %d, waiting for init)", _name, _core);
    }
}

BaseObject::~BaseObject() {
    destroy();
}

// ============================================================================
// УПРАВЛЕНИЕ ЖИЗНЕННЫМ ЦИКЛОМ
// ============================================================================
bool BaseObject::init(IRouter& router) {
    if (_state == ObjectState::STATE_ERROR || _initialized) return false;

    _router = &router;
    // 1. Автоматическая подписка на системные команды
    _router->registerQueue(Topic::SYSTEM_CMD, _cmdQueue);
    // 2. Вызов хука наследника для уникальных подписок
    onSubscribe();

    _initialized = true;
    _state = ObjectState::STATE_RUNNING;
    DBG_PRINTF("[%s] Initialized & Ready", _name);
    return true;
}

void BaseObject::destroy() {
    if (_initialized) {
        _initialized = false;
        _state = ObjectState::STATE_STOPPED;
    }
    if (_taskHandle) {
        DBG_PRINTF("[%s] Deleting task...", _name);
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
    }
    if (_cmdQueue) {
        vQueueDelete(_cmdQueue);
        _cmdQueue = nullptr;
    }
    _router = nullptr;
}

bool BaseObject::isReady() const { return _initialized; }
ObjectState BaseObject::getState() const { return _state; }
const char* BaseObject::getName() const { return _name; }

void BaseObject::onCommand(Command cmd) { (void)cmd; }
void BaseObject::onSubscribe() {} // По умолчанию ничего не подписываем

// ============================================================================
// ЦИКЛ ЗАДАЧИ
// ============================================================================
void BaseObject::taskLoop() {
    while (true) {
        // Задача ожидает вызова init() перед выполнением логики
        if (!_initialized) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (_state != ObjectState::STATE_RUNNING) break;

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
    vTaskDelete(nullptr);
}

// ============================================================================
// СТАТИЧЕСКАЯ ОБЁРТКА ДЛЯ ЗАДАЧИ FREERTOS
// ============================================================================
void BaseObject::taskWrapper(void* pvParameters) {
    BaseObject* obj = static_cast<BaseObject*>(pvParameters);
    if (obj) {
        obj->taskLoop();
    }
}