#include "hw390_sensor.h"

// ============================================
// 全局实例（用于C风格API的后向兼容）
// ============================================
static HW390Sensor hw390SensorInstance(SOIL_MOISTURE_PIN);

// ============================================
// 校准工具类实现
// ============================================

/**
 * @brief 将原始ADC值转换为湿度百分比
 * 使用参考代码中的校准公式
 * @param rawValue 原始ADC值（0-4095）
 * @return 湿度百分比（0-100）
 */
int SoilMoistureCalibration::convertToPercentage(int rawValue) {
    // 基于参考代码的校准曲线
    if (rawValue >= 3500) return 0;      // 完全干燥
    if (rawValue <= 1000) return 100;    // 完全湿润
    return 100 - ((rawValue - 1000) * 100) / (3500 - 1000);
}

/**
 * @brief 验证土壤湿度读数是否有效
 * @param rawValue 原始ADC值
 * @return true表示有效，false表示无效
 */
bool CalibrationUtils::validateSoilMoistureReading(int rawValue) {
    return (rawValue >= 0 && rawValue <= 4095);
}

// ============================================
// HW390Sensor 类实现
// ============================================

/**
 * @brief 构造函数
 * @param analogPin 模拟引脚编号
 */
HW390Sensor::HW390Sensor(int analogPin) : pin(analogPin) {
    rawValue = 0;
    percentage = 0;
}

/**
 * @brief 读取传感器数据
 * @return true表示读取成功，false表示失败
 */
bool HW390Sensor::readData() {
    long sampleSum = 0;
    int minValue = 4095;
    int maxValue = 0;

    for (int i = 0; i < SOIL_SENSOR_SAMPLE_COUNT; ++i) {
        const int sample = analogRead(pin);
        sampleSum += sample;
        if (sample < minValue) {
            minValue = sample;
        }
        if (sample > maxValue) {
            maxValue = sample;
        }
        delay(2);
    }

    if (SOIL_SENSOR_SAMPLE_COUNT >= 3) {
        sampleSum -= minValue;
        sampleSum -= maxValue;
        rawValue = static_cast<int>(sampleSum / (SOIL_SENSOR_SAMPLE_COUNT - 2));
    } else {
        rawValue = static_cast<int>(sampleSum / SOIL_SENSOR_SAMPLE_COUNT);
    }

    percentage = convertToPercentage(rawValue);
    return isValidReading();
}

/**
 * @brief 获取原始ADC值
 * @return 原始ADC值（0-4095）
 */
int HW390Sensor::getRawValue() const {
    return rawValue;
}

/**
 * @brief 获取湿度百分比
 * @return 湿度百分比（0-100）
 */
int HW390Sensor::getPercentage() const {
    return percentage;
}

/**
 * @brief 打印调试信息
 */
void HW390Sensor::printDebugInfo() const {
    Serial.printf("DEBUG: Soil Moisture GPIO%d = %d (%d%% moisture)\n",
                  pin, rawValue, percentage);
}

/**
 * @brief 验证读数是否有效（私有方法）
 * @return true表示有效，false表示无效
 */
bool HW390Sensor::isValidReading() const {
    return CalibrationUtils::validateSoilMoistureReading(rawValue);
}

/**
 * @brief 私有方法：将原始ADC值转换为湿度百分比
 * @param rawValue 原始ADC值
 * @return 湿度百分比
 */
int HW390Sensor::convertToPercentage(int rawValue) {
    return SoilMoistureCalibration::convertToPercentage(rawValue);
}

/**
 * @brief 初始化HW-390传感器
 * @return 总是返回 SENSOR_OK
 */
int initHW390() {
    // 设置ADC引脚衰减
    analogSetAttenuation(ADC_11db);    // 设置ADC衰减为11dB（输入电压范围0-3.6V）
    
    delay(50); // 等待ADC稳定
    return SENSOR_OK;
}

/**
 * @brief 读取HW-390土壤湿度数据（C风格API，后向兼容）
 * @param moisture 输出参数：土壤湿度百分比（0-100%）
 * @return 状态码：SENSOR_OK 或 SENSOR_ERROR
 */
int readHW390(float &moisture) {
    if (hw390SensorInstance.readData()) {
        moisture = hw390SensorInstance.getPercentage();
        return SENSOR_OK;
    } else {
        moisture = 0.0;
        return SENSOR_ERROR;
    }
}
