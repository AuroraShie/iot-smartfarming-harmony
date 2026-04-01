# iottest - ESP32 本地网关与农业设备控制工程

## 项目定位

`iottest` 是整个仓库中的 ESP32 设备端工程，当前已经不是早期单纯的“多传感器采集 demo”，而是一个面向 HarmonyOS 前端联调的本地农业网关。

它负责：

- 采集环境与状态类传感器数据
- 控制真实继电器执行器
- 在局域网中启动 HTTP 网关
- 提供 WebSocket 实时消息
- 接收前端命令并返回执行结果

当前版本重点是把“采集 + 网关 + 命令 + 执行器”链路做稳。

## 当前能力

### 传感器采集

已接入：

- DHT11：空气温度、空气湿度
- SGP30：TVOC、eCO2
- HW-390：土壤湿度
- DS18B20 x2：土壤温度
- MH-RD：雨滴检测
- HC-SR501：人体红外检测
- 光敏传感器：明暗状态检测

### 执行器控制

已接入：

- `pump-001`：水泵继电器
- `growlight-001`：补光继电器

### 网络网关

当前提供：

- `GET /gateway/status`
- `GET /telemetry/realtime`
- `GET /devices`
- `POST /devices/{id}/command`
- `GET /commands/{requestId}`
- `ws://<esp-ip>:8080/ws`

### WebSocket 推送主题

- `telemetry.update`
- `gateway.status.changed`
- `device.status.changed`
- `command.result`

## 当前目录结构

```text
iottest
├─ platformio.ini
├─ README.md
├─ 扩展计划.md
└─ src
   ├─ main.cpp
   ├─ config.h
   ├─ network_module.h / .cpp
   ├─ pump_controller.h / .cpp
   ├─ grow_light_controller.h / .cpp
   ├─ dht11_sensor.h / .cpp
   ├─ sgp30_sensor.h / .cpp
   ├─ hw390_sensor.h / .cpp
   ├─ ds18b20_sensor.h / .cpp
   ├─ rain_sensor.h / .cpp
   ├─ pir_sensor.h / .cpp
   └─ light_sensor.h / .cpp
```

## 代码结构说明

### `main.cpp`

负责：

- 串口初始化
- 传感器初始化
- RGB 状态灯初始化
- 执行器初始化
- 周期性读取传感器
- 汇总本轮快照
- 调用网络模块同步网关数据
- 注册网关命令处理函数

当前原则是：`main.cpp` 只做初始化、调度、汇总和命令落地，不在里面堆复杂网络协议细节。

### `network_module.h/.cpp`

负责：

- Wi-Fi 连接与重连
- 本地 HTTP 服务
- WebSocket 推送
- 网关心跳与时间戳生成
- 设备列表组装
- 命令请求解析
- 命令历史缓存
- 执行结果与状态广播

这个模块是 HarmonyOS 前端和 ESP32 之间的协议适配核心。

### `pump_controller.h/.cpp`

负责：

- 水泵继电器初始化
- 开关控制
- 定时自动关泵

### `grow_light_controller.h/.cpp`

负责：

- 补光继电器初始化
- 开关控制
- 档位记录

注意：当前“档位”是业务层记录值，不代表真实 PWM 调光。

### 各传感器模块

每种硬件一个独立模块，便于：

- 更换具体型号
- 独立排错
- 后续继续扩展

## 当前硬件引脚

### 传感器

| 模块 | 引脚 |
|---|---|
| DHT11 | `GPIO4` |
| SGP30 SDA/SCL | `GPIO18` / `GPIO19` |
| HW-390 | `GPIO34` |
| DS18B20 #1 | `GPIO5` |
| DS18B20 #2 | `GPIO21` |
| MH-RD | `GPIO13` |
| HC-SR501 | `GPIO12` |
| 光敏传感器 | `GPIO32` |

### 指示与执行器

| 模块 | 引脚 | 说明 |
|---|---|---|
| 板载指示灯 | `GPIO2` | 明暗状态提示 |
| RGB 红灯 | `GPIO25` | 状态灯 |
| RGB 绿灯 | `GPIO26` | 状态灯 |
| 水泵继电器 | `GPIO27` | 低电平触发 |
| 补光继电器 | `GPIO33` | 低电平触发 |

## 当前配置说明

配置集中在 `src/config.h`。

重点配置项包括：

- 串口波特率：`SERIAL_BAUD_RATE`
- 传感器引脚
- 执行器引脚
- `WIFI_SSID`
- `WIFI_PASSWORD`
- `GATEWAY_ID`
- `GATEWAY_PORT`
- `GATEWAY_WS_PATH`
- `TELEMETRY_PUSH_INTERVAL_MS`
- `DEFAULT_PUMP_DURATION_SEC`
- `DEFAULT_GROW_LIGHT_LEVEL`

建议在上板前先检查：

- Wi-Fi 是否为可连接的 2.4G 网络
- `upload_port` 是否与当前开发板串口一致
- 继电器是否确实为低电平触发

## 编译与烧录

### 环境要求

- VSCode
- PlatformIO
- ESP32 开发板串口驱动

### 依赖库

当前 `platformio.ini` 已配置：

