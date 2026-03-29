#ifndef HW390_SENSOR_H
#define HW390_SENSOR_H

#include <Arduino.h>
#include "config.h"

// ============================================
// HW-390 土壤湿度传感器 - 面向对象设计
// ============================================

// 传感器状态码
#define SENSOR_OK     1
#define SENSOR_ERROR  0
#define SENSOR_DISABLED -1

// ============================================
// 校准工具类
// ============================================
class SoilMoistureCalibration {
public:
    static int convertToPercentage(int rawValue);
};

class CalibrationUtils {
public:
    static bool validateSoilMoistureReading(int rawValue);
};

// ============================================
// HW390 传感器类
// ============================================
class HW390Sensor {
private:
    int pin;
    int rawValue;
    int percentage;
    
    int convertToPercentage(int rawValue);
    bool isValidReading() const;

public:
    HW390Sensor(int analogPin);
    bool readData();
    int getRawValue() const;
    int getPercentage() const;
    void printDebugInfo() const;
};

// 向后兼容的C风格API
int initHW390();
int readHW390(float &moisture);

#endif // HW390_SENSOR_H
