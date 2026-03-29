#ifndef LIGHT_SENSOR_H
#define LIGHT_SENSOR_H

#include <Arduino.h>
#include "config.h"

// ============================================
// 光敏传感器 - 面向对象设计
// ============================================

// 传感器状态码
#define LIGHT_SENSOR_OK     1
#define LIGHT_SENSOR_ERROR  0
#define LIGHT_SENSOR_DISABLED -1

// ============================================
// LightSensor Class Definition
// ============================================
class LightSensor {
private:
    int pin;
    bool lightDetected;  // true = 有光, false = 无光（根据传感器特性调整）
    
    void updateReading();

public:
    LightSensor(int digitalPin);
    bool begin();
    bool readData();
    bool isLightDetected() const;  // 有光
    bool isDark() const;          // 无光（暗）
    int getDigitalValue() const;
    void printDebugInfo() const;
};

#endif // LIGHT_SENSOR_H