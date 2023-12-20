///*******************************************
///@file watchdog.cpp
///@brief This class is wrapper around the arduino watchdog functions,
/// it will handle the watchdog timer and reset the arduino for the atmel328p and r4 minima/wifi
///*******************************************

#include "watchdog.h"

Watchdog watchdog;

Watchdog::Watchdog() {
}

void Watchdog::enable(int timeout_ms) {
    #ifdef __AVR__
    uint8_t timeout = static_cast<uint8_t>(msToWatchdogTimeout(timeout_ms));
    wdt_enable(timeout); // Enable watchdog timer with 8s timeout on AVR
    #elif _RENESAS_RA_
    WDT.begin(timeout_ms); // Enable watchdog timer with 8s timeout on Renesas RA
    #endif
}

void Watchdog::refresh() {
    #ifdef __AVR__
    wdt_reset(); // Reset watchdog timer on AVR
    #elif _RENESAS_RA_
    WDT.refresh(); // Reset watchdog timer on Renesas RA
    #endif
}

Watchdog::~Watchdog() {
    // Destructor logic here
    #ifdef __AVR__
    wdt_disable(); // Disable watchdog timer on AVR
    #elif _RENESAS_RA_
    WDT.~WDTimer(); // Disable watchdog timer on Renesas RA
    #endif
}

WatchdogTimeout Watchdog::msToWatchdogTimeout(int ms) {
    if (ms <= 15) return WatchdogTimeout::WDTO_15_MS;
    else if (ms <= 30) return WatchdogTimeout::WDTO_30_MS;
    else if (ms <= 60) return WatchdogTimeout::WDTO_60_MS;
    else if (ms <= 120) return WatchdogTimeout::WDTO_120_MS;
    else if (ms <= 250) return WatchdogTimeout::WDTO_250_MS;
    else if (ms <= 500) return WatchdogTimeout::WDTO_500_MS;
    else if (ms <= 1000) return WatchdogTimeout::WDTO_1_S;
    else return WatchdogTimeout::WDTO_2_S;
}