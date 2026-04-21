/*
================================================================================
SYSTEM OBJECT BASE CLASS
================================================================================
Назначение:
-----------
Базовый класс для всех автономных модулей системы.
Обеспечивает единый стандарт управления жизненным циклом задач FreeRTOS.

Функционал:
-----------
1. Создание и удаление задачи (Task)
2. Контроль состояния (Running / Stopped)
3. Безопасная остановка (с ожиданием завершения)
4. Хранение контекста (TaskHandle, State)

Использование:
--------------
Наследуйтесь от этого класса для создания автономных модулей:
class BluetoothManager : public SystemObject { ... };
================================================================================
*/

#ifndef SYSTEM_OBJECT_H
#define SYSTEM_OBJECT_H

#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Состояния объекта
enum ObjectState {
    STATE_STOPPED = 0,
    STATE_RUNNING = 1,
    STATE_ERROR   = 2
};

class SystemObject {
public:
    SystemObject(const char* name, UBaseType_t priority = 5, uint32_t stackSize = 4096);
    virtual ~SystemObject();

    // Запуск объекта (создает задачу)
    bool start();
    
    // Остановка объекта (удаляет задачу)
    void stop();
    
    // Проверка состояния
    bool isRunning() const;
    ObjectState getState() const;

    // Получение имени (для отладки)
    const char* getName() const;

protected:
    // Виртуальный метод задачи (переопределяется в наследниках)
    virtual void taskLoop() = 0;
    
    // Статическая обертка для вызова метода класса из C-задачи
    static void taskWrapper(void* pvParameters);

    // Внутренние поля
    TaskHandle_t _taskHandle;
    const char* _name;
    ObjectState _state;
    UBaseType_t _priority;
    uint32_t _stackSize;
};

#endif // SYSTEM_OBJECT_H