#ifndef BLE_SERVER_MODULE_H
#define BLE_SERVER_MODULE_H

#include "IModule.h"
#include "BLETelemetryPacket.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

/**
 * @brief Bluetooth Low Energy (BLE) Server module.
 * Streams real-time telemetry packets to connected mobile devices (iOS / Android)
 * and receives smartphone telematics (music track info, turn-by-turn navigation data).
 */
class BLEServerModule : public IModule, public BLEServerCallbacks, public BLECharacteristicCallbacks {
private:
    BLEServer* _pServer = nullptr;
    BLECharacteristic* _pTxCharacteristic = nullptr;
    BLECharacteristic* _pRxCharacteristic = nullptr;

    bool _deviceConnected = false;
    bool _oldDeviceConnected = false;
    unsigned long _lastNotify = 0;

    SystemState* _pSystemState = nullptr;

public:
    BLEServerModule();
    virtual ~BLEServerModule() {}

    bool begin() override;
    void update(SystemState& state) override;

    // BLEServerCallbacks
    void onConnect(BLEServer* pServer) override;
    void onDisconnect(BLEServer* pServer) override;

    // BLECharacteristicCallbacks
    void onWrite(BLECharacteristic* pCharacteristic) override;
};

#endif // BLE_SERVER_MODULE_H
