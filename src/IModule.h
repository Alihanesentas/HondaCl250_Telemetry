#ifndef IMODULE_H
#define IMODULE_H

#include "SystemState.h"

/**
 * @brief Abstract interface for all telemetry system modules.
 * Every hardware peripheral or communication handler implements this interface.
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
     * @brief Asynchronous non-blocking update routine called inside main loop.
     * Reads sensors, processes communication streams, and updates SystemState.
     * @param state Reference to the global SystemState object.
     */
    virtual void update(SystemState& state) = 0;
};

#endif // IMODULE_H