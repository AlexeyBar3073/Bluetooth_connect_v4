#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "types.h"      // ПЕРВЫМ!
#include "BaseObject.h"
#include <BluetoothSerial.h>
#include "debug.h"

#define BT_LED_PIN             2
#define BT_DEFAULT_DEVICE_NAME "Car Monitor V3"
#define BT_LED_BLINK_INTERVAL  300
#define BT_LED_BLINK_COUNT     5
#define BT_OUT_QUEUE_DEPTH     1

class BluetoothManager : public BaseObject {
public:
    BluetoothManager();
    ~BluetoothManager();

    // init() — НЕ override (своя сигнатура)
    bool init(const char* deviceName = BT_DEFAULT_DEVICE_NAME);
    
    // stop() — override (совпадает с BaseObject)
    void stop() override;

    QueueHandle_t getOutQueue() const { return _outQueue; }
    void setRxTarget(QueueHandle_t queue) { _rxTarget = queue; }
    bool isConnected() const { return _isConnected; }

protected:
    void process() override;
    void onCommand(Command cmd) override;

private:
    BluetoothSerial _bt;
    QueueHandle_t _outQueue;   // ВЛАДЕЕМ
    QueueHandle_t _rxTarget;   // ССЫЛКА (только запись)

    bool _isConnected;
    uint32_t _lastBlinkTime;
    uint8_t _blinkCount;
    bool _ledState;
    bool _blinkSequenceActive;

    void _updateLed();
    void _handleConnectionChange(bool currentState);
    void _setLed(bool state);
    void _processTx();
    void _processRx();
};

#endif // BLUETOOTH_MANAGER_H