#include "network_module.h"
#include <time.h>

// 全局网络管理器实例
NetworkManager networkManager;

// NTP服务器配置
const char* ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 8 * 3600;  // 东八区
const int daylightOffset_sec = 0;

NetworkManager::NetworkManager() {
    status = NET_DISCONNECTED;
    lastConnectAttempt = 0;
    lastDataSend = 0;
    retryCount = 0;
    
    // 初始化配置
    config.ssid = WIFI_SSID;
    config.password = WIFI_PASSWORD;
    config.serverHost = SERVER_HOST;
    config.serverPort = SERVER_PORT;
    config.deviceId = DEVICE_ID;
    config.deviceName = DEVICE_NAME;
    config.deviceType = DEVICE_TYPE;
    config.deviceLocation = DEVICE_LOCATION;
}

void NetworkManager::begin() {
    Serial.println("初始化网络模块...");
    
    // 配置NTP时间
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
    
    // 尝试连接WiFi
    if (connectWiFi()) {
        status = NET_CONNECTED;
        Serial.println("网络初始化成功");
        Serial.print("IP地址: ");
        Serial.println(WiFi.localIP());
        
        // 注册设备到服务器
        registerDevice();
    } else {
        status = NET_DISCONNECTED;
        Serial.println("网络初始化失败，将在后台继续尝试连接");
    }
}

void NetworkManager::update() {
    unsigned long currentTime = millis();
    
    // 检查WiFi连接状态
    if (status == NET_CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("WiFi连接断开");
            status = NET_DISCONNECTED;
        }
    }
    
    // 如果未连接且超过重试间隔，尝试重连
    if (status == NET_DISCONNECTED && 
        currentTime - lastConnectAttempt >= WIFI_RETRY_INTERVAL) {
        lastConnectAttempt = currentTime;
        if (connectWiFi()) {
            status = NET_CONNECTED;
            retryCount = 0;
            Serial.println("WiFi重新连接成功");
            
            // 重新注册设备
            registerDevice();
        } else {
            retryCount++;
            Serial.print("WiFi连接失败，重试次数: ");
            Serial.println(retryCount);
            
            if (retryCount >= MAX_RETRY_COUNT) {
                status = NET_ERROR;
                Serial.println("达到最大重试次数，网络进入错误状态");
            }
        }
    }
}

NetworkStatus NetworkManager::getStatus() {
    return status;
}

bool NetworkManager::connectWiFi() {
    Serial.print("正在连接WiFi: ");
    Serial.println(config.ssid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
           millis() - startTime < WIFI_CONNECT_TIMEOUT) {
        delay(500);
        Serial.print(".");
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println();
        Serial.print("WiFi连接成功，IP地址: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println();
        Serial.println("WiFi连接失败");
        return false;
    }
}

String NetworkManager::getTimestamp() {
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
        // 如果无法获取NTP时间，使用系统运行时间
        return String(millis() / 1000);
    }
    
    char buffer[30];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &timeinfo);
    return String(buffer);
}

