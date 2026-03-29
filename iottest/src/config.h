#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// 配置文件 - 集中管理所有针脚定义和配置
// ============================================

// ==================== 串口配置 ====================
#define SERIAL_BAUD_RATE 115200

// ==================== DHT11 配置 ====================
#define DHT_PIN 4           // DHT11连接的GPIO引脚
#define DHT_TYPE DHT11      // 传感器类型

// ==================== SGP30 配置 ====================
#define SGP30_SDA 18        // SGP30 I2C SDA引脚
#define SGP30_SCL 19        // SGP30 I2C SCL引脚

// ==================== HW-390 土壤传感器配置 ====================
#define SOIL_MOISTURE_PIN 34   // HW-390模拟输出连接的GPIO (ADC引脚)

// ==================== MH-RD 雨滴传感器配置 ====================
#define RAIN_SENSOR_PIN 13     // MH-RD数字输出连接的GPIO引脚

// ==================== DS18B20 温度传感器配置 ====================
#define DS18B20_PIN_1 5       // 第一个DS18B20数据引脚（1-Wire总线）
#define DS18B20_PIN_2 21     // 第二个DS18B20数据引脚（1-Wire总线）
#define DS18B20_ADDRESS 0x28 // DS18B20地址（默认，可自动查找）

// ==================== HC-SR501 红外人体传感器配置 ====================
#define PIR_PIN 12            // HC-SR501数字输出连接的GPIO引脚

// ==================== 光敏传感器配置 ====================
#define LIGHT_SENSOR_PIN 32   // 光敏传感器数字输出连接的GPIO引脚
// 注意：如果使用模拟输出，需要选择ADC引脚，如GPIO32、33、34、35等

// ==================== 板载LED配置 ====================
#define BUILTIN_LED_PIN 2     // ESP32内置LED引脚（GPIO2，大多数开发板）

// ==================== RGB LED 配置 ====================
#define RGB_RED_PIN    25     // RGB LED红色引脚
#define RGB_GREEN_PIN  26     // RGB LED绿色引脚
#define RGB_BLUE_PIN  -1     // RGB LED蓝色引脚（-1表示未连接）

// PWM设置
#define LEDC_CHANNEL_RED     0
#define LEDC_CHANNEL_GREEN   1
#define LEDC_FREQUENCY       5000  // 5kHz
#define LEDC_RESOLUTION      8     // 8位分辨率（0-255）

// 亮度值（0-255）
#define BRIGHTNESS_RED    255
#define BRIGHTNESS_GREEN  255
#define BRIGHTNESS_YELLOW_RED   255
#define BRIGHTNESS_YELLOW_GREEN 50

// ==================== I2C 地址 ====================
#define SGP30_I2C_ADDRESS 0x58  // SGP30默认I2C地址

// ==================== 系统配置 ====================
#define SENSOR_READ_INTERVAL 2000  // 传感器读取间隔（毫秒）
#define I2C_SCAN_ENABLED false     // 是否启用I2C扫描（生产环境设为false）

// ==================== 网络配置 ====================
// WiFi配置
#define WIFI_SSID "YOUR_WIFI_SSID"           // WiFi名称
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"   // WiFi密码
#define WIFI_CONNECT_TIMEOUT 10000           // WiFi连接超时时间（毫秒）
#define WIFI_RETRY_INTERVAL 5000             // WiFi重试间隔（毫秒）

// 服务器配置
#define SERVER_HOST "192.168.1.100"          // 服务器IP地址
#define SERVER_PORT 3000                     // 服务器端口
#define SERVER_PROTOCOL "http"               // 协议（http或https）

// 设备配置
#define DEVICE_ID "esp32-sensor-001"         // 设备ID
#define DEVICE_NAME "ESP32环境监测站"         // 设备名称
#define DEVICE_TYPE "SENSOR"                 // 设备类型
#define DEVICE_LOCATION "北侧棚区"            // 设备位置

// 网络通信配置
#define HTTP_TIMEOUT 5000                    // HTTP请求超时时间（毫秒）
#define DATA_SEND_INTERVAL 5000              // 数据发送间隔（毫秒）
#define MAX_RETRY_COUNT 3                    // 最大重试次数

// WebSocket配置（可选）
#define WS_ENABLED false                     // 是否启用WebSocket
#define WS_PATH "/ws"                         // WebSocket路径

#endif // CONFIG_H