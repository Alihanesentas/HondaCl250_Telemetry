#include <Arduino.h>
#include <esp_timer.h>
#include <esp_task_wdt.h>
#include <esp_system.h>

#include "SystemState.h"
#include "IModule.h"
#ifdef MOCK_CAN_DATA
#include "MockCANModule.h"
#else
#include "HondaCANModule.h"
#endif
#include "IMUModule.h"
#include "NextionModule.h"
#include "BLEServerModule.h"
#include "WiFiServerModule.h"
#include "SerialLoggerModule.h"

// ============================================================================
// HARDWARE PIN DEFINITIONS (ESP32-S3 DevKit C1)
// ============================================================================
#define CAN_TX_PIN      GPIO_NUM_4
#define CAN_RX_PIN      GPIO_NUM_5

#define I2C_SDA_PIN     GPIO_NUM_1
#define I2C_SCL_PIN     GPIO_NUM_2

#define UART2_TX_PIN    GPIO_NUM_17
#define UART2_RX_PIN    GPIO_NUM_18

// Free GPIO used purely for loop-timing observation (oscilloscope / logic analyzer probe).
// Not wired to any peripheral above (4,5,1,2,17,18 are taken).
#define DEBUG_LOOP_PIN  GPIO_NUM_8

// G1.4 -- Task Watchdog Timer. If loop() ever stalls (a module hangs) for longer than
// this, the TWDT panics and resets the board rather than leaving a dead unit riding.
#define TWDT_TIMEOUT_S  5

// Hardware Serial 2 Instance for Nextion HMI Display
HardwareSerial NextionSerial(2);

// ============================================================================
// CENTRAL TELEMETRY STATE STORE
// ============================================================================
SystemState globalState;

// ============================================================================
// SYSTEM MODULE INSTANTIATIONS
// ============================================================================
// G3.2-alt -- MOCK_CAN_DATA build flag (see platformio.ini env:esp32-s3-devkitc-1-mock)
// swaps in MockCANModule (synthetic telemetry, no ECU/CAN hardware) so the rest of the
// system -- Nextion, BLE, WiFi, staleness, watchdog -- can be exercised on real ESP32
// hardware without a Honda ECU. HondaCANModule.cpp/.h are never touched by this.
#ifdef MOCK_CAN_DATA
MockCANModule      canModule;
#else
HondaCANModule     canModule(CAN_TX_PIN, CAN_RX_PIN);
#endif
IMUModule          imuModule(I2C_SDA_PIN, I2C_SCL_PIN);
NextionModule      displayModule(NextionSerial, UART2_RX_PIN, UART2_TX_PIN);
BLEServerModule    bleModule;
WiFiServerModule   wifiModule(80);      // SoftAP HTTP JSON Backend Server on port 80
SerialLoggerModule loggerModule(1000); // Prints serial log every 1000ms

// Polymorphic module array for clean asynchronous execution
IModule* modules[] = {
    &canModule,
    &imuModule,
    &displayModule,
    &bleModule,
    &wifiModule,
    &loggerModule
};

const uint8_t MODULE_COUNT = sizeof(modules) / sizeof(modules[0]);
const char* MODULE_NAMES[MODULE_COUNT] = {"CAN", "IMU", "Nextion", "BLE", "WiFi", "Logger"};

// ============================================================================
// G0.1 -- LOOP TIMING INSTRUMENTATION
// Tracks per-module update() duration (esp_timer_get_time, microsecond resolution)
// and prints min/avg/max to Serial every 10s. DEBUG_LOOP_PIN pulses HIGH for the
// duration of one full loop() pass so it can be probed with a scope/logic analyzer.
// ============================================================================
struct TimingStats {
    uint32_t minUs = UINT32_MAX;
    uint32_t maxUs = 0;
    uint64_t sumUs = 0;
    uint32_t samples = 0;

    void record(uint32_t us) {
        if (us < minUs) minUs = us;
        if (us > maxUs) maxUs = us;
        sumUs += us;
        samples++;
    }

    void reset() { *this = TimingStats(); }
};

TimingStats moduleTiming[MODULE_COUNT];
TimingStats loopTiming;
unsigned long lastTimingReport = 0;
const unsigned long TIMING_REPORT_INTERVAL_MS = 10000;

// ============================================================================
// G0.2 -- MODULE HEALTH TRACKING
// A module whose begin() fails (or that later reports unhealthy) is excluded
// from update() so one broken peripheral (e.g. IMU not wired) cannot stall
// or crash the modules that are working.
// ============================================================================
bool moduleActive[MODULE_COUNT];