- `adafruit/DHT sensor library`
- `adafruit/Adafruit Unified Sensor`
- `adafruit/Adafruit SGP30 Sensor`
- `paulstoffregen/OneWire`
- `milesburton/DallasTemperature`
- `bblanchon/ArduinoJson`
- `links2004/WebSockets`

### 常用命令

在 `iottest` 目录执行：

```powershell
pio run
pio run -t upload
pio device monitor
```

## 启动后的预期行为

### 串口启动阶段

你应看到：

- 各传感器初始化结果
- 板载指示灯初始化
- RGB 状态灯初始化
- 执行器引脚初始化
- Wi-Fi 连接状态
- 网关服务地址

典型输出类似：

```text
ESP32 多传感器环境监测系统
版本: 3.0 (本地网关 + 继电器执行器版本)

Wi-Fi 已连接，IP: 192.168.x.x
Gateway is serving at http://192.168.x.x:8080
```

### 运行阶段

系统会按 `SENSOR_READ_INTERVAL` 周期：

- 读取全部传感器
- 打印本轮状态
- 更新 RGB 状态灯
- 同步最新快照到本地网关

### RGB 状态灯逻辑

- 绿色：本轮全部传感器正常
- 黄色：存在环境异常，例如土壤偏干或环境偏暗
- 红色：存在传感器故障

## 接口说明

### `GET /gateway/status`

返回：

- 网关 ID
- 在线状态
- 最后心跳
- 固件版本
- Wi-Fi 信息
- 运行时长

### `GET /telemetry/realtime`

返回当前快照，包括：

- 温度
- 湿度
- 光照
- CO2 / eCO2
- TVOC
- 土壤湿度
- 土壤温度
- 雨滴检测
- 人体活动检测
- 时间戳

### `GET /devices`

返回：

- 9 个传感器类设备项
- 2 个执行器设备项

并对齐 HarmonyOS 当前消费字段：

- `id`
- `gatewayId`
- `name`
- `type`
- `online`
- `status`
- `location`
- `capabilities`
- `telemetry.metricType/value/unit/timestamp`

### `POST /devices/{id}/command`

当前支持：

#### 水泵 `pump-001`

- `TURN_ON`
- `TURN_OFF`

可选参数：

- `durationSec`

#### 补光 `growlight-001`

- `TURN_ON`
- `TURN_OFF`
- `SET_LEVEL`

可选参数：

- `level`

### `GET /commands/{requestId}`

用于查询命令执行结果，返回：

- `requestId`
- `deviceId`
- `result`
- `finalStatus`
- `message`
- `timestamp`

## WebSocket 实时消息

路径：

```text
ws://<esp-ip>:8080/ws
```

### 当前主题

- `telemetry.update`
- `gateway.status.changed`
- `device.status.changed`
- `command.result`

### 用途

- 推送最新遥测
- 推送网关心跳更新
- 推送设备状态变化
- 推送命令最终结果

## 与 HarmonyOS 的兼容约定

当前 IoT 端已经按 HarmonyOS 前端现有接口对齐，后续开发尽量遵守以下约定：

- 不随意改动 `pump-001`、`growlight-001`
- 不随意改动 `/devices` 中 `telemetry.metricType/value/unit/timestamp` 结构
- `device.status.changed` 的 `data` 维持数组结构
- 默认端口保持 `8080`
- 默认 WebSocket 路径保持 `/ws`

## 当前实现亮点

### 1. 本地网关化

ESP32 直接作为局域网网关工作，而不是向外部服务上报后再转发。

### 2. 执行器独立模块化

水泵和补光控制已经从网络层中拆出，避免“只改状态、不驱动硬件”的假控制。

### 3. 命令闭环完整

已经形成：

- 前端提交命令
- ESP32 接收并执行
- 本地记录命令结果
- HTTP 查询结果
- WebSocket 推送结果

### 4. 结构便于继续扩展

后续可继续增加：

- 自动灌溉
- 自动补光
- 更准确的光照传感器
- 更多执行器

## 运行与联调建议

### 真实硬件联调建议顺序

1. 先看串口是否正常启动
2. 再测 `/gateway/status`
3. 再测 `/telemetry/realtime`
4. 再测 `/devices`
5. 再测命令接口
6. 最后连接 HarmonyOS 前端

### HarmonyOS 配置建议

- `host = ESP32 局域网 IP`
- `port = 8080`
- `protocol = HTTP_WS`
- `wsPath = /ws`

## 当前限制与待完善项

- 光照值当前仍为映射值，不是真实 lux
- 自动控制逻辑默认关闭
- 上板后的长期稳定性还需要更多实测
- 补光档位当前仅作业务记录，不是物理调光

## 后续开发规则

- 新增硬件继续保持“一个模块一对 `.h/.cpp`”
- 不要把执行器状态回写成网络层里的假状态
- `main.cpp` 继续保持轻量，只做调度和汇总
- 所有 GPIO、时间参数、默认值统一进 `config.h`

## 相关说明

- 项目总览请看仓库根目录 `README.md`
- 早期扩展思路和历史方案请看 `扩展计划.md`

## 一句话总结

当前 `iottest` 已经是一个可直接为 HarmonyOS 前端提供服务的 ESP32 农业网关工程，重点价值不再是“采多少种传感器”，而是“传感器、接口、命令和真实继电器控制已经能形成闭环”。
