#include "grow_light_controller.h"

#include "config.h"

GrowLightController growLightController(GROW_LIGHT_RELAY_PIN, RELAY_ACTIVE_LOW != 0);

GrowLightController::GrowLightController(uint8_t relayPinValue, bool activeLowValue)
    : relayPin(relayPinValue),
      activeLow(activeLowValue),
      growLightOn(false),
      growLightLevel(0) {
}

bool GrowLightController::begin() {
    pinMode(relayPin, OUTPUT);
    writeRelay(false);
    growLightOn = false;
    growLightLevel = 0;
    return true;
}

bool GrowLightController::turnOn(uint8_t level) {
    growLightOn = true;
    growLightLevel = level > 0 ? level : DEFAULT_GROW_LIGHT_LEVEL;
    writeRelay(true);
    return true;
}

bool GrowLightController::turnOff() {
    growLightOn = false;
    growLightLevel = 0;
    writeRelay(false);
    return true;
}

bool GrowLightController::isOn() const {
    return growLightOn;
}

uint8_t GrowLightController::getLevel() const {
    return growLightLevel;
}

void GrowLightController::writeRelay(bool enabled) {
    const uint8_t outputLevel = enabled
        ? (activeLow ? LOW : HIGH)
        : (activeLow ? HIGH : LOW);
    digitalWrite(relayPin, outputLevel);
}
