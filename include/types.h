/*
================================================================================
TYPES.H — ЕДИНЫЙ РЕЕСТР ТИПОВ СИСТЕМЫ
================================================================================
Файл: types.h
Назначение:
-----------
Централизованное определение ВСЕХ типов, перечислений и констант.
Этот файл ДОЛЖЕН подключаться ПЕРВЫМ в любом модуле, который использует
системные типы.
Порядок подключения (критично!):
--------------------------------
1. types.h
2. Arduino.h / FreeRTOS headers
3. Остальные заголовки
Автор: [Ваше имя/команда]
Версия: 3.0
Дата: 2026-04-22
================================================================================
*/
#ifndef SYSTEM_TYPES_H
#define SYSTEM_TYPES_H

#include <stdint.h>

// ============================================================================
// 1. СТАТУСЫ ОБЪЕКТОВ (используется в BaseObject)
// ============================================================================
enum class ObjectState : uint8_t {
    STATE_STOPPED = 0,
    STATE_RUNNING = 1,
    STATE_ERROR   = 2
};

enum class Topic : uint8_t {
    MSG_INCOMING,   // Входящие JSON строки (BT -> Protocol)
    MSG_OUTGOING,   // Исходящие JSON строки (Protocol -> BT)
    SYSTEM_CMD      // Системные команды (Command enum)
};

// ============================================================================
// 2. КОМАНДЫ И СТАТУСЫ ОТВЕТОВ
// ============================================================================
enum Command : uint8_t {
    CMD_NONE            = 0x00,  // Нет команды (пустышка, не используется)
    // --- Управление trip/топливом (обрабатывает Calculator Task) ---
    CMD_RESET_TRIP_A    = 0x01,  // Сброс trip_a и fuel_trip_a в 0
    CMD_RESET_TRIP_B    = 0x02,  // Сброс trip_b и fuel_trip_b в 0
    CMD_RESET_AVG       = 0x03,  // Сброс среднего расхода текущей поездки
    CMD_FULL_TANK       = 0x04,  // Заправка: fuel_base = tank_capacity
    CMD_CORRECT_ODO     = 0x05,  // Корректировка одометра (параметр: odo_value)
    // --- Настройки (обрабатывает Protocol Task) ---
    CMD_GET_CFG         = 0x06,  // Запрос всех настроек (ответ из кэша)
    CMD_SET_CFG         = 0x07,  // Установка настроек (параметры в set_cfg)
    // --- К-Line диагностика (обрабатывает K-Line Task) ---
    CMD_KL_GET_DTC      = 0x08,  // Чтение кодов ошибок ЭБУ
    CMD_KL_CLEAR_DTC    = 0x09,  // Сброс кодов ошибок ЭБУ
    CMD_KL_RESET_ADAPT  = 0x0A,  // Сброс адаптации АКПП (TCM)
    CMD_KL_PUMP_ATF     = 0x0B,  // Прокачка жидкости АКПП
    CMD_KL_DETECT_PROTO = 0x0C,   // Принудительное автоопределение протокола
    // --- Калибровка (обрабатывает RealEngine Task) ---
    CMD_CALIBRATE_SPEED     = 0x0D,  // Калибровка VSS (расстояние → pulses_per_meter)
    CMD_CALIBRATE_INJECTOR  = 0x0E,  // Калибровка форсунки (реальный расход → injector_flow)
    CMD_CALIBRATE_DEADTIME  = 0x0F,  // Калибровка dead time (расход на холостых → dead_time_us)
    // --- OTA (обрабатывают все задачи) ---
    CMD_OTA_START           = 0x11,  // Начало OTA: все задачи завершаются (освобождение памяти)
    CMD_OTA_END             = 0x10,  // Завершение OTA: Update.end() → ESP.restart()
};

struct CommandMap {
    const char* str;
    Command cmd;
};

static const CommandMap COMMAND_TABLE[] = {
    {"get_cfg",   Command::CMD_GET_CFG},
    {"set_cfg", Command::CMD_SET_CFG},
    {"reset_trip_a", Command::CMD_RESET_TRIP_A},
    {"reset_trip_b", Command::CMD_RESET_TRIP_B},
    {"reset_avg", Command::CMD_RESET_AVG},
    {"full_tank", Command::CMD_FULL_TANK},
    {"kl_get_dtc", Command::CMD_KL_GET_DTC},
    {"kl_clear_dtc", Command::CMD_KL_CLEAR_DTC},
    {"kl_reset_adapt", Command::CMD_KL_RESET_ADAPT},
    {"kl_pump_atf", Command::CMD_KL_PUMP_ATF},
    {"kl_detect_protocol", Command::CMD_KL_DETECT_PROTO},
    {"calibrate_speed_start", Command::CMD_CALIBRATE_SPEED},
    {"ota_update",   Command::CMD_OTA_END}
};

enum class ResponseStatus : uint8_t {
    STATUS_OK       = 0x00,
    STATUS_ERROR    = 0x01
};

// ============================================================================
// 3. КОНСТАНТЫ ОЧЕРЕДЕЙ (используется в BaseObject)
// ============================================================================
#define BASE_CMD_QUEUE_DEPTH     5
#define BASE_CMD_TYPE            uint8_t

// ============================================================================
// 4. СТРУКТУРЫ СООБЩЕНИЙ (общие для модулей)
// ============================================================================
/**
 * @brief Сообщение для исходящей очереди Bluetooth.
 * @details Владеет очередью BluetoothManager.
 */
struct BtOutMessage {
    uint16_t len;
    char     data[512];
};

/**
 * @brief Сообщение для входящей очереди протокола.
 * @details Владеет очередью ProtocolManager.
 */
struct ProtocolInMessage {
    uint16_t len;
    char     payload[128];
};

#endif // SYSTEM_TYPES_H