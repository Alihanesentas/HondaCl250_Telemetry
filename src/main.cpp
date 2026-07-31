#include <Arduino.h>

#include "SystemState.h"
#include "IModule.h"
#include "HondaCANModule.h"
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

// Hardware Serial 2 Instance for Nextion HMI Display
HardwareSerial NextionSerial(2);

// ============================================================================
// CENTRAL TELEMETRY STATE STORE
// ============================================================================
SystemState globalState;

// ============================================================================
// SYSTEM MODULE INSTANTIATIONS
// ============================================================================
HondaCANModule     canModule(CAN_TX_PIN, CAN_RX_PIN);
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

    // Initialize all registered system modules
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        if (modules[i]->begin()) {
            Serial.printf(" [OK] Module [%d] successfully initialized.\n", i);
        } else {
            Serial.printf(" [WARNING] Module [%d] failed to initialize!\n", i);
        }
    }

    Serial.println(" [SYSTEM] Setup completed. Dual BLE + Wi-Fi active.\n");
}

void loop() {
    // Update all system modules asynchronously with binding to global SystemState
    for (uint8_t i = 0; i < MODULE_COUNT; i++) {
        modules[i]->update(globalState);
    }
}