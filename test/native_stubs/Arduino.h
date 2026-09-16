#ifndef NATIVE_TEST_ARDUINO_STUB_H
#define NATIVE_TEST_ARDUINO_STUB_H

// G5.1/G5.2 -- Minimal Arduino.h stand-in so header-only, hardware-independent files
// (SystemState.h, BLETelemetryPacket.h) can be compiled and unit-tested on the host
// (platform = native) with no ESP32 toolchain or device attached. Only on the native
// build's include path (see platformio.ini's [env:native] build_flags) -- the real
// ESP32 environments never see this file, so this cannot affect on-device behavior.
//
// Deliberately NOT a general-purpose Arduino shim: add symbols here only as the
// specific headers under test require them.

#include <stdint.h>
#include <stddef.h>

// SystemState.h's isStale() calls millis(). Tests control the fake clock directly
// instead of sleeping in real time, so results are deterministic.
inline unsigned long& native_millis_ref() {
    static unsigned long fakeMillis = 0;
    return fakeMillis;
}

inline unsigned long millis() {
    return native_millis_ref();
}

// Test helper (not part of the real Arduino API) -- lets test code set the fake
// clock explicitly, e.g. to simulate "500ms have passed since the last update".
inline void test_setMillis(unsigned long ms) {
    native_millis_ref() = ms;
}

#endif // NATIVE_TEST_ARDUINO_STUB_H
