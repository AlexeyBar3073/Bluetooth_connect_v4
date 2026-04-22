/*
================================================================================
MAIN.CPP — ТОЧКА ВХОДА
================================================================================
НАЗНАЧЕНИЕ:
-----------
Создаёт глобальные объекты, запускает их, связывает через шину данных.
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
DataRouter router;
BluetoothManager bt;
ProtocolManager protocol;

// ============================================================================
// SETUP
// ============================================================================
void setup() {
    Serial.begin(115200);
    delay(1000);

    DBG_NEWLINE();
    DBG_PRINTLN("=== Car Monitor V4 Starting ===");
    DBG_PRINTF("Free Heap: %u bytes", ESP.getFreeHeap());
    DBG_NEWLINE();

    // Инициализация Bluetooth через шину данных
    if (!bt.init(router, "Car Monitor V3")) {
        DBG_PRINTLN("[MAIN] CRITICAL: BT init failed");
        while (true) { delay(1000); }
    }
    delay(100);

    // Инициализация Protocol через шину данных
    if (!protocol.init(router)) {
        DBG_PRINTLN("[MAIN] CRITICAL: Protocol init failed");
        while (true) { delay(1000); }
    }
    delay(1000);

    // Ручная регистрация очередей БОЛЬШЕ НЕ ТРЕБУЕТСЯ.
    // Подписки выполняются автоматически внутри init() и хуков onSubscribe().

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