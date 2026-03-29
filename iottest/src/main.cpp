/**
 * ESP32 多传感器环境监测系统
 * 
 * 功能说明：
 * - 支持7种传感器数据采集
 * - RGB LED状态指示
 * - 网络传输功能（WiFi/HTTP）
 * - 预留自动控制接口（继电器/LED补光）
 * 
 * 版本：3.0（网络互联版本）
 */

#include <Arduino.h>
#include "config.h"
#include "dht11_sensor.h"
#include "sgp30_sensor.h"
#include "hw390_sensor.h"
#include "ds18b20_sensor.h"
#include "rain_sensor.h"
#include "pir_sensor.h"
#include "light_sensor.h"
#include "network_module.h"  // 网络通信模块

// ============================================
// 传感器状态变量
// ============================================
bool dht11Available = false;
bool sgp30Available = false;
bool hw390Available = false;
bool rainSensorAvailable = false;
bool ds18b20Sensor1Available = false;
bool ds18b20Sensor2Available = false;
bool pirAvailable = false;
bool lightSensorAvailable = false;

// ============================================
// 传感器实例
// ============================================
HW390Sensor hw390Sensor(SOIL_MOISTURE_PIN);
RainSensor rainSensor(RAIN_SENSOR_PIN);
DS18B20Sensor ds18b20Sensor1(DS18B20_PIN_1);
DS18B20Sensor ds18b20Sensor2(DS18B20_PIN_2);
PIRSensor pirSensor(PIR_PIN);
LightSensor lightSensor(LIGHT_SENSOR_PIN);

// DS18B20平均温度
float ds18b20AvgTemperature = 0.0f;
bool ds18b20AvgValid = false;

// 当前传感器数据
SensorData currentSensorData;

