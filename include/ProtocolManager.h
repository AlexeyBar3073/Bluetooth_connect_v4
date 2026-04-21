#ifndef PROTOCOL_MANAGER_H
#define PROTOCOL_MANAGER_H

#include "types.h"      // ПЕРВЫМ!
#include "BaseObject.h"
#include <ArduinoJson.h>
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "debug.h"

#define PROTOCOL_QUEUE_DEPTH   1
#define PROTOCOL_TASK_STACK    4096
#define PROTOCOL_TASK_PRIORITY 5

typedef void (*ProtocolCommandCallback)(Command cmd, const char* payload);

class ProtocolManager : public BaseObject {
public:
    ProtocolManager();
    ~ProtocolManager();

    bool start() override;   // Совпадает с BaseObject::start()
    void stop() override;

    QueueHandle_t getInQueue() const { return _inQueue; }
    void setTxTarget(QueueHandle_t queue) { _btOutQueue = queue; }

    void setCommandCallback(ProtocolCommandCallback callback);
    bool sendResponse(Command cmd, ResponseStatus status, const char* data = nullptr);

protected:
    void process() override;
    void onCommand(Command cmd) override;

private:
    QueueHandle_t _inQueue;      // ВЛАДЕЕМ
    QueueHandle_t _btOutQueue;   // ССЫЛКА (только запись)
    SemaphoreHandle_t _jsonMutex;
    ProtocolCommandCallback _cmdCallback;
    uint32_t _processedCount;
    uint32_t _errorCount;

    void _processIncoming();
    void _pushToPeer(const char* jsonStr);
};

#endif // PROTOCOL_MANAGER_H