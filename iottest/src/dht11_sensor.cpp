#include "dht11_sensor.h"

// DHT传感器对象（全局单例）
static DHT dht(DHT_PIN, DHT_TYPE);

// ============================================
// DHT11 传感器实现
// ============================================

/**
 * @brief 初始化DHT11传感器
 * @return 总是返回 SENSOR_OK（初始化不会失败）
 */
int initDHT11() {
  dht.begin();
  return SENSOR_OK;
}

/**
 * @brief 读取DHT11温湿度数据
 * @param humidity 输出参数：湿度值（%）
 * @param temperature 输出参数：温度值（°C）
 * @return 状态码：
 *         SENSOR_OK - 读取成功
 *         SENSOR_ERROR - 读取失败（数据NaN）
 */
int readDHT11(float &humidity, float &temperature) {
  humidity = dht.readHumidity();
  temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    return SENSOR_ERROR;
  }
  return SENSOR_OK;
}