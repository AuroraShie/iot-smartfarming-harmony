/**
 * ESP32 多传感器环境监测系统
 *
 * 当前版本特点：
 * 1. 保留原有 7 类传感器模块化结构
 * 2. main.cpp 只负责初始化、调度与数据汇总
 * 3. 本地网关接口对齐鸿蒙端，支持 HTTP + WebSocket
 * 4. 水泵与补光灯改为独立继电器模块控制
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
#include "network_module.h"
#include "pump_controller.h"
#include "grow_light_controller.h"

namespace {
const char* PUMP_DEVICE_ID = "pump-001";
const char* GROW_LIGHT_DEVICE_ID = "growlight-001";
const char* DEVICE_ON = "ON";
const char* DEVICE_OFF = "OFF";

struct ReadSummary {
    bool allSensorsOK;
    bool anySensorFailed;
    bool hasAbnormalCondition;
};
}

bool dht11Available = false;
bool sgp30Available = false;
bool hw390Available = false;
bool rainSensorAvailable = false;
bool ds18b20Sensor1Available = false;
bool ds18b20Sensor2Available = false;
bool pirAvailable = false;
bool lightSensorAvailable = false;

HW390Sensor hw390Sensor(SOIL_MOISTURE_PIN);
RainSensor rainSensor(RAIN_SENSOR_PIN);
DS18B20Sensor ds18b20Sensor1(DS18B20_PIN_1);
DS18B20Sensor ds18b20Sensor2(DS18B20_PIN_2);
PIRSensor pirSensor(PIR_PIN);
LightSensor lightSensor(LIGHT_SENSOR_PIN);

float ds18b20AvgTemperature = 0.0f;
bool ds18b20AvgValid = false;
SensorData currentSensorData;
unsigned long lastSensorReadMs = 0;

bool handleGatewayCommand(const String& deviceId, const String& command,
    int durationSec, bool hasDuration, int level, bool hasLevel,
    String& finalStatus, String& message);

void setupRGB() {
    ledcSetup(LEDC_CHANNEL_RED, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcSetup(LEDC_CHANNEL_GREEN, LEDC_FREQUENCY, LEDC_RESOLUTION);
    ledcAttachPin(RGB_RED_PIN, LEDC_CHANNEL_RED);
    ledcAttachPin(RGB_GREEN_PIN, LEDC_CHANNEL_GREEN);
    ledcWrite(LEDC_CHANNEL_RED, 0);
    ledcWrite(LEDC_CHANNEL_GREEN, 0);
}

void setRGBColor(int red, int green, int blue) {
    (void)blue;
    if (red >= 0) {
        ledcWrite(LEDC_CHANNEL_RED, constrain(red, 0, 255));
    }
    if (green >= 0) {
        ledcWrite(LEDC_CHANNEL_GREEN, constrain(green, 0, 255));
    }
}

void setRed() {
    setRGBColor(BRIGHTNESS_RED, 0, -1);
}

void setGreen() {
    setRGBColor(0, BRIGHTNESS_GREEN, -1);
}

void setYellow() {
    setRGBColor(BRIGHTNESS_YELLOW_RED, BRIGHTNESS_YELLOW_GREEN, -1);
}

void setOff() {
    setRGBColor(0, 0, -1);
}

void resetSensorData() {
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

void printSensorStatus(const char* sensorName, int status) {
    Serial.print("  ");
    Serial.print(sensorName);
    Serial.print(" 状态: ");
    switch (status) {
        case SENSOR_OK:
            Serial.println("正常");
            break;
        case SENSOR_ERROR:
            Serial.println("读取错误");
            break;
        case SENSOR_DISABLED:
            Serial.println("未启用");
            break;
        default:
            Serial.println("未知");
            break;
    }
}

void initSensors() {
    Serial.println("正在初始化传感器...");
    Serial.println();

    if (initDHT11() == SENSOR_OK) {
        dht11Available = true;
        Serial.println("DHT11 初始化成功");
    } else {
        Serial.println("DHT11 初始化失败");
    }

    if (initSGP30() == SENSOR_OK) {
        sgp30Available = true;
        Serial.println("SGP30 初始化成功");
    } else {
        Serial.println("SGP30 初始化失败");
    }

    if (initHW390() == SENSOR_OK) {
        hw390Available = true;
        Serial.println("HW-390 初始化成功");
    } else {
        Serial.println("HW-390 初始化失败");
    }

    if (ds18b20Sensor1.begin() == SENSOR_OK) {
        ds18b20Sensor1Available = true;
        Serial.println("DS18B20 传感器 1 初始化成功");
    } else {
        Serial.println("DS18B20 传感器 1 初始化失败");
    }

    if (ds18b20Sensor2.begin() == SENSOR_OK) {
        ds18b20Sensor2Available = true;
        Serial.println("DS18B20 传感器 2 初始化成功");
    } else {
        Serial.println("DS18B20 传感器 2 初始化失败");
    }

    if (pirSensor.begin() == SENSOR_OK) {
        pirAvailable = true;
        Serial.println("HC-SR501 初始化成功");
    } else {
        Serial.println("HC-SR501 初始化失败");
    }

    if (rainSensor.begin()) {
        rainSensorAvailable = true;
        Serial.println("MH-RD 雨滴传感器初始化成功");
    } else {
        Serial.println("MH-RD 雨滴传感器初始化失败");
    }

    if (lightSensor.begin()) {
        lightSensorAvailable = true;
        Serial.println("光敏传感器初始化成功");
    } else {
        Serial.println("光敏传感器初始化失败");
    }

    Serial.println();
}

void initBuiltinIndicator() {
    pinMode(BUILTIN_LED_PIN, OUTPUT);
    digitalWrite(BUILTIN_LED_PIN, LOW);
    Serial.printf("板载指示灯已配置在 GPIO%d\n", BUILTIN_LED_PIN);
}

void initActuators() {
    pumpController.begin();
    growLightController.begin();
    networkManager.setCommandHandler(handleGatewayCommand);

    Serial.printf("水泵继电器已配置在 GPIO%d\n", PUMP_RELAY_PIN);
    Serial.printf("补光继电器已配置在 GPIO%d\n", GROW_LIGHT_RELAY_PIN);
    Serial.println(RELAY_ACTIVE_LOW ? "继电器触发方式: 低电平触发" : "继电器触发方式: 高电平触发");
}

void printGatewayAccessStatus() {
    const String ip = networkManager.getLocalIp();

    Serial.println("网关访问:");
    if (networkManager.isGatewayServing() && ip.length() > 0) {
        Serial.printf("HTTP: http://%s:%d\n", ip.c_str(), networkManager.getGatewayPort());
    } else {
        Serial.printf("HTTP: 未就绪 (端口 %d)\n", networkManager.getGatewayPort());
    }

    if (!networkManager.isWebSocketEnabled()) {
        Serial.println("WebSocket: 未启用 (未编译 CONFIG_HTTPD_WS_SUPPORT)");
        return;
    }

    if (networkManager.isGatewayServing() && ip.length() > 0) {
        Serial.printf("WebSocket: ws://%s:%d%s (运行中)\n",
            ip.c_str(),
            networkManager.getGatewayPort(),
            networkManager.getWebSocketPath().c_str());
    } else {
        Serial.printf("WebSocket: 已启用，等待网络/服务就绪 (路径 %s)\n",
            networkManager.getWebSocketPath().c_str());
    }
}

void printSystemStatus() {
    Serial.println("========== 当前系统状态汇总 ==========");
    Serial.print("DHT11: ");
    Serial.println(dht11Available ? "已启用" : "未启用");
    Serial.print("SGP30: ");
    Serial.println(sgp30Available ? "已启用" : "未启用");
    Serial.print("HW-390: ");
    Serial.println(hw390Available ? "已启用" : "未启用");

    if (ds18b20Sensor1Available || ds18b20Sensor2Available) {
        Serial.print("DS18B20: ");
        if (ds18b20AvgValid) {
            Serial.printf("已启用，平均温度 %.1f C\n", ds18b20AvgTemperature);
        } else {
            Serial.println("已启用，但本轮数据无效");
        }
    } else {
        Serial.println("DS18B20: 未启用");
    }

    Serial.print("HC-SR501: ");
    Serial.println(pirAvailable ? "已启用" : "未启用");
    Serial.print("MH-RD: ");
    Serial.println(rainSensorAvailable ? "已启用" : "未启用");
    Serial.print("光敏传感器: ");
    Serial.println(lightSensorAvailable ? "已启用" : "未启用");

    Serial.print("水泵继电器: ");
    Serial.println(pumpController.isOn() ? "开启" : "关闭");
    Serial.print("补光继电器: ");
    Serial.println(growLightController.isOn() ? "开启" : "关闭");

    Serial.print("网络状态: ");
    switch (networkManager.getStatus()) {
        case NET_CONNECTED:
            Serial.println("已连接");
            break;
        case NET_CONNECTING:
            Serial.println("连接中");
            break;
        case NET_DISCONNECTED:
            Serial.println("未连接");
            break;
        case NET_ERROR:
            Serial.println("错误");
            break;
        default:
            Serial.println("未知");
            break;
    }

    printGatewayAccessStatus();
    Serial.println("==================================");
}

void markSensorFailure(ReadSummary& summary) {
    summary.allSensorsOK = false;
    summary.anySensorFailed = true;
}

void readDHT11Data(ReadSummary& summary) {
    float humidity = 0.0f;
    float temperature = 0.0f;
    const int dhtStatus = readDHT11(humidity, temperature);

    Serial.println("[传感器 DHT11]");
    if (dhtStatus == SENSOR_OK) {
        Serial.print("  湿度: ");
        Serial.print(humidity);
        Serial.println(" %");
        Serial.print("  温度: ");
        Serial.print(temperature);
        Serial.println(" C");
        currentSensorData.temperature = temperature;
        currentSensorData.humidity = humidity;
    } else {
        markSensorFailure(summary);
    }
    printSensorStatus("DHT11", dhtStatus);
    Serial.println();
}

void readAirQualityData(ReadSummary& summary) {
    uint16_t tvoc = 0;
    uint16_t eco2 = 0;
    const int sgpStatus = readSGP30(tvoc, eco2);

    Serial.println("[传感器 SGP30]");
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
        markSensorFailure(summary);
    }
    printSensorStatus("SGP30", sgpStatus);
    Serial.println();
}

void readSoilMoistureData(ReadSummary& summary) {
    Serial.println("[传感器 HW-390]");

    if (hw390Available && hw390Sensor.readData()) {
        const int moisture = hw390Sensor.getPercentage();
        Serial.print("  土壤湿度: ");
        Serial.print(moisture);
        Serial.println(" %");
        Serial.print("  原始值: ");
        Serial.println(hw390Sensor.getRawValue());

        currentSensorData.soilMoisture = moisture;

        if (moisture < 20) {
            Serial.println("  状态: 土壤干燥，需要浇水");
        } else if (moisture < 50) {
            Serial.println("  状态: 土壤偏干，建议浇水");
        } else if (moisture < 80) {
            Serial.println("  状态: 土壤湿度良好");
        } else {
            Serial.println("  状态: 土壤过湿");
        }

        if (moisture < 50) {
            summary.hasAbnormalCondition = true;
        }
    } else {
        Serial.println("  错误: 土壤湿度读取失败");
        markSensorFailure(summary);
    }
    Serial.println();
}

void readSoilTemperatureData(ReadSummary& summary) {
    const int ds18b20Status1 = ds18b20Sensor1.readData();
    const int ds18b20Status2 = ds18b20Sensor2.readData();

    ds18b20AvgValid = false;
    ds18b20AvgTemperature = 0.0f;

    const bool sensor1Valid = ds18b20Sensor1Available && ds18b20Sensor1.isDataValid();
    const bool sensor2Valid = ds18b20Sensor2Available && ds18b20Sensor2.isDataValid();

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

    Serial.println("[传感器 DS18B20]");

    if (ds18b20Sensor1Available) {
        Serial.print("  传感器 1 温度: ");
        if (sensor1Valid) {
            Serial.print(ds18b20Sensor1.getTemperature());
            Serial.println(" C");
        } else {
            Serial.println("读取失败/无效");
        }
        printSensorStatus("DS18B20 #1", ds18b20Status1);
        if (ds18b20Status1 != SENSOR_OK) {
            markSensorFailure(summary);
        }
    }

    if (ds18b20Sensor2Available) {
        Serial.print("  传感器 2 温度: ");
        if (sensor2Valid) {
            Serial.print(ds18b20Sensor2.getTemperature());
            Serial.println(" C");
        } else {
            Serial.println("读取失败/无效");
        }
        printSensorStatus("DS18B20 #2", ds18b20Status2);
        if (ds18b20Status2 != SENSOR_OK) {
            markSensorFailure(summary);
        }
    }

    if (ds18b20AvgValid) {
        Serial.print("  平均温度: ");
        Serial.print(ds18b20AvgTemperature);
        Serial.println(" C");
    } else {
        Serial.println("  警告: 本轮 DS18B20 数据无效");
    }
    Serial.println();
}

void readEnvironmentSensors(ReadSummary& summary) {
    readDHT11Data(summary);
    readAirQualityData(summary);
}

void readSoilSensors(ReadSummary& summary) {
    readSoilMoistureData(summary);
    readSoilTemperatureData(summary);
}

void readMotionData(ReadSummary& summary) {
    Serial.println("[传感器 HC-SR501]");

    if (pirAvailable && pirSensor.readData() == SENSOR_OK) {
        currentSensorData.motionDetected = pirSensor.isMotionDetected();
        if (currentSensorData.motionDetected) {
            Serial.println("  检测到人体活动");
        } else {
            Serial.println("  当前无人活动");
        }
    } else {
        Serial.println("  错误: 人体红外读取失败");
        markSensorFailure(summary);
    }
    Serial.println();
}

void readRainData(ReadSummary& summary) {
    Serial.println("[传感器 MH-RD 雨滴]");

    if (rainSensorAvailable && rainSensor.readData()) {
        currentSensorData.rainDetected = rainSensor.isRainDetected();
        if (currentSensorData.rainDetected) {
            Serial.println("  检测到雨滴或传感器表面潮湿");
        } else {
            Serial.println("  未检测到雨滴");
        }
    } else {
        Serial.println("  错误: 雨滴传感器读取失败");
        markSensorFailure(summary);
    }
    Serial.println();
}

void readLightData(ReadSummary& summary) {
    Serial.println("[传感器 光敏]");

    if (lightSensorAvailable && lightSensor.readData()) {
        const bool lightDetected = lightSensor.isLightDetected();
        const int lightLevel = lightDetected ? 500 : 100;
        currentSensorData.light = lightLevel;

        if (lightDetected) {
            Serial.println("  检测到光线，环境明亮");
            digitalWrite(BUILTIN_LED_PIN, LOW);
        } else {
            Serial.println("  未检测到光线，环境偏暗");
            digitalWrite(BUILTIN_LED_PIN, HIGH);
            summary.hasAbnormalCondition = true;
        }

        Serial.print("  光照强度(映射值): ");
        Serial.println(lightLevel);
    } else {
        Serial.println("  错误: 光敏传感器读取失败");
        markSensorFailure(summary);
    }
    Serial.println();
}

void readStateSensors(ReadSummary& summary) {
    readMotionData(summary);
    readRainData(summary);
    readLightData(summary);
}

void updateIndicatorLights(const ReadSummary& summary) {
    Serial.print("[RGB LED] ");
    if (summary.anySensorFailed) {
        setRed();
        Serial.println("红色，存在传感器故障");
    } else if (summary.hasAbnormalCondition) {
        setYellow();
        Serial.println("黄色，存在环境异常");
    } else if (summary.allSensorsOK) {
        setGreen();
        Serial.println("绿色，系统运行正常");
    } else {
        setOff();
        Serial.println("关闭，状态未知");
    }
    Serial.println();
}

void runAutoControlHooks() {
    if (!AUTO_CONTROL_ENABLED) {
        return;
    }

    // 当前版本默认关闭自动控制。
    // 后续如需开启，可在这里补充土壤湿度联动水泵、光照联动补光的规则。
}

void syncNetworkSnapshot() {
    networkManager.updateActuatorState(
        pumpController.isOn(),
        growLightController.isOn(),
        growLightController.getLevel());

    Serial.println("[网络传输]");
    if (networkManager.getStatus() == NET_CONNECTED) {
        const bool sendResult = networkManager.sendSensorData(currentSensorData);
        if (sendResult) {
            Serial.println("  本地网关快照已更新");
        } else {
            Serial.println("  本地网关快照更新失败");
        }
    } else {
        Serial.println("  网络未连接，跳过本地网关快照更新");
    }
    Serial.println();
}

void printLoopHeader() {
    Serial.println("----------------------------------");
    Serial.print("运行时间: ");
    Serial.print(millis() / 1000);
    Serial.println(" 秒");
    Serial.println();
}

void handleLoopTasks() {
    const unsigned long nowMs = millis();
    if (nowMs - lastSensorReadMs < SENSOR_READ_INTERVAL) {
        return;
    }
    lastSensorReadMs = nowMs;

    printLoopHeader();
    resetSensorData();
    currentSensorData.timestamp = String(nowMs / 1000);

    ReadSummary summary = {true, false, false};
    readEnvironmentSensors(summary);
    readSoilSensors(summary);
    readStateSensors(summary);
    runAutoControlHooks();
    updateIndicatorLights(summary);
    syncNetworkSnapshot();
    printSystemStatus();
    Serial.println();
}

bool handleGatewayCommand(const String& deviceId, const String& command,
    int durationSec, bool hasDuration, int level, bool hasLevel,
    String& finalStatus, String& message) {
    if (deviceId == PUMP_DEVICE_ID) {
        if (command == "TURN_ON") {
            const uint32_t safeDuration = hasDuration
                ? static_cast<uint32_t>(durationSec)
                : static_cast<uint32_t>(DEFAULT_PUMP_DURATION_SEC);
            pumpController.turnOn(safeDuration);
            finalStatus = DEVICE_ON;
            if (safeDuration > 0) {
                message = String("水泵已开启，持续 ") + String(safeDuration) + String(" 秒");
            } else {
                message = "水泵已开启";
            }
        } else if (command == "TURN_OFF") {
            pumpController.turnOff();
            finalStatus = DEVICE_OFF;
            message = "水泵已关闭";
        } else {
            finalStatus = pumpController.isOn() ? DEVICE_ON : DEVICE_OFF;
            message = "不支持的水泵命令";
            return false;
        }
    } else if (deviceId == GROW_LIGHT_DEVICE_ID) {
        if (command == "TURN_ON") {
            const uint8_t targetLevel = hasLevel ? static_cast<uint8_t>(level) : DEFAULT_GROW_LIGHT_LEVEL;
            growLightController.turnOn(targetLevel);
            finalStatus = DEVICE_ON;
            message = "补光继电器已开启";
        } else if (command == "TURN_OFF") {
            growLightController.turnOff();
            finalStatus = DEVICE_OFF;
            message = "补光继电器已关闭";
        } else if (command == "SET_LEVEL") {
            const uint8_t targetLevel = hasLevel ? static_cast<uint8_t>(level) : DEFAULT_GROW_LIGHT_LEVEL;
            growLightController.turnOn(targetLevel);
            finalStatus = DEVICE_ON;
            message = String("补光继电器已开启，等级记录为 ") + String(targetLevel);
        } else {
            finalStatus = growLightController.isOn() ? DEVICE_ON : DEVICE_OFF;
            message = "不支持的补光命令";
            return false;
        }
    } else {
        finalStatus = "UNKNOWN";
        message = "未找到对应设备";
        return false;
    }

    networkManager.updateActuatorState(
        pumpController.isOn(),
        growLightController.isOn(),
        growLightController.getLevel());
    return true;
}

void setup() {
    Serial.begin(SERIAL_BAUD_RATE);
    delay(100);

    Serial.println("==================================");
    Serial.println("ESP32 多传感器环境监测系统");
    Serial.println("版本: 3.0 (本地网关 + 继电器执行器版本)");
    Serial.println("==================================");
    Serial.println();

    resetSensorData();
    initSensors();
    initBuiltinIndicator();
    setupRGB();
    Serial.println("RGB 状态灯初始化完成");
    initActuators();
    networkManager.begin();
    printGatewayAccessStatus();

    Serial.println();
    Serial.println("==================================");
    Serial.println("系统就绪，开始采集与同步数据...");
    Serial.println("==================================");
    Serial.println();
}

void loop() {
    networkManager.update();
    pumpController.update();
    networkManager.updateActuatorState(
        pumpController.isOn(),
        growLightController.isOn(),
        growLightController.getLevel());
    handleLoopTasks();
}
