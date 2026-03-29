#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "config.h"

// 网络状态枚举
enum NetworkStatus {
    NET_DISCONNECTED = 0,
    NET_CONNECTING = 1,
    NET_CONNECTED = 2,
    NET_ERROR = 3
};

// 传感器数据结构
struct SensorData {
    float temperature;      // 温度 (°C)
    float humidity;         // 湿度 (%)
    int light;              // 光照强度
    uint16_t tvoc;          // TVOC (ppb)
    uint16_t eco2;          // eCO2 (ppm)
    int soilMoisture;       // 土壤湿度 (%)
    float soilTemperature;  // 土壤温度 (°C)
    bool rainDetected;      // 雨滴检测
    bool motionDetected;    // 运动检测
    String timestamp;       // 时间戳
};

// 网络配置结构
struct NetworkConfig {
    String ssid;
    String password;
    String serverHost;
    int serverPort;
    String deviceId;
    String deviceName;
    String deviceType;
    String deviceLocation;
};

// 网络管理器类
class NetworkManager {
private:
    NetworkStatus status;
    NetworkConfig config;
    unsigned long lastConnectAttempt;
    unsigned long lastDataSend;
    int retryCount;
    
    // 内部方法
    bool connectWiFi();
    bool sendHttpRequest(const String& endpoint, const String& method, const String& payload = "");
    String buildSensorJson(const SensorData& data);
    String buildDeviceJson();
    String getTimestamp();
    
public:
    NetworkManager();
    
    // 初始化网络
    void begin();
    
    // 更新网络状态
    void update();
    
    // 获取网络状态
    NetworkStatus getStatus();
    
    // 发送传感器数据
    bool sendSensorData(const SensorData& data);
    
    // 注册设备
    bool registerDevice();
    
    // 获取网关状态
    bool fetchGatewayStatus();
    
    // 发送命令结果
    bool sendCommandResult(const String& requestId, const String& deviceId, 
                          const String& result, const String& message);
    
    // 断开连接
    void disconnect();
    
    // 重连
    void reconnect();
};

// 全局网络管理器实例
extern NetworkManager networkManager;

#endif // NETWORK_MODULE_H