#ifndef DS18B20_SENSOR_H
#define DS18B20_SENSOR_H

#include <Arduino.h>
#include "config.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// ============================================
// DS18B20 Temperature Sensor Class
// ============================================

// Sensor status codes
#define SENSOR_OK     1
#define SENSOR_ERROR  0
#define SENSOR_DISABLED -1

class DS18B20Sensor {
private:
    int pin;                    // Sensor pin
    float temperature;          // Current temperature value
    bool dataValid;             // Data validity flag
    unsigned long lastReadTime; // Last read timestamp
    
    OneWire oneWire;            // OneWire bus object
    DallasTemperature sensors;  // DallasTemperature library object
    
    // Static constants
    static const unsigned long READ_INTERVAL = 750;  // Read interval (ms)
    static constexpr float MIN_TEMP = -20.0f;        // Minimum temperature (C)
    static constexpr float MAX_TEMP = 60.0f;         // Maximum temperature (C)
    
    // Validate temperature reading
    bool validateReading(float temp);
    
public:
    // Constructor
    DS18B20Sensor(int sensorPin);
    
    // Initialize sensor
    int begin();
    
    // Read temperature data
    int readData();
    
    // Get current temperature
    float getTemperature() const;
    
    // Check if data is valid
    bool isDataValid() const;
    
    // Print debug information
    void printDebugInfo() const;
    
    // Print status information
    void printStatus() const;
};

#endif // DS18B20_SENSOR_H