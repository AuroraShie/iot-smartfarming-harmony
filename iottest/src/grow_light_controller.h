#ifndef GROW_LIGHT_CONTROLLER_H
#define GROW_LIGHT_CONTROLLER_H

#include <Arduino.h>

/**
 * 补光继电器控制模块
 * 当前版本只做开关控制，不做真实亮度调光。
 */
class GrowLightController {
public:
    GrowLightController(uint8_t relayPin, bool activeLow);

    bool begin();
    bool turnOn(uint8_t level = 0);
    bool turnOff();

    bool isOn() const;
    uint8_t getLevel() const;

private:
    uint8_t relayPin;
    bool activeLow;
    bool growLightOn;
    uint8_t growLightLevel;

    void writeRelay(bool enabled);
};

extern GrowLightController growLightController;

#endif
