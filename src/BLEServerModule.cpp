#include "BLEServerModule.h"

// Custom UUIDs for Honda Telemetry BLE Service & Characteristics
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_TX "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RX "828919fe-e41c-40ee-b4c6-2c974c2d3345"

// Standard Device Information Service UUID (0x180A)
#define DEVICE_INFO_SERVICE_UUID "0000180a-0000-1000-8000-00805f9b34fb"

BLEServerModule::BLEServerModule() {}

bool BLEServerModule::begin() {
    // Initialize BLE Device with maximum MTU (512 bytes)
    BLEDevice::init("Honda-CL250-Telemetry");
    BLEDevice::setMTU(512);

    // Create BLE Server instance
    _pServer = BLEDevice::createServer();
    _pServer->setCallbacks(this);

    // 1. Create Standard Device Information Service (0x180A)
    BLEService* pInfoService = _pServer->createService(BLEUUID(DEVICE_INFO_SERVICE_UUID));
    
    // Manufacturer Name String (0x2A29)
    BLECharacteristic* pMfgChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A29),
        BLECharacteristic::PROPERTY_READ
    );
    pMfgChar->setValue("Honda Telemetry Labs");

    // Model Number String (0x2A24)
    BLECharacteristic* pModelChar = pInfoService->createCharacteristic(
        BLEUUID((uint16_t)0x2A24),
        BLECharacteristic::PROPERTY_READ
    );
    pModelChar->setValue("CL250-v1.0");

    pInfoService->start();

    // 2. Create Custom Telemetry Service
    BLEService* pService = _pServer->createService(SERVICE_UUID);

    // Create Telemetry TX (Notify) Characteristic
    _pTxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_TX,
        BLECharacteristic::PROPERTY_NOTIFY
    );
    _pTxCharacteristic->addDescriptor(new BLE2902());

    // Create Telematics RX (Write) Characteristic
    _pRxCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID_RX,
        BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR
    );
    _pRxCharacteristic->setCallbacks(this);

    pService->start();

    // 3. Configure Fast Auto-Advertising for iOS & Android
    BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06); // Preferred connection interval for fast iPhone pairing
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();

    return true;
}

void BLEServerModule::onConnect(BLEServer* pServer) {
    _deviceConnected = true;
    if (_pSystemState) {
        _pSystemState->telematics.phoneConnected = true;
    }
}

void BLEServerModule::onDisconnect(BLEServer* pServer) {
    _deviceConnected = false;
    if (_pSystemState) {
        _pSystemState->telematics.phoneConnected = false;
    }
}

void BLEServerModule::onWrite(BLECharacteristic* pCharacteristic) {
    std::string rxValue = pCharacteristic->getValue();
    if (rxValue.length() > 0 && _pSystemState) {
        const char* data = rxValue.c_str();
        
        const char* songPtr = strstr(data, "SONG:");
        if (songPtr) {
            sscanf(songPtr, "SONG:%31[^|]", _pSystemState->telematics.songTitle);
        }

        const char* artistPtr = strstr(data, "ARTIST:");
        if (artistPtr) {
            sscanf(artistPtr, "ARTIST:%31[^|]", _pSystemState->telematics.artistName);
        }

        const char* distPtr = strstr(data, "DIST:");
        if (distPtr) {
            int distVal = 0;
            if (sscanf(distPtr, "DIST:%d", &distVal) == 1) {
                _pSystemState->telematics.navDistance = (uint16_t)distVal;
            }
        }

        const char* iconPtr = strstr(data, "ICON:");
        if (iconPtr) {
            int iconVal = 0;
            if (sscanf(iconPtr, "ICON:%d", &iconVal) == 1) {
                _pSystemState->telematics.navIconID = (uint8_t)iconVal;
            }
        }
    }
}

void BLEServerModule::update(SystemState& state) {
    _pSystemState = &state;
    unsigned long now = millis();

    // Stream real-time binary packet at 10Hz (100ms interval) to connected phone
    if (_deviceConnected && (now - _lastNotify >= 100)) {
        _lastNotify = now;

        BLETelemetryPacket packet;
        packet.rpm          = (uint16_t)state.engine.rpm;
        packet.speed        = state.engine.speed;
        packet.coolantTemp  = (int8_t)state.engine.coolantTemp;
        packet.throttlePos  = (uint8_t)state.engine.throttlePos;
        packet.batteryVolt  = (uint16_t)(state.engine.batteryVoltage * 1000.0f);
        packet.leanAngle    = (int16_t)(state.dynamics.leanAngle * 10.0f);
        packet.maxLeanRight = (int16_t)(state.dynamics.maxLeanRight * 10.0f);
        packet.maxLeanLeft  = (int16_t)(state.dynamics.maxLeanLeft * 10.0f);

        _pTxCharacteristic->setValue((uint8_t*)&packet, sizeof(BLETelemetryPacket));
        _pTxCharacteristic->notify();
    }

    // Auto-restart BLE advertising upon disconnection so phone can reconnect
    if (!_deviceConnected && _oldDeviceConnected) {
        delay(500); // Reset delay
        _pServer->startAdvertising();
        _oldDeviceConnected = _deviceConnected;
    }

    if (_deviceConnected && !_oldDeviceConnected) {
        _oldDeviceConnected = _deviceConnected;
    }
}
