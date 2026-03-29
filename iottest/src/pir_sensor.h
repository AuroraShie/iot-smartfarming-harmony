#ifndef PIR_SENSOR_H
#define PIR_SENSOR_H

#include <Arduino.h>
#include "config.h"

// ============================================
// HC-SR501 红外人体传感器 - 面向对象设计
// ============================================

// 传感器状态码
#define SENSOR_OK     1
#define SENSOR_ERROR  0
#define SENSOR_DISABLED -1

// ============================================
// PIRSensor 类定义
// ============================================
class PIRSensor {
private:
    int pin;
    bool motionDetected;
    int digitalValue;
    
    void updateReading();

public:
    explicit PIRSensor(int digitalPin);
    int begin();
    int readData();
    bool isMotionDetected() const;
    bool isIdle() const;
    int getDigitalValue() const;
    void printDebugInfo() const;
};

#endif // PIR_SENSOR_H