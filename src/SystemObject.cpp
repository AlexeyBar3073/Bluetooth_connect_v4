/*
================================================================================
SYSTEM OBJECT IMPLEMENTATION
================================================================================
*/

#include "SystemObject.h"

SystemObject::SystemObject(const char* name, UBaseType_t priority, uint32_t stackSize)
    : _taskHandle(nullptr)
    , _name(name)
    , _state(STATE_STOPPED)
    , _priority(priority)
    , _stackSize(stackSize)
{
}

SystemObject::~SystemObject() {
    // Гарантированная очистка при уничтожении объекта
    if (_state == STATE_RUNNING) {
        stop();
    }
}

bool SystemObject::start() {
    if (_state == STATE_RUNNING) {
        return true; // Уже запущен
    }

    // Создание задачи FreeRTOS
    BaseType_t result = xTaskCreate(
        taskWrapper,      // Функция задачи
        _name,            // Имя для отладчика
        _stackSize,       // Размер стека
        (void*)this,      // Параметр (указатель на объект)
        _priority,        // Приоритет
        &_taskHandle      // Хендл задачи
    );

    if (result == pdPASS) {
        _state = STATE_RUNNING;
        Serial.printf("[SYS] %s started\n", _name);
        return true;
    } else {
        _state = STATE_ERROR;
        Serial.printf("[SYS] %s start failed (err %d)\n", _name, result);
        return false;
    }
}

void SystemObject::stop() {
    if (_state == STATE_RUNNING && _taskHandle != nullptr) {
        Serial.printf("[SYS] %s stopping...\n", _name);
        
        // Удаление задачи
        vTaskDelete(_taskHandle);
        _taskHandle = nullptr;
        _state = STATE_STOPPED;
        
        Serial.printf("[SYS] %s stopped\n", _name);
    }
}

bool SystemObject::isRunning() const {
    return _state == STATE_RUNNING;
}

ObjectState SystemObject::getState() const {
    return _state;
}

const char* SystemObject::getName() const {
    return _name;
}

// Статический метод-обертка
// Позволяет вызвать метод класса из C-функции задачи FreeRTOS
void SystemObject::taskWrapper(void* pvParameters) {
    SystemObject* obj = (SystemObject*)pvParameters;
    if (obj) {
        obj->taskLoop(); // Вызов виртуального метода наследника
    }
    vTaskDelete(nullptr); // Самоудаление при выходе из loop
}