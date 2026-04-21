/*
================================================================================
BLUETOOTH MANAGER OBJECT (V3 - Autonomous)
================================================================================
Назначение:
-----------
Автономный объект управления Bluetooth Serial и LED-индикацией.
Наследуется от SystemObject, работает в собственной задаче FreeRTOS.

Функционал:
-----------
1. Автономный прием/передача данных (не требует вызовов из loop)
2. LED-индикация состояния:
   - Горит: Подключение активно
   - Мигает 5 раз: Подключение разорвано
   - Погас: После 5 миганий (ожидание переподключения)
3. Потокобезопасный доступ к данным (Mutex)
4. Автоматическая очистка ресурсов при остановке

Архитектура:
------------
[ SystemObject ]
       ^
       |
[ BluetoothManager ] -> [ BluetoothSerial ]
                      -> [ LED Driver ]
================================================================================
*/

#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include "SystemObject.h"
#include <BluetoothSerial.h>
#include "freertos/semphr.h"

// Конфигурация
#define BT_LED_PIN           2
#define BT_DEVICE_NAME       "Car Monitor"
#define BLINK_INTERVAL_MS    300
#define BLINK_COUNT_TARGET   5

class BluetoothManager : public SystemObject {
public:
    BluetoothManager();
    ~BluetoothManager();

    // Переопределение метода запуска (доп. инициализация)
    bool start(const char* deviceName = BT_DEVICE_NAME);

    // Методы для обмена данными (вызываются из других задач)
    int available();
    int read();
    size_t write(const uint8_t* buffer, size_t size);
    
    // Статус соединения
    bool isConnected();

protected:
    // Основной цикл задачи (обязательный метод SystemObject)
    void taskLoop() override;

private:
    BluetoothSerial _bt;
    SemaphoreHandle_t _mutex;
    
    // Состояние соединения
    bool _isConnected;
    
    // Логика LED
    uint32_t _lastBlinkTime;
    uint8_t _blinkCount;
    bool _ledState;
    bool _blinkSequenceActive;

    // Внутренняя логика
    void _updateLed();
    void _handleConnectionChange(bool currentState);
    void _setLed(bool state);
};

#endif // BLUETOOTH_MANAGER_H