// ============================================================================
// G1.4 -- helper to name a reset reason for the boot log
// ============================================================================
const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_EXT:       return "EXTERNAL_PIN";
        case ESP_RST_SW:        return "SOFTWARE";
        case ESP_RST_PANIC:     return "PANIC (exception)";
        case ESP_RST_INT_WDT:   return "INTERRUPT_WATCHDOG";
        case ESP_RST_TASK_WDT:  return "TASK_WATCHDOG (loop stalled)";
        case ESP_RST_WDT:       return "OTHER_WATCHDOG";
        case ESP_RST_BROWNOUT:  return "BROWNOUT (power dip)";
        case ESP_RST_SDIO:      return "SDIO";
        default:                return "UNKNOWN";
    }
}

// ============================================================================
// SETUP & MAIN LOOP
// ============================================================================
void setup() {
    // Initialize USB Serial Debug Console
    Serial.begin(115200);
    delay(500);

    Serial.println("\n==================================================");
    Serial.println("   HONDA CL250 DUAL-TRANSPORT TELEMETRY STARTING  ");
    Serial.println("==================================================");
    Serial.printf(" [BOOT] Reset reason: %s\n", resetReasonName(esp_reset_reason()));

    // G1.4: enable Task Watchdog Timer and subscribe the main loop task to it.
    esp_task_wdt_init(TWDT_TIMEOUT_S, true /* panic + reset on timeout */);
    esp_task_wdt_add(NULL);

    pinMode(DEBUG_LOOP_PIN, OUTPUT);
    digitalWrite(DEBUG_LOOP_PIN, LOW);

    // Initialize all registered system modules
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        moduleActive[i] = modules[i]->begin();
        if (moduleActive[i]) {
            Serial.printf(" [OK] Module [%d] %s successfully initialized.\n", i, MODULE_NAMES[i]);
        } else {
            Serial.printf(" [WARNING] Module [%d] %s failed to initialize -- %s unavailable, excluded from update loop.\n",
                i, MODULE_NAMES[i], MODULE_NAMES[i]);
        }
    }

    Serial.println(" [SYSTEM] Setup completed. Dual BLE + Wi-Fi active.\n");
}

void loop() {
    digitalWrite(DEBUG_LOOP_PIN, HIGH);
    int64_t loopStartUs = esp_timer_get_time();

    // Update all system modules asynchronously with binding to global SystemState.
    // Modules that failed begin() (or later report unhealthy) are skipped so one
    // broken peripheral cannot stall the ones that are working (G0.2).
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        bool healthyNow = modules[i]->isHealthy();
        if (healthyNow != moduleActive[i]) {
            moduleActive[i] = healthyNow;
            Serial.printf(" [HEALTH] Module [%d] %s is now %s.\n",
                i, MODULE_NAMES[i], healthyNow ? "healthy" : "unhealthy -- excluded from update loop");
        }
        if (!moduleActive[i]) {
            continue;
        }

        int64_t moduleStartUs = esp_timer_get_time();
        modules[i]->update(globalState);
        moduleTiming[i].record((uint32_t)(esp_timer_get_time() - moduleStartUs));
    }

    loopTiming.record((uint32_t)(esp_timer_get_time() - loopStartUs));
    digitalWrite(DEBUG_LOOP_PIN, LOW);

    // G1.4: feed the watchdog once per completed loop pass. If any module hangs
    // (blocking I/O, infinite loop) this stops happening and TWDT resets the board.
    esp_task_wdt_reset();

    unsigned long now = millis();
    if (now - lastTimingReport >= TIMING_REPORT_INTERVAL_MS) {
        lastTimingReport = now;
        Serial.println("\n---- LOOP TIMING (last 10s, microseconds) ----");
        for (uint8_t i = 0; i < MODULE_COUNT; i++) {
            TimingStats& t = moduleTiming[i];
            if (t.samples > 0) {
                Serial.printf("  [%-8s] min=%6lu  avg=%6lu  max=%6lu  (n=%lu)\n",
                    MODULE_NAMES[i],
                    (unsigned long)t.minUs,
                    (unsigned long)(t.sumUs / t.samples),
                    (unsigned long)t.maxUs,
                    (unsigned long)t.samples);
            }
            t.reset();
        }
        if (loopTiming.samples > 0) {
            Serial.printf("  [%-8s] min=%6lu  avg=%6lu  max=%6lu  (n=%lu)\n",
                "LOOP",
                (unsigned long)loopTiming.minUs,
                (unsigned long)(loopTiming.sumUs / loopTiming.samples),
                (unsigned long)loopTiming.maxUs,
                (unsigned long)loopTiming.samples);
        }
        loopTiming.reset();
        Serial.println("-----------------------------------------------\n");
    }
}