#ifndef SGP30_SENSOR_H
#define SGP30_SENSOR_H

#include <Arduino.h>
#include <Adafruit_SGP30.h>
#include "config.h"

// ============================================
// SGP30 传感器模块
// ============================================

// 传感器状态码（与config.h保持一致）
#define SENSOR_OK     1
#define SENSOR_ERROR  0
#define SENSOR_DISABLED -1

// 初始化SGP30传感器
// 返回值: 初始化状态
int initSGP30();

// 读取SGP30数据
// 返回值: SENSOR_OK, SENSOR_ERROR, SENSOR_DISABLED
// 输出参数: tvoc(总挥发性有机化合物), eco2(等效二氧化碳)
int readSGP30(uint16_t &tvoc, uint16_t &eco2);

#endif // SGP30_SENSOR_H