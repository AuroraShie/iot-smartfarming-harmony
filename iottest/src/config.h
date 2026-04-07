#ifndef CONFIG_H
#define CONFIG_H

// ==================== 串口配置 ====================
#define SERIAL_BAUD_RATE 115200

// ==================== DHT11 配置 ====================
#define DHT_PIN 4
#define DHT_TYPE DHT11

// ==================== SGP30 配置 ====================
#define SGP30_SDA 18
#define SGP30_SCL 19
#define SGP30_I2C_ADDRESS 0x58

// ==================== 传感器引脚配置 ====================
#define SOIL_MOISTURE_PIN 34
#define RAIN_SENSOR_PIN 13
#define DS18B20_PIN_1 5
#define DS18B20_PIN_2 21
#define DS18B20_ADDRESS 0x28
#define PIR_PIN 14
#define LIGHT_SENSOR_PIN 32

// ==================== 本地指示灯配置 ====================
#define BUILTIN_LED_PIN 2
#define RGB_RED_PIN 25
#define RGB_GREEN_PIN 26
#define RGB_BLUE_PIN -1

// ==================== 继电器执行器配置 ====================
#define PUMP_RELAY_PIN 27
#define GROW_LIGHT_RELAY_PIN 33
#define RELAY_ACTIVE_LOW 0

// ==================== PWM 配置 ====================
#define LEDC_CHANNEL_RED 0
#define LEDC_CHANNEL_GREEN 1
#define LEDC_FREQUENCY 5000
#define LEDC_RESOLUTION 8

#define BRIGHTNESS_RED 255
#define BRIGHTNESS_GREEN 255
#define BRIGHTNESS_YELLOW_RED 255
#define BRIGHTNESS_YELLOW_GREEN 50

// ==================== 系统配置 ====================
#define SENSOR_READ_INTERVAL 1000
#define I2C_SCAN_ENABLED false
#define AUTO_CONTROL_ENABLED false
#define SOIL_SENSOR_SAMPLE_COUNT 5
#define SOIL_SENSOR_PUMP_SETTLE_MS 2000
#define SOIL_SENSOR_MAX_STEP_DURING_PUMP 2
#define RAIN_SENSOR_SAMPLE_COUNT 5

// ==================== Wi-Fi 配置 ====================
#define WIFI_SSID "Aurora-iPhone"
#define WIFI_PASSWORD "88888888"
#define WIFI_CONNECT_TIMEOUT 10000
#define WIFI_RETRY_INTERVAL 5000
#define MAX_RETRY_COUNT 5

// ==================== 本地网关配置 ====================
#define GATEWAY_ID "esp-gateway-001"
#define GATEWAY_NAME "ESP32 Gateway"
#define GATEWAY_LOCATION "North Greenhouse"
#define GATEWAY_PORT 8080
#define GATEWAY_WS_PATH "/ws"
#define GATEWAY_FIRMWARE_VERSION "1.2.0"

// ==================== 网络推送配置 ====================
#define TELEMETRY_PUSH_INTERVAL_MS 1000
#define HEARTBEAT_INTERVAL_MS 15000
#define COMMAND_IDEMPOTENT_WINDOW_MS 30000
#define MAX_COMMAND_HISTORY 12
#define MAX_WS_CLIENTS 8

// ==================== 执行器默认参数 ====================
#define DEFAULT_GROW_LIGHT_LEVEL 3
#define DEFAULT_PUMP_DURATION_SEC 30
#define MAX_PUMP_DURATION_SEC 600

#endif
