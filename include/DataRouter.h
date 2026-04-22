#ifndef DATA_ROUTER_H
#define DATA_ROUTER_H

#include "types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "debug.h"

class DataRouter {
public:
    // Инициализация (сброс указателей)
    static void init();

    // Регистрация очереди для конкретного топика
    static void registerQueue(Topic topic, QueueHandle_t queue);

    // Публикация данных в топик
    static bool publish(Topic topic, const void* data);

private:
    static QueueHandle_t _incomingQueue;
    static QueueHandle_t _outgoingQueue;
    static QueueHandle_t _commandQueue;
};

#endif