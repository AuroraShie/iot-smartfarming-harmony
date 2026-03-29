#ifndef DHT11_SENSOR_H
#define DHT11_SENSOR_H

#include <Arduino.h>
#include <DHT.h>
#include "config.h"

// ============================================
// DHT11 传感器模块
// ============================================

// 传感器状态码
#define SENSOR_OK     1
#define SENSOR_ERROR  0
#define SENSOR_DISABLED -1

// 初始化DHT11传感器
int initDHT11();

// 读取DHT11数据
// 返回值: SENSOR_OK, SENSOR_ERROR, SENSOR_DISABLED
// 输出参数: humidity(湿度), temperature(温度)
int readDHT11(float &humidity, float &temperature);

#endif // DHT11_SENSOR_H