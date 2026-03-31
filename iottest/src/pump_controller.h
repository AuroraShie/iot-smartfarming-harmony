#ifndef PUMP_CONTROLLER_H
#define PUMP_CONTROLLER_H

#include <Arduino.h>

/**
 * 水泵继电器控制模块
 * 负责继电器初始化、开关控制和定时关泵。
 */
class PumpController {
public:
    PumpController(uint8_t relayPin, bool activeLow);

    bool begin();
    bool turnOn(uint32_t durationSec);
    bool turnOff();
    void update();

    bool isOn() const;
    uint32_t getLastDurationSec() const;
    unsigned long getAutoOffAtMs() const;

private:
    uint8_t relayPin;
    bool activeLow;
    bool pumpOn;
    uint32_t lastDurationSec;
    unsigned long autoOffAtMs;

    void writeRelay(bool enabled);
};

extern PumpController pumpController;

#endif
