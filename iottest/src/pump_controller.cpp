#include "pump_controller.h"

#include "config.h"

PumpController pumpController(PUMP_RELAY_PIN, RELAY_ACTIVE_LOW != 0);

PumpController::PumpController(uint8_t relayPinValue, bool activeLowValue)
    : relayPin(relayPinValue),
      activeLow(activeLowValue),
      pumpOn(false),
      lastDurationSec(0),
      autoOffAtMs(0) {
}

bool PumpController::begin() {
    pinMode(relayPin, OUTPUT);
    writeRelay(false);
    pumpOn = false;
    lastDurationSec = 0;
    autoOffAtMs = 0;
    return true;
}

bool PumpController::turnOn(uint32_t durationSec) {
    pumpOn = true;
    lastDurationSec = durationSec;
    autoOffAtMs = durationSec > 0 ? millis() + durationSec * 1000UL : 0;
    writeRelay(true);
    return true;
}

bool PumpController::turnOff() {
    pumpOn = false;
    lastDurationSec = 0;
    autoOffAtMs = 0;
    writeRelay(false);
    return true;
}

void PumpController::update() {
    if (pumpOn && autoOffAtMs > 0 && millis() >= autoOffAtMs) {
        turnOff();
    }
}

bool PumpController::isOn() const {
    return pumpOn;
}

uint32_t PumpController::getLastDurationSec() const {
    return lastDurationSec;
}

unsigned long PumpController::getAutoOffAtMs() const {
    return autoOffAtMs;
}

void PumpController::writeRelay(bool enabled) {
    const uint8_t outputLevel = enabled
        ? (activeLow ? LOW : HIGH)
        : (activeLow ? HIGH : LOW);
    digitalWrite(relayPin, outputLevel);
}
