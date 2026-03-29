#include "light_sensor.h"

// ============================================
// LightSensor Class Implementation
// ============================================

/**
 * @brief Constructor
 * @param digitalPin Digital pin number
 */
LightSensor::LightSensor(int digitalPin) : pin(digitalPin) {
    lightDetected = false;
}

/**
 * @brief Initialize sensor
 * @return Always returns LIGHT_SENSOR_OK
 */
bool LightSensor::begin() {
    pinMode(pin, INPUT);
    
    // Initial read
    readData();
    
    Serial.printf("Light sensor initialized on GPIO%d\n", pin);
    Serial.println("Note: Digital light sensor - LOW = light detected, HIGH = dark");
    
    return LIGHT_SENSOR_OK;
}

/**
 * @brief Read sensor data
 * @return Always returns true (digital sensor cannot fail)
 */
bool LightSensor::readData() {
    updateReading();
    return true;
}

/**
 * @brief Check if light is detected
 * @return true if light detected, false otherwise
 */
bool LightSensor::isLightDetected() const {
    return lightDetected;
}

/**
 * @brief Check if dark (opposite of isLightDetected)
 * @return true if dark (no light), false if light
 */
bool LightSensor::isDark() const {
    return !lightDetected;
}

/**
 * @brief Get raw digital pin value
 * @return Digital value (HIGH or LOW)
 */
int LightSensor::getDigitalValue() const {
    return digitalRead(pin);
}

/**
 * @brief Print debug information
 */
void LightSensor::printDebugInfo() const {
    int digitalValue = digitalRead(pin);
    Serial.printf("DEBUG: Light sensor GPIO%d = %d (Light: %s)\n",
                  pin,
                  digitalValue,
                  lightDetected ? "true" : "false");
}

/**
 * @brief Update internal reading (private method)
 */
void LightSensor::updateReading() {
    int digitalValue = digitalRead(pin);
    lightDetected = (digitalValue == LOW);  // 根据实际测试调整：LOW = 有光，HIGH = 无光
}
