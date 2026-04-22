/*
================================================================================
MAIN.CPP — ТОЧКА ВХОДА
================================================================================

НАЗНАЧЕНИЕ:
-----------
Создаёт глобальные объекты, запускает их, связывает через прямые вызовы.
Вся бизнес-логика вынесена в модули.

================================================================================
*/

#include <Arduino.h>
#include "types.h"
#include "debug.h"
#include "DataRouter.h"
#include "BluetoothManager.h"
#include "ProtocolManager.h"

// ============================================================================
// ГЛОБАЛЬНЫЕ ОБЪЕКТЫ
// ============================================================================

BluetoothManager bt;
ProtocolManager protocol;

// ============================================================================
// SETUP
// ============================================================================

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    DBG_NEWLINE();
    DBG_PRINTLN("=== Car Monitor V3 Starting ===");
    DBG_PRINTF("Free Heap: %u bytes", ESP.getFreeHeap());
    DBG_NEWLINE();

    // Инициализация Bluetooth
    if (!bt.init("Car Monitor V3")) {
        DBG_PRINTLN("[MAIN] CRITICAL: BT init failed");
        while (true) { delay(1000); }
    }
    
    // Запуск Protocol
    if (!protocol.start()) {
        DBG_PRINTLN("[MAIN] CRITICAL: Protocol start failed");
        while (true) { delay(1000); }
    }
    
    // 3. Регистрация очередей в роутере
    DataRouter::registerQueue(Topic::MSG_INCOMING, protocol.getInQueue());
    DataRouter::registerQueue(Topic::MSG_OUTGOING, bt.getOutQueue());
    // Очередь системных команд (например, очередь самого протокола или отдельная)
    DataRouter::registerQueue(Topic::SYSTEM_CMD, protocol.getCommandQueue());

    DBG_NEWLINE();
    DBG_PRINTLN("=== System READY ===");
    DBG_PRINTF("Free Heap: %u bytes", ESP.getFreeHeap());
    DBG_NEWLINE();
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {
#if DEBUG_LOG
    static uint32_t lastMemLog = 0;
    if (millis() - lastMemLog > 10000) {
        lastMemLog = millis();
        DBG_PRINTF("[MEM] Free: %u", ESP.getFreeHeap());
    }
#endif
    delay(100);
}