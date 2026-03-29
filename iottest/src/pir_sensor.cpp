#include "pir_sensor.h"

// ============================================
// PIRSensor 类实现
// ============================================

/**
 * @brief 构造函数
 * @param digitalPin 数字引脚编号
 */
PIRSensor::PIRSensor(int digitalPin)
    : pin(digitalPin), motionDetected(false), digitalValue(LOW) {
}

/**
 * @brief 初始化PIR传感器
 * @return 总是返回 SENSOR_OK
 */
int PIRSensor::begin() {
    pinMode(pin, INPUT);
    updateReading();

    Serial.printf("PIR sensor initialized on GPIO%d\n", pin);
    Serial.println("Note: HC-SR501 sensor - HIGH = motion detected, LOW = idle");

    return SENSOR_OK;
}

/**
 * @brief 读取传感器数据
 * @return SENSOR_OK（数字传感器不会失败）
 */
int PIRSensor::readData() {
    updateReading();
    return SENSOR_OK;
}

/**
 * @brief 是否检测到运动
 * @return true表示检测到运动，false表示无运动
 */
bool PIRSensor::isMotionDetected() const {
    return motionDetected;
}

/**
 * @brief 是否处于空闲状态（与isMotionDetected相反）
 * @return true表示空闲，false表示有运动
 */
bool PIRSensor::isIdle() const {
    return !motionDetected;
}

/**
 * @brief 获取数字引脚原始值
 * @return 数字值（LOW或HIGH）
 */
int PIRSensor::getDigitalValue() const {
    return digitalValue;
}

/**
 * @brief 打印调试信息
 */
void PIRSensor::printDebugInfo() const {
    Serial.printf("DEBUG: PIR sensor GPIO%d = %d (Motion: %s)\n",
                  pin,
                  digitalValue,
                  motionDetected ? "true" : "false");
}

/**
 * @brief 更新内部读数（私有方法）
 */
void PIRSensor::updateReading() {
    digitalValue = digitalRead(pin);
    motionDetected = (digitalValue == HIGH);
}