/*
================================================================================
BASEOBJECT.H — БАЗОВЫЙ КЛАСС ДЛЯ АВТОНОМНЫХ ЗАДАЧ
================================================================================
Файл: BaseObject.h
Назначение:
-----------
Абстрактный базовый класс для всех модулей системы.
Предоставляет:
• Единый цикл задачи (process() хук)
• Очередь команд управления
• Управление состоянием (старт/стоп)

ВАЖНО: types.h должен быть подключён ПЕРВЫМ!
================================================================================
*/

#ifndef BASE_OBJECT_H
#define BASE_OBJECT_H

// 1. ПЕРВЫМ — наши типы
#include "types.h"

// 2. Потом — системные заголовки
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

/// ============================================================================
/// КЛАСС: BaseObject
/// ============================================================================
class BaseObject {
public:
    // Конструктор / Деструктор
    BaseObject(const char* name, UBaseType_t priority = 5, uint32_t stackSize = 4096, BaseType_t core = 1);
    virtual ~BaseObject();

    // start() НЕ override — у него другая сигнатура (с параметрами)
    bool start();
    
    // stop() — override, т.к. совпадает с виртуальным методом базового класса
    virtual void stop();

    // Статус
    bool isRunning() const;
    ObjectState getState() const;
    const char* getName() const;

    // Очередь команд
    QueueHandle_t getCommandQueue() const { return _cmdQueue; }
    bool sendCommand(Command cmd);

protected:
    // Хуки для переопределения
    virtual void process() = 0;              // Бизнес-логика (обязателен)
    virtual void onCommand(Command cmd);     // Обработка команд (опционален)

    // Внутренний цикл задачи (не переопределять!)
    void taskLoop();

    // Защищённые поля
    TaskHandle_t _taskHandle;
    const char* _name;
    ObjectState _state;
    UBaseType_t _priority;
    uint32_t _stackSize;
    BaseType_t _core;
    QueueHandle_t _cmdQueue;

private:
    static void taskWrapper(void* pvParameters);
};

#endif // BASE_OBJECT_H