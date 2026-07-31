#ifndef SERIAL_LOGGER_MODULE_H
#define SERIAL_LOGGER_MODULE_H

#include "IModule.h"

/**
 * @brief Serial Logger debug module.
 * Formats and prints real-time telemetry metrics to USB Serial terminal for monitoring.
 */
class SerialLoggerModule : public IModule {
private:
    unsigned long _lastPrint = 0;
    unsigned long _printIntervalMs;

public:
    SerialLoggerModule(unsigned long printIntervalMs = 1000);
    bool begin() override;
    void update(SystemState& state) override;
};

#endif // SERIAL_LOGGER_MODULE_H
