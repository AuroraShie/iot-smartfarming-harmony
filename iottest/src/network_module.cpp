#include "network_module.h"

#include <limits.h>
#include <time.h>

NetworkManager networkManager;

namespace {
const char* NTP_SERVER_1 = "pool.ntp.org";
const char* NTP_SERVER_2 = "ntp.aliyun.com";
const char* PUMP_DEVICE_ID = "pump-001";
const char* GROW_LIGHT_DEVICE_ID = "growlight-001";
const char* DEVICE_ON = "ON";
const char* DEVICE_OFF = "OFF";
const time_t MIN_VALID_EPOCH = 1700000000;
const time_t FALLBACK_EPOCH = 1704067200;
}

NetworkManager::NetworkManager()
    : status(NET_DISCONNECTED),
      sensorDataReady(false),
      lastConnectAttempt(0),
      bootTimeMs(0),
      lastHeartbeatMs(0),
      lastTelemetryPublishMs(0),
      messageSequence(0),
      serverHandle(NULL) {
    config.ssid = WIFI_SSID;
    config.password = WIFI_PASSWORD;
    config.gatewayId = GATEWAY_ID;
    config.deviceName = GATEWAY_NAME;
    config.deviceLocation = GATEWAY_LOCATION;
    config.firmwareVersion = GATEWAY_FIRMWARE_VERSION;
    config.wsPath = GATEWAY_WS_PATH;
    config.gatewayPort = GATEWAY_PORT;

    latestSensorData.temperature = 0.0f;
    latestSensorData.humidity = 0.0f;
    latestSensorData.light = 0;
    latestSensorData.tvoc = 0;
    latestSensorData.eco2 = 0;
    latestSensorData.soilMoisture = 0;
    latestSensorData.soilTemperature = 0.0f;
    latestSensorData.rainDetected = false;
    latestSensorData.motionDetected = false;
    latestSensorData.timestamp = "";

    actuatorState.pumpOn = false;
    actuatorState.growLightOn = false;
    actuatorState.growLightLevel = 0;
    actuatorState.pumpAutoOffAtMs = 0;

    for (int i = 0; i < MAX_COMMAND_HISTORY; ++i) {
        commandHistory[i].used = false;
        commandHistory[i].createdAtMs = 0;
    }
}

void NetworkManager::begin() {
    Serial.println("Starting gateway network module...");
    bootTimeMs = millis();
    status = NET_CONNECTING;
    configureTime();

    if (!connectWiFi()) {
        status = NET_DISCONNECTED;
        Serial.println("Wi-Fi is not ready. The gateway will retry in background.");
        return;
    }

    if (!startServer()) {
        status = NET_ERROR;
        Serial.println("Failed to start the local gateway server.");
        return;
    }

    refreshHeartbeat(true);
    status = NET_CONNECTED;
    Serial.print("Gateway is serving at http://");
    Serial.print(WiFi.localIP());
    Serial.print(":");
    Serial.println(config.gatewayPort);
}

void NetworkManager::update() {
    const unsigned long nowMs = millis();
    processAutoControl();

    if (WiFi.status() != WL_CONNECTED) {
        if (status == NET_CONNECTED) {
            Serial.println("Wi-Fi connection lost, stopping gateway server.");
            stopServer();
            status = NET_DISCONNECTED;
        }
    } else if (!isServerRunning()) {
        if (!startServer()) {
            status = NET_ERROR;
        } else {
            status = NET_CONNECTED;
            refreshHeartbeat(true);
        }
    }

    if ((status == NET_DISCONNECTED || status == NET_ERROR) &&
        nowMs - lastConnectAttempt >= WIFI_RETRY_INTERVAL) {
        lastConnectAttempt = nowMs;
        status = NET_CONNECTING;
        if (connectWiFi() && startServer()) {
            status = NET_CONNECTED;
            refreshHeartbeat(true);
            Serial.println("Gateway network recovered.");
        } else {
            status = NET_DISCONNECTED;
        }
    }

    if (status == NET_CONNECTED && nowMs - lastHeartbeatMs >= HEARTBEAT_INTERVAL_MS) {
        refreshHeartbeat(true);
    }
}

