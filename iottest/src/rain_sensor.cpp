#include "rain_sensor.h"

// ============================================
// RainSensor Class Implementation
// ============================================

/**
 * @brief Constructor
 * @param digitalPin Digital pin number
 */
RainSensor::RainSensor(int digitalPin) : pin(digitalPin) {
    rainDetected = false;
}

/**
 * @brief Initialize sensor
 * @return Always returns RAIN_SENSOR_OK
 */
bool RainSensor::begin() {
    pinMode(pin, INPUT);
    
    // Initial read
    readData();
    
    Serial.printf("Rain sensor initialized on GPIO%d\n", pin);
    Serial.println("Note: MH-RD sensor - LOW = rain detected, HIGH = no rain");
    
    return RAIN_SENSOR_OK;
}

/**
 * @brief Read sensor data
 * @return Always returns true (digital sensor cannot fail)
 */
bool RainSensor::readData() {
    updateReading();
    return true;
}

/**
 * @brief Check if rain is detected
 * @return true if rain detected, false otherwise
 */
bool RainSensor::isRainDetected() const {
    return rainDetected;
}

/**
 * @brief Check if no rain (opposite of isRainDetected)
 * @return true if no rain, false if rain
 */
bool RainSensor::isNoRain() const {
    return !rainDetected;
}

/**
 * @brief Get raw digital pin value
 * @return Digital value (LOW or HIGH)
 */
int RainSensor::getDigitalValue() const {
    return digitalRead(pin);
}

/**
 * @brief Print debug information
 */
void RainSensor::printDebugInfo() const {
    int digitalValue = digitalRead(pin);
    Serial.printf("DEBUG: Rain sensor GPIO%d = %d (Rain: %s)\n",
                  pin,
                  digitalValue,
                  rainDetected ? "true" : "false");
}

/**
 * @brief Update internal reading (private method)
 */
void RainSensor::updateReading() {
    int digitalValue = digitalRead(pin);
    rainDetected = (digitalValue == LOW);
}