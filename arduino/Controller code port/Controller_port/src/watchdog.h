///*******************************************
///@file watchdog.h
///@brief This class is wrapper around the arduino watchdog functions,
/// it will handle the watchdog timer and reset the arduino for the atmel328p and r4 minima/wifi
///*******************************************

#ifndef WATCHDOG_H
#define WATCHDOG_H

#include <Arduino.h>
#ifdef __AVR__
#include <avr/wdt.h>
#elif _RENESAS_RA_
#include <WDT.h>
#endif

enum class WatchdogTimeout {
    WDTO_15_MS = 0,
    WDTO_30_MS = 1,
    WDTO_60_MS = 2,
    WDTO_120_MS = 3,
    WDTO_250_MS = 4,
    WDTO_500_MS = 5,
    WDTO_1_S = 6,
    WDTO_2_S = 7
};

class Watchdog {
public:
    Watchdog();
    void enable(int timeout_ms);
    void refresh();

    ~Watchdog();
private:
    WatchdogTimeout msToWatchdogTimeout(int ms);

};

extern Watchdog watchdog;

#endif // WATCHDOG_H