NetworkStatus NetworkManager::getStatus() const {
    return status;
}

bool NetworkManager::sendSensorData(const SensorData& data) {
    latestSensorData = data;
    latestSensorData.timestamp = createTimestamp();
    sensorDataReady = true;

    if (status != NET_CONNECTED) {
        return false;
    }

    refreshHeartbeat(false);
    publishTelemetryUpdate(false);
    return true;
}

bool NetworkManager::registerDevice() {
    return status == NET_CONNECTED;
}

bool NetworkManager::fetchGatewayStatus() {
    return status == NET_CONNECTED;
}

bool NetworkManager::sendCommandResult(const String& requestId, const String& deviceId,
    const String& result, const String& message) {
    CommandRecord& record = writeCommandRecord(requestId, deviceId, result, result, message);
    publishCommandResult(record);
    return true;
}

void NetworkManager::disconnect() {
    stopServer();
    WiFi.disconnect(true);
    status = NET_DISCONNECTED;
}

void NetworkManager::reconnect() {
    disconnect();
    delay(500);
    begin();
}

void NetworkManager::configureTime() {
    configTime(0, 0, NTP_SERVER_1, NTP_SERVER_2);
}

bool NetworkManager::connectWiFi() {
    if (WiFi.status() == WL_CONNECTED) {
        return true;
    }

    Serial.print("Connecting to Wi-Fi SSID: ");
    Serial.println(config.ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(config.ssid.c_str(), config.password.c_str());

    const unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startMs < WIFI_CONNECT_TIMEOUT) {
        delay(250);
        Serial.print(".");
    }
    Serial.println();

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Wi-Fi connection failed.");
        return false;
    }

    Serial.print("Wi-Fi connected, IP: ");
    Serial.println(WiFi.localIP());
    return true;
}

bool NetworkManager::startServer() {
    if (serverHandle != NULL) {
        return true;
    }

    httpd_config_t httpConfig = HTTPD_DEFAULT_CONFIG();
    httpConfig.server_port = static_cast<uint16_t>(config.gatewayPort);
    httpConfig.ctrl_port = static_cast<uint16_t>(config.gatewayPort + 1000);
    httpConfig.max_uri_handlers = 8;
    httpConfig.max_open_sockets = MAX_WS_CLIENTS;
    httpConfig.lru_purge_enable = true;
    httpConfig.uri_match_fn = httpd_uri_match_wildcard;

    if (httpd_start(&serverHandle, &httpConfig) != ESP_OK) {
        serverHandle = NULL;
        return false;
    }

    httpd_uri_t gatewayStatusUri = {};
    gatewayStatusUri.uri = "/gateway/status";
    gatewayStatusUri.method = HTTP_GET;
    gatewayStatusUri.handler = NetworkManager::handleGatewayStatus;

    httpd_uri_t telemetryUri = {};
    telemetryUri.uri = "/telemetry/realtime";
    telemetryUri.method = HTTP_GET;
    telemetryUri.handler = NetworkManager::handleRealtimeTelemetry;

    httpd_uri_t devicesUri = {};
    devicesUri.uri = "/devices";
    devicesUri.method = HTTP_GET;
    devicesUri.handler = NetworkManager::handleDevices;

    httpd_uri_t commandUri = {};
    commandUri.uri = "/devices/*/command";
    commandUri.method = HTTP_POST;
    commandUri.handler = NetworkManager::handleDeviceCommandRequest;

    httpd_uri_t commandQueryUri = {};
    commandQueryUri.uri = "/commands/*";
    commandQueryUri.method = HTTP_GET;
    commandQueryUri.handler = NetworkManager::handleCommandQuery;

    httpd_uri_t wsUri = {};
    wsUri.uri = config.wsPath.c_str();
    wsUri.method = HTTP_GET;
    wsUri.handler = NetworkManager::handleWebSocket;
#ifdef CONFIG_HTTPD_WS_SUPPORT
    wsUri.is_websocket = true;
    wsUri.handle_ws_control_frames = true;
    wsUri.supported_subprotocol = NULL;
#endif

    bool ok = httpd_register_uri_handler(serverHandle, &gatewayStatusUri) == ESP_OK
        && httpd_register_uri_handler(serverHandle, &telemetryUri) == ESP_OK
        && httpd_register_uri_handler(serverHandle, &devicesUri) == ESP_OK
        && httpd_register_uri_handler(serverHandle, &commandUri) == ESP_OK
        && httpd_register_uri_handler(serverHandle, &commandQueryUri) == ESP_OK
        && httpd_register_uri_handler(serverHandle, &wsUri) == ESP_OK;

    if (!ok) {
        stopServer();
        return false;
    }

    return true;
}