// ============================================
// RGB LED 控制模块
// ============================================
void setupRGB() {
    ledcSetup(LEDC_CHANNEL_RED, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_GREEN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(RGB_RED_PIN, LEDC_CHANNEL_RED);
    ledcAttachPin(RGB_GREEN_PIN, LEDC_CHANNEL_GREEN);
    ledcWrite(LEDC_CHANNEL_RED, 0);
    ledcWrite(LEDC_CHANNEL_GREEN, 0);
}

void setRGBColor(int red, int green, int blue) {
    if (red >= 0) ledcWrite(LEDC_CHANNEL_RED, constrain(red, 0, 255));
    if (green >= 0) ledcWrite(LEDC_CHANNEL_GREEN, constrain(green, 0, 255));
}

void setRed() { setRGBColor(BRIGHTNESS_RED, 0, -1); }
void setGreen() { setRGBColor(0, BRIGHTNESS_GREEN, -1); }
void setYellow() { setRGBColor(BRIGHTNESS_YELLOW_RED, BRIGHTNESS_YELLOW_GREEN, -1); }
void setOff() { setRGBColor(0, 0, -1); }

// ============================================
// 自动控制模块函数（预留接口）
// ============================================

/**
 * 初始化继电器引脚（预留接口）
 */
void setupRelay() {
    // TODO: 实现继电器初始化
    // pinMode(RELAY_PIN, OUTPUT);
    // digitalWrite(RELAY_PIN, LOW);
    // Serial.println("继电器初始化完成");
}

/**
 * 控制水泵灌溉（预留接口）
 * 
 * @param duration 灌溉时长（毫秒）
 */
void startIrrigation(int duration) {
    // TODO: 实现灌溉控制
    // Serial.println("开始灌溉...");
    // digitalWrite(RELAY_PIN, HIGH);
    // delay(duration);
    // digitalWrite(RELAY_PIN, LOW);
    // Serial.println("灌溉完成");
}

/**
 * 初始化补光LED引脚（预留接口）
 */
void setupGrowLight() {
    // TODO: 实现补光灯初始化
    // pinMode(GROW_LIGHT_PIN, OUTPUT);
    // digitalWrite(GROW_LIGHT_PIN, LOW);
    // Serial.println("补光灯初始化完成");
}

/**
 * 控制补光灯（预留接口）
 * 
 * @param state true开启，false关闭
 */
void setGrowLight(bool state) {
    // TODO: 实现补光灯控制
    // digitalWrite(GROW_LIGHT_PIN, state ? HIGH : LOW);
}

// ============================================
// 数据存储模块函数（预留接口）
// ============================================

/**
 * 保存传感器数据到SD卡（预留接口）
 * 
 * @param data 数据字符串
 * @return true 保存成功
 * @return false 保存失败
 */
bool saveToSDCard(const String& data) {
    // TODO: 实现SD卡存储
    // File file = SD.open("/sensor_log.txt", FILE_APPEND);
    // if (file) {
    //     file.println(data);
    //     file.close();
    //     return true;
    // }
    return false;
}

// ============================================
// 辅助函数
// ============================================

/**
 * 打印传感器状态
 */
void printSensorStatus(const char* name, int status) {
    Serial.print("  状态: ");
    switch (status) {
        case SENSOR_OK:
            Serial.println("✓ 正常");
            break;
        case SENSOR_ERROR:
            Serial.println("✗ 读取错误");
            break;
        case SENSOR_DISABLED:
            Serial.println("⚠ 未启用");
            break;
        default:
            Serial.println("? 未知状态");
            break;
    }
}

/**
 * 打印系统状态汇总
 */
void printSystemStatus() {
    Serial.println("========== 当前读取状态汇总 ==========");
    Serial.print("DHT11: ");
    Serial.println(dht11Available ? "✓ 已启用" : "✗ 未启用");
    Serial.print("SGP30: ");
    Serial.println(sgp30Available ? "✓ 已启用" : "✗ 未启用");
    Serial.print("HW-390: ");
    Serial.println(hw390Available ? "✓ 已启用" : "✗ 未启用");
    
    if (ds18b20Sensor1Available || ds18b20Sensor2Available) {
        Serial.print("DS18B20: ");
        if (ds18b20AvgValid) {
            Serial.printf("✓ 已启用 (平均: %.1f°C)", ds18b20AvgTemperature);
        } else {
            Serial.print("✓ 已启用 (数据无效)");
        }
        Serial.println(ds18b20Sensor1Available && ds18b20Sensor2Available ? " - 双传感器模式" : " - 单传感器模式");
    } else {
        Serial.println("DS18B20: ✗ 未启用");
    }
    
    Serial.print("HC-SR501: ");
    Serial.println(pirAvailable ? "✓ 已启用" : "✗ 未启用");
    Serial.print("MH-RD: ");
    Serial.println(rainSensorAvailable ? "✓ 已启用" : "✗ 未启用");
    Serial.print("光敏传感器: ");
    Serial.println(lightSensorAvailable ? "✓ 已启用" : "✗ 未启用");
    
    // 网络状态
    Serial.print("网络状态: ");
    NetworkStatus netStatus = networkManager.getStatus();
    switch (netStatus) {
        case NET_CONNECTED:
            Serial.println("✓ 已连接");
            break;
        case NET_CONNECTING:
            Serial.println("⏳ 连接中...");
            break;
        case NET_DISCONNECTED:
            Serial.println("✗ 未连接");
            break;
        case NET_ERROR:
            Serial.println("⚠ 错误状态");
            break;
        default:
            Serial.println("? 未知状态");
            break;
    }
    
    Serial.println("==================================");
}

// ============================================
// 传感器初始化函数
// ============================================

/**
 * 初始化所有传感器
 */
void initSensors() {
    Serial.println("正在初始化传感器...");
    Serial.println();
    
    // DHT11 温湿度传感器
    if (initDHT11() == SENSOR_OK) {
        dht11Available = true;
        Serial.println("✓ DHT11 初始化成功");
    } else {
        Serial.println("✗ DHT11 初始化失败");
    }

    // SGP30 空气质量传感器
    if (initSGP30() == SENSOR_OK) {
        sgp30Available = true;
        Serial.println("✓ SGP30 初始化成功");
    } else {
        Serial.println("✗ SGP30 初始化失败");
    }

    // HW-390 土壤湿度传感器
    if (initHW390() == SENSOR_OK) {
        hw390Available = true;
        Serial.println("✓ HW-390 初始化成功");
    } else {
        Serial.println("✗ HW-390 初始化失败");
    }

    // DS18B20 温度传感器 1
    if (ds18b20Sensor1.begin() == SENSOR_OK) {
        ds18b20Sensor1Available = true;
        Serial.println("✓ DS18B20 传感器1 初始化成功");
    } else {
        Serial.println("✗ DS18B20 传感器1 初始化失败");
    }
    
    // DS18B20 温度传感器 2
    if (ds18b20Sensor2.begin() == SENSOR_OK) {
        ds18b20Sensor2Available = true;
        Serial.println("✓ DS18B20 传感器2 初始化成功");
    } else {
        Serial.println("✗ DS18B20 传感器2 初始化失败");
    }

    // HC-SR501 人体传感器
    if (pirSensor.begin() == SENSOR_OK) {
        pirAvailable = true;
        Serial.println("✓ HC-SR501 初始化成功");
    } else {
        Serial.println("✗ HC-SR501 初始化失败");
    }

    // MH-RD 雨滴传感器
    if (rainSensor.begin()) {
        rainSensorAvailable = true;
        Serial.println("✓ MH-RD 雨滴传感器 初始化成功");
    } else {
        Serial.println("✗ MH-RD 雨滴传感器 初始化失败");
    }

    // 光敏传感器
    if (lightSensor.begin()) {
        lightSensorAvailable = true;
        Serial.println("✓ 光敏传感器 初始化成功");
    } else {
        Serial.println("✗ 光敏传感器 初始化失败");
    }
    
    Serial.println();
}

/**
 * 初始化传感器数据结构
 */
void initSensorData() {
    currentSensorData.temperature = 0.0f;
    currentSensorData.humidity = 0.0f;
    currentSensorData.light = 0;
    currentSensorData.tvoc = 0;
    currentSensorData.eco2 = 0;
    currentSensorData.soilMoisture = 0;
    currentSensorData.soilTemperature = 0.0f;
    currentSensorData.rainDetected = false;
    currentSensorData.motionDetected = false;
    currentSensorData.timestamp = "";
}

// ============================================
// 主程序
// ============================================

void setup() {
    // 初始化串口
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);

    Serial.println("==================================");
    Serial.println("ESP32 多传感器环境监测系统");
    Serial.println("版本：3.0 (网络互联版本)");
    Serial.println("==================================");
    Serial.println();

    // 初始化传感器数据结构
    initSensorData();

    // 初始化传感器
    initSensors();

    // 初始化板载LED
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    digitalWrite(BUILTIN_LED_PIN, LOW);
    Serial.printf("板载LED已配置在 GPIO%d\n", BUILTIN_LED_PIN);

    // 初始化RGB LED
    setupRGB();
    Serial.println("✓ RGB LED 初始化完成");

    // 初始化网络模块
    networkManager.begin();

    // 自动控制初始化（预留）
    // setupRelay();
    // setupGrowLight();

    Serial.println();
    Serial.println("==================================");
    Serial.println("系统就绪，开始读取数据...");
    Serial.println("==================================");
    Serial.println();
}

