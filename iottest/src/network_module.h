#ifndef NETWORK_MODULE_H
#define NETWORK_MODULE_H

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <esp_http_server.h>
#include "config.h"

enum NetworkStatus {
    NET_DISCONNECTED = 0,
    NET_CONNECTING = 1,
    NET_CONNECTED = 2,
    NET_ERROR = 3
};

struct SensorData {
    float temperature;
    float humidity;
    int light;
    uint16_t tvoc;
    uint16_t eco2;
    int soilMoisture;
    float soilTemperature;
    bool rainDetected;
    bool motionDetected;
    String timestamp;
};

struct NetworkConfig {
    String ssid;
    String password;
    String gatewayId;
    String deviceName;
    String deviceLocation;
    String firmwareVersion;
    String wsPath;
    int gatewayPort;
};

using DeviceCommandHandler = bool (*)(const String& deviceId, const String& command,
    int durationSec, bool hasDuration, int level, bool hasLevel,
    String& finalStatus, String& message);

class NetworkManager {
private:
    struct CommandRecord {
        bool used;
        String requestId;
        String deviceId;
        String result;
        String finalStatus;
        String message;
        String timestamp;
        unsigned long createdAtMs;
    };

    struct ActuatorState {
        bool pumpOn;
        bool growLightOn;
        int growLightLevel;
    };

    NetworkStatus status;
    NetworkConfig config;
    SensorData latestSensorData;
    bool sensorDataReady;
    unsigned long lastConnectAttempt;
    unsigned long bootTimeMs;
    unsigned long lastHeartbeatMs;
    unsigned long lastTelemetryPublishMs;
    unsigned long messageSequence;
    String lastHeartbeatIso;
    httpd_handle_t serverHandle;
    ActuatorState actuatorState;
    CommandRecord commandHistory[MAX_COMMAND_HISTORY];
    DeviceCommandHandler commandHandler;

    void configureTime();
    bool connectWiFi();
    bool startServer();
    void stopServer();
    void refreshHeartbeat(bool forcePublish = false);
    bool isServerRunning() const;
    String createTimestamp() const;
    String createMessageId();
    String readRequestBody(httpd_req_t* req) const;
    void sendJson(httpd_req_t* req, const String& payload, const char* status = "200 OK") const;
    String buildGatewayStatusResponse() const;
    String buildTelemetryResponse() const;
    String buildDevicesResponse() const;
    String buildCommandAcceptedResponse(const String& requestId, const String& deviceId, bool accepted,
        const String& statusText, const String& message, int code) const;
    String buildCommandQueryResponse(const CommandRecord& record) const;
    void appendTelemetry(JsonObject data) const;
    void appendSensorDevice(JsonArray devices, const char* id, const char* name, const char* type,
        const char* metricType, float value, const char* unit) const;
    void appendActuatorDevice(JsonArray devices, const char* id, const char* name, const char* type,
        bool online, const String& statusText, int level, bool supportLevel) const;
    int findCommandRecord(const String& requestId) const;
    CommandRecord& writeCommandRecord(const String& requestId, const String& deviceId, const String& result,
        const String& finalStatus, const String& message);
    void publishTelemetryUpdate(bool force);
    void publishGatewayStatusChanged();
    void publishDeviceStatusChanged(const String& deviceId);
    void publishCommandResult(const CommandRecord& record);
    void broadcastWsMessage(const String& payload);
    bool handleDeviceCommand(const String& deviceId, const String& command, JsonVariant params,
        String& finalStatus, String& message);
    void syncCommandState(const String& deviceId, const String& finalStatus, int level);
    static NetworkManager& instance();
    static esp_err_t handleGatewayStatus(httpd_req_t* req);
    static esp_err_t handleRealtimeTelemetry(httpd_req_t* req);
    static esp_err_t handleDevices(httpd_req_t* req);
    static esp_err_t handleDeviceCommandRequest(httpd_req_t* req);
    static esp_err_t handleCommandQuery(httpd_req_t* req);
    static esp_err_t handleWebSocket(httpd_req_t* req);

public:
    NetworkManager();

    void begin();
    void update();
    NetworkStatus getStatus() const;
    String getLocalIp() const;
    int getGatewayPort() const;
    String getWebSocketPath() const;
    bool isWebSocketEnabled() const;
    bool isGatewayServing() const;
    bool sendSensorData(const SensorData& data);
    bool registerDevice();
    bool fetchGatewayStatus();
    bool sendCommandResult(const String& requestId, const String& deviceId,
        const String& result, const String& message);
    void disconnect();
    void reconnect();
    void setCommandHandler(DeviceCommandHandler handler);
    void updateActuatorState(bool pumpOn, bool growLightOn, int growLightLevel);
};

extern NetworkManager networkManager;

#endif