void NetworkManager::stopServer() {
    if (serverHandle != NULL) {
        httpd_stop(serverHandle);
        serverHandle = NULL;
    }
}

void NetworkManager::refreshHeartbeat(bool forcePublish) {
    lastHeartbeatMs = millis();
    lastHeartbeatIso = createTimestamp();
    if (forcePublish) {
        publishGatewayStatusChanged();
    }
}

bool NetworkManager::isServerRunning() const {
    return serverHandle != NULL;
}

String NetworkManager::createTimestamp() const {
    time_t nowValue = time(NULL);
    if (nowValue < MIN_VALID_EPOCH) {
        nowValue = FALLBACK_EPOCH + static_cast<time_t>(millis() / 1000);
    }

    struct tm timeinfo;
    gmtime_r(&nowValue, &timeinfo);

    char buffer[25];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%SZ", &timeinfo);
    return String(buffer);
}

String NetworkManager::createMessageId() {
    messageSequence++;
    return String("msg-") + String(millis()) + "-" + String(messageSequence);
}

String NetworkManager::readRequestBody(httpd_req_t* req) const {
    String body;
    if (req->content_len <= 0) {
        return body;
    }

    body.reserve(req->content_len);
    int remaining = req->content_len;
    char buffer[129];

    while (remaining > 0) {
        const int received = httpd_req_recv(req, buffer, remaining < static_cast<int>(sizeof(buffer)) ? remaining :
            static_cast<int>(sizeof(buffer) - 1));
        if (received <= 0) {
            return String();
        }
        buffer[received] = '\0';
        body.concat(buffer);
        remaining -= received;
    }

    return body;
}

void NetworkManager::sendJson(httpd_req_t* req, const String& payload, const char* statusText) const {
    httpd_resp_set_status(req, statusText);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_send(req, payload.c_str(), payload.length());
}