void loop() {
    // 更新网络状态
    networkManager.update();

    // 等待读取间隔
    delay(SENSOR_READ_INTERVAL);

    Serial.println("----------------------------------");
    Serial.print("运行时间: ");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
    Serial.println();

    // 传感器状态标志
    bool allSensorsOK = true;
    bool anySensorFailed = false;
    bool hasAbnormalCondition = false;

    // 重置传感器数据
    initSensorData();
    currentSensorData.timestamp = String(millis() / 1000);

    // ==================== DHT11 温湿度传感器 ====================
    float humidity, temperature;
    int dhtStatus = readDHT11(humidity, temperature);
    
    Serial.println("[传感器: DHT11]");
    if (dhtStatus == SENSOR_OK) {
        Serial.print("  湿度: ");
        Serial.print(humidity);
        Serial.println(" %");
        Serial.print("  温度: ");
        Serial.print(temperature);
        Serial.println(" °C");
        currentSensorData.temperature = temperature;
        currentSensorData.humidity = humidity;
    } else {
        allSensorsOK = false;
        anySensorFailed = true;
    }
    printSensorStatus("DHT11", dhtStatus);
    Serial.println();

    // ==================== SGP30 空气质量传感器 ====================
    uint16_t tvoc, eco2;
    int sgpStatus = readSGP30(tvoc, eco2);
    
    Serial.println("[传感器: SGP30]");
    if (sgpStatus == SENSOR_OK) {
        Serial.print("  TVOC: ");
        Serial.print(tvoc);
        Serial.println(" ppb");
        Serial.print("  eCO2: ");
        Serial.print(eco2);
        Serial.println(" ppm");
        currentSensorData.tvoc = tvoc;
        currentSensorData.eco2 = eco2;
    } else {
        allSensorsOK = false;
        anySensorFailed = true;
    }
    printSensorStatus("SGP30", sgpStatus);
    Serial.println();

    // ==================== HW-390 土壤湿度传感器 ====================
    Serial.println("[传感器: HW-390]");
    
    if (hw390Available && hw390Sensor.readData()) {
        int moisture = hw390Sensor.getPercentage();
        Serial.print("  土壤湿度: ");
        Serial.print(moisture);
        Serial.println(" %");
        Serial.print("  原始值: ");
        Serial.println(hw390Sensor.getRawValue());
        
        currentSensorData.soilMoisture = moisture;
        
        // 湿度状态判断
        if (moisture < 20) {
            Serial.println("  状态: 土壤干燥，需要浇水");
        } else if (moisture < 50) {
            Serial.println("  状态: 土壤偏干，建议浇水");
        } else if (moisture < 80) {
            Serial.println("  状态: 土壤湿度良好");
        } else {
            Serial.println("  状态: 土壤过湿，需要排水");
        }
        
        // 土壤干燥警告
        if (moisture < 50) {
            hasAbnormalCondition = true;
        }
        
        // 自动灌溉逻辑（预留）
        // if (moisture < SOIL_MOISTURE_THRESHOLD) {
        //     startIrrigation(IRRIGATION_DURATION);
        // }
    } else {
        Serial.println("  错误: 传感器读取失败");
        allSensorsOK = false;
        anySensorFailed = true;
    }
    Serial.println();

    // ==================== DS18B20 温度传感器（双传感器） ====================
    int ds18b20Status1 = ds18b20Sensor1.readData();
    int ds18b20Status2 = ds18b20Sensor2.readData();
    
    ds18b20AvgValid = false;
    ds18b20AvgTemperature = 0.0f;
    
    bool sensor1Valid = ds18b20Sensor1Available && ds18b20Sensor1.isDataValid();
    bool sensor2Valid = ds18b20Sensor2Available && ds18b20Sensor2.isDataValid();
    
    int validCount = 0;
    float tempSum = 0.0f;
    
    if (sensor1Valid) {
        tempSum += ds18b20Sensor1.getTemperature();
        validCount++;
    }
    if (sensor2Valid) {
        tempSum += ds18b20Sensor2.getTemperature();
        validCount++;
    }
    
    if (validCount > 0) {
        ds18b20AvgTemperature = tempSum / validCount;
        ds18b20AvgValid = true;
        currentSensorData.soilTemperature = ds18b20AvgTemperature;
    }
    
    Serial.println("[传感器: DS18B20]");
    
    if (ds18b20Sensor1Available) {
        Serial.print("  传感器1温度: ");
        if (sensor1Valid) {
            Serial.print(ds18b20Sensor1.getTemperature());
            Serial.println(" °C");
        } else {
            Serial.println("读取失败/无效");
        }
    }
    
    if (ds18b20Sensor2Available) {
        Serial.print("  传感器2温度: ");
        if (sensor2Valid) {
            Serial.print(ds18b20Sensor2.getTemperature());
            Serial.println(" °C");
        } else {
            Serial.println("读取失败/无效");
        }
    }
    
    if (ds18b20AvgValid) {
        Serial.print("  平均温度: ");
        Serial.print(ds18b20AvgTemperature);
        Serial.println(" °C");
    } else if (validCount == 0) {
        Serial.println("  警告: 所有传感器数据无效");
    }
    
    if (ds18b20Sensor1Available) {
        printSensorStatus("DS18B20 传感器1", ds18b20Status1);
        if (ds18b20Status1 != SENSOR_OK) {
            allSensorsOK = false;
            anySensorFailed = true;
        }
    }
    if (ds18b20Sensor2Available) {
        printSensorStatus("DS18B20 传感器2", ds18b20Status2);
        if (ds18b20Status2 != SENSOR_OK) {
            allSensorsOK = false;
            anySensorFailed = true;
        }
    }
    Serial.println();

    // ==================== HC-SR501 人体传感器 ====================
    Serial.println("[传感器: HC-SR501]");
    
    if (pirAvailable && pirSensor.readData() == SENSOR_OK) {
        if (pirSensor.isMotionDetected()) {
            Serial.println("  ⚠️ 检测到运动!");
            Serial.println("  状态: 有人存在");
            currentSensorData.motionDetected = true;
        } else {
            Serial.println("  ✓ 无运动");
            Serial.println("  状态: 无人");
            currentSensorData.motionDetected = false;
        }
    } else {
        Serial.println("  错误: 传感器读取失败");
        allSensorsOK = false;
        anySensorFailed = true;
    }
    Serial.println();

    // ==================== MH-RD 雨滴传感器 ====================
    Serial.println("[传感器: MH-RD 雨滴]");
    
    if (rainSensorAvailable && rainSensor.readData()) {
        if (rainSensor.isRainDetected()) {
            Serial.println("  ⚠️ 检测到雨水!");
            Serial.println("  状态: 传感器表面潮湿");
            currentSensorData.rainDetected = true;
        } else {
            Serial.println("  ✓ 无雨水");
            Serial.println("  状态: 传感器干燥");
            currentSensorData.rainDetected = false;
        }
    } else {
        Serial.println("  错误: 传感器读取失败");
        allSensorsOK = false;
        anySensorFailed = true;
    }
    Serial.println();

    // ==================== 光敏传感器 ====================
    Serial.println("[传感器: 光敏]");
    
    if (lightSensorAvailable && lightSensor.readData()) {
        int lightLevel = lightSensor.isLightDetected() ? 500 : 100;  // 模拟光照值
        currentSensorData.light = lightLevel;
        
        if (lightSensor.isLightDetected()) {
            Serial.println("  ✓ 检测到光线 - 环境明亮");
            Serial.print("  光照强度: ");
            Serial.println(lightLevel);
            digitalWrite(BUILTIN_LED_PIN, LOW);
        } else {
            Serial.println("  ⚠️ 未检测到光线 - 环境黑暗");
            Serial.print("  光照强度: ");
            Serial.println(lightLevel);
            hasAbnormalCondition = true;
            digitalWrite(BUILTIN_LED_PIN, HIGH);
            
            // 自动补光逻辑（预留）
            // setGrowLight(true);
        }
    } else {
        Serial.println("  错误: 传感器读取失败");
        allSensorsOK = false;
        anySensorFailed = true;
    }
    Serial.println();

    // ==================== RGB LED 状态控制 ====================
    Serial.print("[RGB LED] ");
    
    if (anySensorFailed) {
        setRed();
        Serial.println("红色 - 检测到传感器故障");
    } else if (hasAbnormalCondition) {
        setYellow();
        Serial.println("黄色 - 环境异常（黑暗或土壤干燥）");
    } else if (allSensorsOK) {
        setGreen();
        Serial.println("绿色 - 所有传感器正常");
    } else {
        setOff();
        Serial.println("关闭 - 未知状态");
    }
    Serial.println();

    // ==================== 网络数据传输 ====================
    Serial.println("[网络传输]");
    
    if (networkManager.getStatus() == NET_CONNECTED) {
        // 发送传感器数据到服务器
        bool sendResult = networkManager.sendSensorData(currentSensorData);
        if (sendResult) {
            Serial.println("  ✓ 数据发送成功");
        } else {
            Serial.println("  ⚠ 数据发送失败或未到发送时间");
        }
    } else {
        Serial.println("  ⚠ 网络未连接，跳过数据发送");
    }
    Serial.println();

    // ==================== 数据存储（预留） ====================
    // String logData = String(millis()/1000) + "," + 
    //                  String(currentSensorData.temperature) + "," + 
    //                  String(currentSensorData.humidity);
    // saveToSDCard(logData);

    // ==================== 系统状态汇总 ====================
    printSystemStatus();
    Serial.println();
}