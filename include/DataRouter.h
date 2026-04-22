/*
================================================================================
DATA_ROUTER.H — СТАТИЧЕСКИЙ МАРШРУТИЗАТОР СООБЩЕНИЙ
================================================================================
Реализует интерфейс IRouter для маршрутизации сообщений между задачами.
================================================================================
*/
#ifndef DATA_ROUTER_H
#define DATA_ROUTER_H

#include "IRouter.h"
#include "debug.h"

class DataRouter : public IRouter {
public:
    // Инициализация (сброс указателей)
    void init();
    // Регистрация очереди для конкретного топика (из IRouter)
    void registerQueue(Topic topic, QueueHandle_t queue) override;
    // Публикация данных в топик (из IRouter)
    bool publish(Topic topic, const void* data) override;

private:
    QueueHandle_t _incomingQueue = nullptr;
    QueueHandle_t _outgoingQueue = nullptr;
    QueueHandle_t _commandQueue  = nullptr;
};

#endif // DATA_ROUTER_H