String NetworkManager::buildGatewayStatusResponse() const {
    DynamicJsonDocument doc(768);
    doc["code"] = 0;
    doc["message"] = "ok";

    JsonObject data = doc.createNestedObject("data");
    data["gatewayId"] = config.gatewayId;
    data["online"] = status == NET_CONNECTED;
    data["lastHeartbeat"] = lastHeartbeatIso.length() > 0 ? lastHeartbeatIso : createTimestamp();
    data["firmwareVersion"] = config.firmwareVersion;
    data["connectedDeviceCount"] = 11;

    JsonObject wifi = data.createNestedObject("wifi");
    wifi["ssid"] = config.ssid;
    wifi["ip"] = WiFi.localIP().toString();
    wifi["rssi"] = WiFi.RSSI();

    data["uptimeSec"] = static_cast<uint32_t>((millis() - bootTimeMs) / 1000);

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String NetworkManager::buildTelemetryResponse() const {
    DynamicJsonDocument doc(768);
    doc["code"] = 0;
    doc["message"] = "ok";
    JsonObject data = doc.createNestedObject("data");
    appendTelemetry(data);

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String NetworkManager::buildDevicesResponse() const {
    DynamicJsonDocument doc(8192);
    doc["code"] = 0;
    doc["message"] = "ok";
    JsonArray devices = doc.createNestedArray("data");

    appendSensorDevice(devices, "sensor-temp-001", "Air Temperature Sensor", "SENSOR",
        "TEMPERATURE", latestSensorData.temperature, "C");
    appendSensorDevice(devices, "sensor-humidity-001", "Air Humidity Sensor", "SENSOR",
        "HUMIDITY", latestSensorData.humidity, "%");
    appendSensorDevice(devices, "sensor-light-001", "Ambient Light Sensor", "LIGHT_SENSOR",
        "LIGHT", latestSensorData.light, "lux");
    appendSensorDevice(devices, "sensor-co2-001", "CO2 Sensor", "CO2_SENSOR",
        "CO2", latestSensorData.eco2, "ppm");
    appendSensorDevice(devices, "sensor-tvoc-001", "TVOC Sensor", "TVOC_SENSOR",
        "TVOC", latestSensorData.tvoc, "ppb");
    appendSensorDevice(devices, "sensor-soil-001", "Soil Moisture Sensor", "SOIL_SENSOR",
        "SOIL_MOISTURE", latestSensorData.soilMoisture, "%");
    appendSensorDevice(devices, "sensor-soil-temp-001", "Soil Temperature Sensor", "SOIL_SENSOR",
        "SOIL_TEMPERATURE", latestSensorData.soilTemperature, "C");
    appendSensorDevice(devices, "sensor-rain-001", "Rain Sensor", "RAIN_SENSOR",
        "RAIN_DETECTED", latestSensorData.rainDetected ? 1.0f : 0.0f, "bool");
    appendSensorDevice(devices, "sensor-motion-001", "Motion Sensor", "MOTION_SENSOR",
        "MOTION_DETECTED", latestSensorData.motionDetected ? 1.0f : 0.0f, "bool");

    appendActuatorDevice(devices, PUMP_DEVICE_ID, "Irrigation Pump A1", "PUMP", true,
        actuatorState.pumpOn ? DEVICE_ON : DEVICE_OFF, actuatorState.pumpOn ? 1 : 0, false);
    appendActuatorDevice(devices, GROW_LIGHT_DEVICE_ID, "Grow Light A1", "LIGHT", true,
        actuatorState.growLightOn ? DEVICE_ON : DEVICE_OFF, actuatorState.growLightLevel, true);

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String NetworkManager::buildCommandAcceptedResponse(const String& requestId, const String& deviceId, bool accepted,
    const String& statusText, const String& message, int code) const {
    DynamicJsonDocument doc(512);
    doc["code"] = code;
    doc["message"] = message;
    doc["requestId"] = requestId;

    JsonObject data = doc.createNestedObject("data");
    data["accepted"] = accepted;
    data["deviceId"] = deviceId;
    data["status"] = statusText;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

String NetworkManager::buildCommandQueryResponse(const CommandRecord& record) const {
    DynamicJsonDocument doc(512);
    doc["code"] = 0;
    doc["message"] = "ok";
    doc["requestId"] = record.requestId;

    JsonObject data = doc.createNestedObject("data");
    data["requestId"] = record.requestId;
    data["deviceId"] = record.deviceId;
    data["result"] = record.result;
    data["finalStatus"] = record.finalStatus;
    data["message"] = record.message;
    data["timestamp"] = record.timestamp;

    String payload;
    serializeJson(doc, payload);
    return payload;
}

void NetworkManager::appendTelemetry(JsonObject data) const {
    data["temperature"] = latestSensorData.temperature;
    data["humidity"] = latestSensorData.humidity;
    data["light"] = latestSensorData.light;
    data["co2"] = latestSensorData.eco2;
    data["eco2"] = latestSensorData.eco2;
    data["tvoc"] = latestSensorData.tvoc;
    data["soilMoisture"] = latestSensorData.soilMoisture;
    data["soilTemperature"] = latestSensorData.soilTemperature;
    data["rainDetected"] = latestSensorData.rainDetected;
    data["motionDetected"] = latestSensorData.motionDetected;
    data["timestamp"] = sensorDataReady ? latestSensorData.timestamp : createTimestamp();
}

void NetworkManager::appendSensorDevice(JsonArray devices, const char* id, const char* name, const char* type,
    const char* metricType, float value, const char* unit) const {
    JsonObject device = devices.createNestedObject();
    device["id"] = id;
    device["gatewayId"] = config.gatewayId;
    device["name"] = name;
    device["type"] = type;
    device["online"] = true;
    device["status"] = "READY";
    device["location"] = config.deviceLocation;
    device["updatedAt"] = sensorDataReady ? latestSensorData.timestamp : createTimestamp();
    device.createNestedArray("capabilities");

    JsonObject telemetry = device.createNestedObject("telemetry");
    telemetry["metricType"] = metricType;
    telemetry["value"] = value;
    telemetry["unit"] = unit;
    telemetry["timestamp"] = sensorDataReady ? latestSensorData.timestamp : createTimestamp();
}

void NetworkManager::appendActuatorDevice(JsonArray devices, const char* id, const char* name, const char* type,
    bool online, const String& statusText, int level, bool supportLevel) const {
    JsonObject device = devices.createNestedObject();
    device["id"] = id;
    device["gatewayId"] = config.gatewayId;
    device["name"] = name;
    device["type"] = type;
    device["online"] = online;
    device["status"] = statusText;
    if (supportLevel) {
        device["level"] = level;
    } else {
        device["level"] = statusText == DEVICE_ON ? 1 : 0;
    }
    device["location"] = config.deviceLocation;
    device["updatedAt"] = createTimestamp();

    JsonArray capabilities = device.createNestedArray("capabilities");
    capabilities.add("TURN_ON");
    capabilities.add("TURN_OFF");
    if (supportLevel) {
        capabilities.add("SET_LEVEL");
    }
}

int NetworkManager::findCommandRecord(const String& requestId) const {
    for (int i = 0; i < MAX_COMMAND_HISTORY; ++i) {
        if (commandHistory[i].used && commandHistory[i].requestId == requestId) {
            return i;
        }
    }
    return -1;
}

NetworkManager::CommandRecord& NetworkManager::writeCommandRecord(const String& requestId, const String& deviceId,
    const String& result, const String& finalStatus, const String& message) {
    int index = findCommandRecord(requestId);
    if (index < 0) {
        index = 0;
        unsigned long oldest = ULONG_MAX;
        for (int i = 0; i < MAX_COMMAND_HISTORY; ++i) {
            if (!commandHistory[i].used) {
                index = i;
                oldest = 0;
                break;
            }
            if (commandHistory[i].createdAtMs < oldest) {
                oldest = commandHistory[i].createdAtMs;
                index = i;
            }
        }
    }

    commandHistory[index].used = true;
    commandHistory[index].requestId = requestId;
    commandHistory[index].deviceId = deviceId;
    commandHistory[index].result = result;
    commandHistory[index].finalStatus = finalStatus;
    commandHistory[index].message = message;
    commandHistory[index].timestamp = createTimestamp();
    commandHistory[index].createdAtMs = millis();
    return commandHistory[index];
}

void NetworkManager::publishTelemetryUpdate(bool force) {
    if (!isServerRunning()) {
        return;
    }

    const unsigned long nowMs = millis();
    if (!force && nowMs - lastTelemetryPublishMs < TELEMETRY_PUSH_INTERVAL_MS) {
        return;
    }
    lastTelemetryPublishMs = nowMs;

    DynamicJsonDocument doc(768);
    doc["topic"] = "telemetry.update";
    doc["messageId"] = createMessageId();
    doc["timestamp"] = createTimestamp();
    JsonObject data = doc.createNestedObject("data");
    appendTelemetry(data);

    String payload;
    serializeJson(doc, payload);
    broadcastWsMessage(payload);
}

void NetworkManager::publishGatewayStatusChanged() {
    if (!isServerRunning()) {
        return;
    }

    DynamicJsonDocument doc(768);
    doc["topic"] = "gateway.status.changed";
    doc["messageId"] = createMessageId();
    doc["timestamp"] = createTimestamp();

    JsonObject data = doc.createNestedObject("data");
    data["gatewayId"] = config.gatewayId;
    data["online"] = true;
    data["lastHeartbeat"] = lastHeartbeatIso.length() > 0 ? lastHeartbeatIso : createTimestamp();
    data["firmwareVersion"] = config.firmwareVersion;
    data["connectedDeviceCount"] = 11;

    JsonObject wifi = data.createNestedObject("wifi");
    wifi["ssid"] = config.ssid;
    wifi["ip"] = WiFi.localIP().toString();
    wifi["rssi"] = WiFi.RSSI();

    data["uptimeSec"] = static_cast<uint32_t>((millis() - bootTimeMs) / 1000);

    String payload;
    serializeJson(doc, payload);
    broadcastWsMessage(payload);
}

void NetworkManager::publishDeviceStatusChanged(const String& deviceId) {
    if (!isServerRunning()) {
        return;
    }

    DynamicJsonDocument doc(512);
    doc["topic"] = "device.status.changed";
    doc["messageId"] = createMessageId();
    doc["timestamp"] = createTimestamp();

    JsonArray data = doc.createNestedArray("data");
    JsonObject device = data.createNestedObject();
    device["id"] = deviceId;
    device["online"] = true;
    device["updatedAt"] = createTimestamp();

    if (deviceId == PUMP_DEVICE_ID) {
        device["status"] = actuatorState.pumpOn ? DEVICE_ON : DEVICE_OFF;
        device["level"] = actuatorState.pumpOn ? 1 : 0;
    } else if (deviceId == GROW_LIGHT_DEVICE_ID) {
        device["status"] = actuatorState.growLightOn ? DEVICE_ON : DEVICE_OFF;
        device["level"] = actuatorState.growLightOn ? actuatorState.growLightLevel : 0;
    } else {
        device["status"] = "READY";
        device["level"] = 0;
    }

    String payload;
    serializeJson(doc, payload);
    broadcastWsMessage(payload);
}

void NetworkManager::publishCommandResult(const CommandRecord& record) {
    if (!isServerRunning()) {
        return;
    }

    DynamicJsonDocument doc(512);
    doc["topic"] = "command.result";
    doc["messageId"] = createMessageId();
    doc["timestamp"] = createTimestamp();

    JsonObject data = doc.createNestedObject("data");
    data["requestId"] = record.requestId;
    data["deviceId"] = record.deviceId;
    data["result"] = record.result;
    data["finalStatus"] = record.finalStatus;
    data["message"] = record.message;
    data["timestamp"] = record.timestamp;

    String payload;
    serializeJson(doc, payload);
    broadcastWsMessage(payload);
}

void NetworkManager::broadcastWsMessage(const String& payload) {
#ifdef CONFIG_HTTPD_WS_SUPPORT
    if (serverHandle == NULL || payload.length() == 0) {
        return;
    }

    size_t fdCount = MAX_WS_CLIENTS;
    int clientFds[MAX_WS_CLIENTS];
    if (httpd_get_client_list(serverHandle, &fdCount, clientFds) != ESP_OK) {
        return;
    }

    for (size_t i = 0; i < fdCount; ++i) {
        if (httpd_ws_get_fd_info(serverHandle, clientFds[i]) != HTTPD_WS_CLIENT_WEBSOCKET) {
            continue;
        }

        httpd_ws_frame_t frame = {};
        frame.type = HTTPD_WS_TYPE_TEXT;
        frame.payload = reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str()));
        frame.len = payload.length();
        httpd_ws_send_data(serverHandle, clientFds[i], &frame);
    }
#else
    (void)payload;
#endif
}

bool NetworkManager::handleDeviceCommand(const String& deviceId, const String& command, JsonVariant params,
    String& finalStatus, String& message) {
    if (deviceId == PUMP_DEVICE_ID) {
        if (command == "TURN_ON") {
            int durationSec = DEFAULT_PUMP_DURATION_SEC;
            if (!params.isNull() && params["durationSec"].is<int>()) {
                durationSec = params["durationSec"].as<int>();
            }
            if (durationSec < 0) {
                durationSec = 0;
            }
            if (durationSec > MAX_PUMP_DURATION_SEC) {
                durationSec = MAX_PUMP_DURATION_SEC;
            }

            actuatorState.pumpOn = true;
            actuatorState.pumpAutoOffAtMs = durationSec > 0 ? millis() + static_cast<unsigned long>(durationSec) * 1000UL : 0;
            finalStatus = DEVICE_ON;
            message = durationSec > 0 ? String("pump enabled for ") + String(durationSec) + "s" : "pump enabled";
            return true;
        }

        if (command == "TURN_OFF") {
            actuatorState.pumpOn = false;
            actuatorState.pumpAutoOffAtMs = 0;
            finalStatus = DEVICE_OFF;
            message = "pump disabled";
            return true;
        }

        finalStatus = actuatorState.pumpOn ? DEVICE_ON : DEVICE_OFF;
        message = "unsupported pump command";
        return false;
    }

    if (deviceId == GROW_LIGHT_DEVICE_ID) {
        if (command == "TURN_ON") {
            actuatorState.growLightOn = true;
            if (actuatorState.growLightLevel <= 0) {
                actuatorState.growLightLevel = DEFAULT_GROW_LIGHT_LEVEL;
            }
            finalStatus = DEVICE_ON;
            message = "grow light enabled";
            return true;
        }

        if (command == "TURN_OFF") {
            actuatorState.growLightOn = false;
            actuatorState.growLightLevel = 0;
            finalStatus = DEVICE_OFF;
            message = "grow light disabled";
            return true;
        }

        if (command == "SET_LEVEL") {
            int level = DEFAULT_GROW_LIGHT_LEVEL;
            if (!params.isNull() && params["level"].is<int>()) {
                level = params["level"].as<int>();
            }
            level = constrain(level, 1, 5);
            actuatorState.growLightOn = true;
            actuatorState.growLightLevel = level;
            finalStatus = DEVICE_ON;
            message = String("grow light level set to ") + String(level);
            return true;
        }

        finalStatus = actuatorState.growLightOn ? DEVICE_ON : DEVICE_OFF;
        message = "unsupported grow light command";
        return false;
    }

    finalStatus = "UNKNOWN";
    message = "device not found";
    return false;
}

void NetworkManager::processAutoControl() {
    if (actuatorState.pumpOn && actuatorState.pumpAutoOffAtMs > 0 && millis() >= actuatorState.pumpAutoOffAtMs) {
        actuatorState.pumpOn = false;
        actuatorState.pumpAutoOffAtMs = 0;
        publishDeviceStatusChanged(PUMP_DEVICE_ID);
    }
}

NetworkManager& NetworkManager::instance() {
    return networkManager;
}

esp_err_t NetworkManager::handleGatewayStatus(httpd_req_t* req) {
    NetworkManager& manager = instance();
    manager.refreshHeartbeat(false);
    manager.sendJson(req, manager.buildGatewayStatusResponse());
    return ESP_OK;
}

esp_err_t NetworkManager::handleRealtimeTelemetry(httpd_req_t* req) {
    NetworkManager& manager = instance();
    manager.sendJson(req, manager.buildTelemetryResponse());
    return ESP_OK;
}

esp_err_t NetworkManager::handleDevices(httpd_req_t* req) {
    NetworkManager& manager = instance();
    manager.sendJson(req, manager.buildDevicesResponse());
    return ESP_OK;
}

esp_err_t NetworkManager::handleDeviceCommandRequest(httpd_req_t* req) {
    NetworkManager& manager = instance();
    const String uri(req->uri);
    const String prefix("/devices/");
    const String suffix("/command");

    if (!uri.startsWith(prefix) || !uri.endsWith(suffix)) {
        manager.sendJson(req, "{\"code\":1002,\"message\":\"invalid device command path\"}", "400 Bad Request");
        return ESP_OK;
    }

    const String deviceId = uri.substring(prefix.length(), uri.length() - suffix.length());
    const String body = manager.readRequestBody(req);

    DynamicJsonDocument doc(512);
    DeserializationError error = deserializeJson(doc, body);
    if (error) {
        manager.sendJson(req, manager.buildCommandAcceptedResponse(String("cmd-") + String(millis()), deviceId,
            false, "REJECTED", "invalid json", 1001));
        return ESP_OK;
    }

    String requestId = doc["requestId"] | "";
    if (requestId.length() == 0) {
        requestId = String("cmd-") + String(millis());
    }

    const int existedIndex = manager.findCommandRecord(requestId);
    if (existedIndex >= 0 &&
        millis() - manager.commandHistory[existedIndex].createdAtMs <= COMMAND_IDEMPOTENT_WINDOW_MS) {
        manager.sendJson(req, manager.buildCommandAcceptedResponse(requestId, deviceId,
            true, "ACCEPTED", "accepted", 0));
        return ESP_OK;
    }

    const String command = doc["command"] | "";
    String finalStatus;
    String message;
    const bool accepted = manager.handleDeviceCommand(deviceId, command, doc["params"], finalStatus, message);

    if (!accepted) {
        manager.writeCommandRecord(requestId, deviceId, "FAILED", finalStatus, message);
        manager.sendJson(req, manager.buildCommandAcceptedResponse(requestId, deviceId,
            false, "REJECTED", message, 1002));
        return ESP_OK;
    }

    CommandRecord& record = manager.writeCommandRecord(requestId, deviceId, "SUCCESS", finalStatus, message);
    manager.publishDeviceStatusChanged(deviceId);
    manager.publishCommandResult(record);
    manager.sendJson(req, manager.buildCommandAcceptedResponse(requestId, deviceId,
        true, "ACCEPTED", "accepted", 0));
    return ESP_OK;
}

esp_err_t NetworkManager::handleCommandQuery(httpd_req_t* req) {
    NetworkManager& manager = instance();
    const String uri(req->uri);
    const String prefix("/commands/");
    if (!uri.startsWith(prefix)) {
        manager.sendJson(req, "{\"code\":1003,\"message\":\"command not found\"}", "404 Not Found");
        return ESP_OK;
    }

    const String requestId = uri.substring(prefix.length());
    const int index = manager.findCommandRecord(requestId);
    if (index < 0) {
        manager.sendJson(req, "{\"code\":1003,\"message\":\"command not found\"}", "404 Not Found");
        return ESP_OK;
    }

    manager.sendJson(req, manager.buildCommandQueryResponse(manager.commandHistory[index]));
    return ESP_OK;
}

esp_err_t NetworkManager::handleWebSocket(httpd_req_t* req) {
#ifdef CONFIG_HTTPD_WS_SUPPORT
    NetworkManager& manager = instance();

    if (req->method == HTTP_GET) {
        return ESP_OK;
    }

    httpd_ws_frame_t frame = {};
    frame.type = HTTPD_WS_TYPE_TEXT;
    if (httpd_ws_recv_frame(req, &frame, 0) != ESP_OK) {
        return ESP_FAIL;
    }

    if (frame.len > 0) {
        uint8_t* buffer = static_cast<uint8_t*>(malloc(frame.len + 1));
        if (buffer == NULL) {
            return ESP_ERR_NO_MEM;
        }

        frame.payload = buffer;
        if (httpd_ws_recv_frame(req, &frame, frame.len) == ESP_OK) {
            buffer[frame.len] = '\0';
            const String content(reinterpret_cast<char*>(buffer));
            if (content == "ping") {
                DynamicJsonDocument doc(256);
                doc["topic"] = "gateway.status.changed";
                doc["messageId"] = manager.createMessageId();
                doc["timestamp"] = manager.createTimestamp();
                JsonObject data = doc.createNestedObject("data");
                data["gatewayId"] = manager.config.gatewayId;
                data["online"] = manager.status == NET_CONNECTED;
                data["lastHeartbeat"] = manager.lastHeartbeatIso.length() > 0 ?
                    manager.lastHeartbeatIso : manager.createTimestamp();

                String payload;
                serializeJson(doc, payload);

                httpd_ws_frame_t reply = {};
                reply.type = HTTPD_WS_TYPE_TEXT;
                reply.payload = reinterpret_cast<uint8_t*>(const_cast<char*>(payload.c_str()));
                reply.len = payload.length();
                httpd_ws_send_frame(req, &reply);
            }
        }

        free(buffer);
    }
    return ESP_OK;
#else
    (void)req;
    return ESP_FAIL;
#endif
}
