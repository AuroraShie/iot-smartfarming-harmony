#include "ds18b20_sensor.h"

// ============================================
// DS18B20 Temperature Sensor Class Implementation
// ============================================

// Constructor
DS18B20Sensor::DS18B20Sensor(int sensorPin)
    : pin(sensorPin),
      temperature(0.0f),
      dataValid(false),
      lastReadTime(0),
      oneWire(sensorPin),
      sensors(&oneWire) {
}

// Initialize sensor
int DS18B20Sensor::begin() {
    delay(50); // Wait for bus stabilization
    
    // Initialize DallasTemperature library
    sensors.begin();
    
    // Check device count
    int deviceCount = sensors.getDeviceCount();
    
    Serial.printf("[DS18B20] Initializing GPIO%d\n", pin);
    Serial.printf("[DS18B20] Devices found: %d\n", deviceCount);
    
    if (deviceCount == 0) {
        Serial.println("[DS18B20] Warning: No sensor detected! Check:");
        Serial.println("  - Wiring (VCC, GND, DATA)");
        Serial.println("  - 4.7K pull-up resistor between DATA and VCC");
        Serial.printf("  Wiring: VCC->3.3V/5V, GND->GND, DATA->GPIO%d\n", pin);
        return SENSOR_ERROR;
    }
    
    // Set resolution (12-bit, precision 0.0625°C)
    sensors.setResolution(12);
    Serial.printf("[DS18B20] Resolution set to 12-bit (%.4f°C precision)\n", 0.0625f);
    Serial.printf("[DS18B20] Temperature range: %.1f - %.1f °C\n", MIN_TEMP, MAX_TEMP);
    
    // Read device address
    byte deviceAddress[8];
    if (sensors.getAddress(deviceAddress, 0)) {
        Serial.print("[DS18B20] Device address: ");
        for (int i = 0; i < 8; i++) {
            Serial.print(deviceAddress[i], HEX);
            if (i < 7) Serial.print(":");
        }
        Serial.println();
    }
    
    Serial.println("[DS18B20] Sensor ready");
    
    // Reset state
    dataValid = false;
    lastReadTime = 0;
    
    return SENSOR_OK;
}

// Read temperature data
int DS18B20Sensor::readData() {
    unsigned long currentTime = millis();
    
    // Check read interval (prevent reading too fast)
    if (currentTime - lastReadTime < READ_INTERVAL) {
        // Not yet time to read, return previous data if valid
        if (dataValid) {
            return SENSOR_OK;
        }
        return SENSOR_ERROR;
    }
    
    // Request temperature conversion
    sensors.requestTemperatures();
    
    // Read first device temperature (Celsius)
    float tempReading = sensors.getTempCByIndex(0);
    
    // Validate reading
    if (validateReading(tempReading)) {
        temperature = tempReading;
        dataValid = true;
        lastReadTime = currentTime;
        return SENSOR_OK;
    } else {
        dataValid = false;
        return SENSOR_ERROR;
    }
}

// Get current temperature
float DS18B20Sensor::getTemperature() const {
    return temperature;
}

// Check if data is valid
bool DS18B20Sensor::isDataValid() const {
    return dataValid;
}

// Print debug information
void DS18B20Sensor::printDebugInfo() const {
    Serial.printf("Temperature: %.2f °C\n", temperature);
}

// Print status information
void DS18B20Sensor::printStatus() const {
    Serial.println("\nDS18B20 Sensor Status:");
    Serial.printf("Pin: GPIO%d\n", pin);
    Serial.printf("Temperature: %.2f °C\n", temperature);
    Serial.printf("Data valid: %s\n", dataValid ? "Yes" : "No");
    Serial.println("========================================================");
}

// Validate temperature reading (private method)
bool DS18B20Sensor::validateReading(float temp) {
    // Check if sensor is disconnected (DS18B20 returns -127 when disconnected)
    if (temp == DEVICE_DISCONNECTED_C) {
        Serial.println("[DS18B20] Error: Sensor not connected");
        return false;
    }
    
    // Check if temperature is within reasonable range
    if (temp < MIN_TEMP || temp > MAX_TEMP) {
        Serial.printf("[DS18B20] Warning: Abnormal temperature %.2f °C\n", temp);
        // Note: This is only a warning, reading is still accepted
        // To strictly filter, change to: return false;
    }
    
    return true;
}