String NetworkManager::buildSensorJson(const SensorData& data) {
    // 创建JSON文档
    StaticJsonDocument<512> doc;
    
    // 添加传感器数据
    doc["temperature"] = data.temperature;
    doc["humidity"] = data.humidity;
    doc["light"] = data.light;
    doc["tvoc"] = data.tvoc;
    doc["eco2"] = data.eco2;
    doc["soilMoisture"] = data.soilMoisture;
    doc["soilTemperature"] = data.soilTemperature;
    doc["rainDetected"] = data.rainDetected;
    doc["motionDetected"] = data.motionDetected;
    doc["timestamp"] = data.timestamp;
    
    // 序列化JSON
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

String NetworkManager::buildDeviceJson() {
    // 创建JSON文档
    StaticJsonDocument<256> doc;
    
    // 添加设备信息
    doc["id"] = config.deviceId;
    doc["name"] = config.deviceName;
    doc["type"] = config.deviceType;
    doc["location"] = config.deviceLocation;
    doc["online"] = true;
    doc["status"] = "就绪";
    
    // 序列化JSON
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool NetworkManager::sendHttpRequest(const String& endpoint, const String& method, const String& payload) {
    if (status != NET_CONNECTED) {
        Serial.println("网络未连接，无法发送HTTP请求");
        return false;
    }
    
    HTTPClient http;
    String url = String(SERVER_PROTOCOL) + "://" + config.serverHost + ":" + String(config.serverPort) + endpoint;
    
    Serial.print("发送HTTP请求: ");
    Serial.print(method);
    Serial.print(" ");
    Serial.println(url);
    
    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(HTTP_TIMEOUT);
    
    int httpCode;
    if (method == "GET") {
        httpCode = http.GET();
    } else if (method == "POST") {
        httpCode = http.POST(payload);
    } else {
        Serial.println("不支持的HTTP方法");
        http.end();
        return false;
    }
    
    if (httpCode > 0) {
        Serial.print("HTTP响应码: ");
        Serial.println(httpCode);
        
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_CREATED) {
            String response = http.getString();
            Serial.print("响应内容: ");
            Serial.println(response);
            http.end();
            return true;
        } else {
            Serial.print("HTTP请求失败，错误码: ");
            Serial.println(httpCode);
            http.end();
            return false;
        }
    } else {
        Serial.print("HTTP请求错误: ");
        Serial.println(http.errorToString(httpCode));
        http.end();
        return false;
    }
}

bool NetworkManager::sendSensorData(const SensorData& data) {
    unsigned long currentTime = millis();
    
    // 检查发送间隔
    if (currentTime - lastDataSend < DATA_SEND_INTERVAL) {
        return false;
    }
    
    // 构建传感器数据JSON
    String jsonPayload = buildSensorJson(data);
    
    // 发送遥测数据到服务器
    bool success = sendHttpRequest("/telemetry/realtime", "POST", jsonPayload);
    
    if (success) {
        lastDataSend = currentTime;
        Serial.println("传感器数据发送成功");
        return true;
    } else {
        Serial.println("传感器数据发送失败");
        return false;
    }
}

bool NetworkManager::registerDevice() {
    Serial.println("正在注册设备到服务器...");
    
    // 构建设备信息JSON
    String jsonPayload = buildDeviceJson();
    
    // 发送设备注册请求
    bool success = sendHttpRequest("/devices", "POST", jsonPayload);
    
    if (success) {
        Serial.println("设备注册成功");
        return true;
    } else {
        Serial.println("设备注册失败");
        return false;
    }
}

bool NetworkManager::fetchGatewayStatus() {
    Serial.println("正在获取网关状态...");
    
    // 发送获取网关状态请求
    bool success = sendHttpRequest("/gateway/status", "GET");
    
    if (success) {
        Serial.println("获取网关状态成功");
        return true;
    } else {
        Serial.println("获取网关状态失败");
        return false;
    }
}

bool NetworkManager::sendCommandResult(const String& requestId, const String& deviceId, 
                                      const String& result, const String& message) {
    Serial.println("正在发送命令执行结果...");
    
    // 创建JSON文档
    StaticJsonDocument<256> doc;
    
    // 添加命令结果
    doc["requestId"] = requestId;
    doc["deviceId"] = deviceId;
    doc["result"] = result;
    doc["message"] = message;
    doc["timestamp"] = getTimestamp();
    
    // 序列化JSON
    String jsonPayload;
    serializeJson(doc, jsonPayload);
    
    // 发送命令结果到服务器
    String endpoint = "/commands/" + requestId;
    bool success = sendHttpRequest(endpoint, "POST", jsonPayload);
    
    if (success) {
        Serial.println("命令结果发送成功");
        return true;
    } else {
        Serial.println("命令结果发送失败");
        return false;
    }
}

void NetworkManager::disconnect() {
    Serial.println("正在断开网络连接...");
    WiFi.disconnect();
    status = NET_DISCONNECTED;
    Serial.println("网络连接已断开");
}

void NetworkManager::reconnect() {
    Serial.println("正在重新连接网络...");
    disconnect();
    delay(1000);
    begin();
}