#ifndef IMODULE_H
#define IMODULE_H

#include "SystemState.h"

/**
 * @brief Common base for every telemetry system module: lifecycle and health only.
 * update() itself lives on IProducerModule/IConsumerModule (see below) so the
 * compiler enforces who may write to SystemState (G3.1).
 */
class IModule {
public:
    virtual ~IModule() {}

    /**
     * @brief Initializes module hardware, peripherals, and driver configurations.
     * @return true if initialization succeeded, false otherwise.
     */
    virtual bool begin() = 0;

    /**
     * @brief Reports whether the module initialized correctly and is safe to update().
     * main.cpp excludes unhealthy modules from the update loop after begin() runs.
     * @return true if the module is operating normally.
     */
    virtual bool isHealthy() const = 0;
};

/**
 * @brief G3.1 -- A module that writes into SystemState: HondaCANModule (engine),
 * IMUModule (dynamics), BLEServerModule (telematics, from phone-sourced BLE writes).
 * Producers run first each loop pass so every consumer sees that pass's freshest data.
 */
class IProducerModule : public IModule {
public:
    /**
     * @brief Reads sensors/communication streams and writes into SystemState.
     * @param state Reference to the global SystemState object.
     */
    virtual void update(SystemState& state) = 0;
};

/**
 * @brief G3.1 -- A module that only reads SystemState: NextionModule, WiFiServerModule,
 * SerialLoggerModule. const& means the compiler rejects any accidental write attempt.
 */
class IConsumerModule : public IModule {
public:
    /**
     * @brief Reads SystemState to render/report/serve it; must not modify it.
     * @param state Const reference to the global SystemState object.
     */
    virtual void update(const SystemState& state) = 0;
};

#endif // IMODULE_H