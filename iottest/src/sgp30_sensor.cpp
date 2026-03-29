#include "sgp30_sensor.h"

// SGP30传感器对象（全局单例）
static Adafruit_SGP30 sgp;

// ============================================
// SGP30 传感器实现
// ============================================

/**
 * @brief 初始化SGP30传感器
 * @return 状态码：
 *         SENSOR_OK - 初始化成功
 *         SENSOR_ERROR - 初始化失败
 */
int initSGP30() {
  // 设置SGP30专用I2C引脚
  Wire.begin(SGP30_SDA, SGP30_SCL);
  delay(100);
  
  // 尝试初始化SGP30
  if (!sgp.begin()) {
    return SENSOR_ERROR;
  }
  
  return SENSOR_OK;
}

/**
 * @brief 读取SGP30空气质量数据
 * @param tvoc 输出参数：总挥发性有机化合物（ppb）
 * @param eco2 输出参数：等效二氧化碳浓度（ppm）
 * @return 状态码：
 *         SENSOR_OK - 读取成功
 *         SENSOR_ERROR - 读取失败
 *         SENSOR_DISABLED - 传感器未初始化
 */
int readSGP30(uint16_t &tvoc, uint16_t &eco2) {
  // 检查传感器是否已初始化
  // SGP30没有直接的"isInitialized"方法，通过IAQmeasure返回值判断
  if (!sgp.IAQmeasure()) {
    return SENSOR_ERROR;
  }
  
  tvoc = sgp.TVOC;
  eco2 = sgp.eCO2;
  return SENSOR_OK;
}