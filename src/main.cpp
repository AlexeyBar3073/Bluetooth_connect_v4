#include <Arduino.h>
#include "types.h"          // ПЕРВЫМ!
#include "debug.h"
#include "BluetoothManager.h"
#include "ProtocolManager.h"

BluetoothManager bt;
ProtocolManager protocol;

void onCommandReceived(Command cmd, const char* payload) {
    DBG_PRINTF("[APP] Cmd: 0x%02X", static_cast<uint8_t>(cmd));
    switch (cmd) {
        case Command::CMD_NONE:
            protocol.sendResponse(cmd, ResponseStatus::STATUS_ERROR, "invalid"); break;
        case Command::CMD_GET_CFG:
            DBG_PRINTLN("[APP] Config requested");
            protocol.sendResponse(cmd, ResponseStatus::STATUS_OK, "{\"tank\":50}"); break;
        default:
            protocol.sendResponse(cmd, ResponseStatus::STATUS_OK, "ok"); break;
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    DBG_PRINTLN("\n[SYSTEM] Boot V3 Minimal");

    if (!bt.init("Car Monitor V3") || !protocol.start()) {
        DBG_PRINTLN("[SYSTEM] CRITICAL: Start failed");
        while(true) { delay(1000); }
    }

    // Связка очередей
    bt.setRxTarget(protocol.getInQueue());
    protocol.setTxTarget(bt.getOutQueue());
    protocol.setCommandCallback(onCommandReceived);
    
    DBG_PRINTLN("[SYSTEM] READY");
}

void loop() {
    #if DEBUG_LOG
    static uint32_t last = 0;
    if (millis() - last > 10000) { last = millis(); DBG_PRINTF("[MEM] Free: %u\n", ESP.getFreeHeap()); }
    #endif
    delay(100);
}