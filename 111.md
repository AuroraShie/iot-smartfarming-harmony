# HarmoAgri 智慧农业联动项目

## 项目简介

这是一个围绕智慧农业温室场景构建的软硬件联动项目，目标不是单纯展示传感器采集，而是打通一条完整的业务闭环：

- ESP32 负责多传感器采集、继电器执行器控制、本地网关服务
- HarmonyOS 应用负责网关接入、田块化管理、设备查看、告警展示和控制下发
- Mock Server 负责在硬件暂未到位时提供可控的模拟联调环境

项目当前已经从“多传感器 demo”演进到“ESP32 本地 HTTP/WebSocket 网关 + HarmonyOS 前端 + 灌溉/补光执行链路”的阶段。

## 项目主体构成

### 1. ESP32 IoT 工程：`iottest`

负责内容：

- 采集空气温湿度、空气质量、土壤湿度、土壤温度、雨滴、人体红外、光照状态
- 通过 `pump_controller`、`grow_light_controller` 控制真实继电器
- 提供本地 HTTP 网关接口
- 提供 WebSocket 实时推送
- 维护命令历史、执行结果和设备状态同步

当前核心文件：

- `iottest/src/main.cpp`：系统初始化、传感器调度、状态汇总、命令处理
- `iottest/src/network_module.cpp`：Wi-Fi、HTTP、WebSocket、本地网关接口
- `iottest/src/config.h`：GPIO、Wi-Fi、网关参数、推送参数
- `iottest/src/pump_controller.cpp`：水泵继电器控制
- `iottest/src/grow_light_controller.cpp`：补光继电器控制

### 2. HarmonyOS 前端工程：`net/HarmoAgri`

负责内容：

- 网关配置管理
- HTTP 请求与 WebSocket 实时状态消费
- 首页遥测展示
- 田块管理与设备归属
- 新设备分配
- 告警推导
- 控制记录持久化

当前核心文件：

- `net/HarmoAgri/entry/src/main/ets/pages/Index.ets`：主页面与业务 UI
- `net/HarmoAgri/entry/src/main/ets/services/GatewayService.ets`：网关服务访问层
- `net/HarmoAgri/entry/src/main/ets/services/FieldRepository.ets`：RDB 本地业务数据持久化
- `net/HarmoAgri/entry/src/main/ets/services/GatewayConfigStore.ets`：Preferences 配置持久化
- `net/HarmoAgri/entry/src/main/ets/models/GatewayModels.ets`：前后端数据模型

### 3. Mock 联调工具：`net/HarmoAgri/tools`

负责内容：

- 模拟网关状态、遥测快照和设备列表
- 模拟命令执行与命令查询
- 注入网关离线、命令失败、设备发现为空、遥测冻结等故障
- 在没有真实 ESP32 的情况下支撑 HarmonyOS 前端联调

当前核心文件：

- `net/HarmoAgri/tools/esp-mock-server.js`
- `net/HarmoAgri/tools/mock-config.default.json`

## 仓库结构

```text
.
├─ README.md
├─ iottest
│  ├─ platformio.ini
│  ├─ README.md
│  └─ src
│     ├─ main.cpp
│     ├─ config.h
│     ├─ network_module.h / .cpp
│     ├─ pump_controller.h / .cpp
│     ├─ grow_light_controller.h / .cpp
│     └─ 各类传感器模块 .h / .cpp
└─ net
   └─ HarmoAgri
      ├─ entry
      │  └─ src/main/ets
      │     ├─ pages
      │     ├─ services
      │     ├─ models
      │     └─ design-system
      └─ tools
         ├─ esp-mock-server.js
         └─ mock-config.default.json
```

## 项目亮点

### 1. 真实网关架构，而不是上传到固定云端的演示程序

当前不是由 ESP32 简单把数据上传到某个固定服务，而是直接由 ESP32 在局域网中提供网关服务。这样带来的价值是：

- HarmonyOS 前端直接对接真实设备接口
- 接口边界清晰，前后端职责明确
- 后续替换更强边缘硬件时，应用侧协议可以尽量保持稳定

### 2. HTTP + WebSocket 双通道设计

- HTTP 负责状态查询、设备列表和命令提交
- WebSocket 负责 `telemetry.update`、`device.status.changed`、`command.result` 等实时消息

这套结构更贴近真实设备系统，而不是纯轮询 demo。

### 3. 模块化硬件结构

IoT 端延续“一种硬件一个独立模块”的方式，避免所有逻辑堆进 `main.cpp`：

- 便于替换传感器
- 便于新增硬件
- 便于排错和单模块定位

### 4. 从“设备列表”升级到“田块化业务结构”

HarmonyOS 前端当前不只是展示设备，而是围绕田块业务组织数据：

- 默认田块初始化
- 新设备池
- 批量分配到田块
- 每个田块独立查看设备、灌溉、补光、告警和记录

### 5. 支持真实硬件联调和 Mock 联调双模式

- 有板子时，走 ESP32 本地网关
- 没板子时，走 Mock Server

这让前端、设备端和联调工作可以并行推进。

## 当前支持的硬件与执行器

### 传感器

| 模块 | 作用 | 当前接口/引脚 |
|---|---|---|
| DHT11 | 空气温度、空气湿度 | `GPIO4` |
| SGP30 | TVOC、eCO2 | `GPIO18/19` I2C |
| HW-390 | 土壤湿度 | `GPIO34` |
| DS18B20 x2 | 土壤温度 | `GPIO5`、`GPIO21` |
| MH-RD | 雨滴检测 | `GPIO13` |
| HC-SR501 | 人体红外 | `GPIO12` |
| 光敏传感器 | 光照状态 | `GPIO32` |

### 执行器

| 执行器 | 设备 ID | GPIO | 说明 |
|---|---|---|---|
| 水泵继电器 | `pump-001` | `GPIO27` | 低电平触发 |
| 补光继电器 | `growlight-001` | `GPIO33` | 当前记录档位，不做真实 PWM 调光 |

### 指示灯

- `GPIO2`：板载状态指示灯
- `GPIO25` / `GPIO26`：RGB 状态灯

## 当前接口协议

### HTTP 接口

- `GET /gateway/status`
- `GET /telemetry/realtime`
- `GET /devices`
- `POST /devices/{id}/command`
- `GET /commands/{requestId}`

### WebSocket 路径

- `ws://<ip>:8080/ws`

### WebSocket 主题

- `telemetry.update`
- `gateway.status.changed`
- `device.status.changed`
- `command.result`

### 关键兼容字段

HarmonyOS 端当前依赖的关键字段已经在 ESP32 网关中对齐，包括：

- 遥测快照：`temperature`、`humidity`、`light`、`co2`、`eco2`、`tvoc`、`soilMoisture`、`soilTemperature`、`rainDetected`、`motionDetected`、`timestamp`
- 设备遥测：`metricType`、`value`、`unit`、`timestamp`
- 执行器设备 ID：`pump-001`、`growlight-001`

## 数据流与业务闭环

```text
传感器采集
  -> main.cpp 汇总数据
  -> network_module 生成网关快照
  -> HTTP / WebSocket 对外提供
  -> HarmonyOS GatewayService 拉取/订阅
  -> FieldRepository 写入本地 RDB
  -> 页面展示、告警推导、田块管理

前端下发命令
  -> POST /devices/{id}/command
  -> ESP32 执行真实继电器控制
  -> 记录命令结果
  -> WebSocket 推送 command.result / device.status.changed
  -> 前端更新控制结果与设备状态
```

## 当前前端功能清单

基于当前代码，HarmonyOS 端已经具备以下能力：

- 首页总览
- 网关连接测试
- 遥测数据展示
- 田块列表与田块详情
- 新设备池与批量分配
- 灌溉控制页
- 补光控制页
- 告警中心
- 控制记录展示
- 网关配置持久化到 Preferences
- 设备与田块数据持久化到 RDB
- WebSocket 实时状态消费，轮询作为刷新兜底

## 项目运行方式

项目建议按两种模式运行：真实 ESP32 网关模式、Mock Server 模式。

### 模式 A：真实 ESP32 网关联调

适合场景：

- 验证真实传感器采集
- 验证真实继电器控制
- 验证 HarmonyOS 与 ESP32 的完整联调链路

#### 1. 环境准备

- 安装 VSCode + PlatformIO
- 准备 ESP32 开发板和对应传感器/继电器
- 电脑与 ESP32 接入同一局域网
- 准备 DevEco Studio 运行 HarmonyOS 前端

#### 2. 配置 ESP32

编辑 `iottest/src/config.h`，至少确认：

- `WIFI_SSID`
- `WIFI_PASSWORD`
- `GATEWAY_PORT`
- `GATEWAY_WS_PATH`

建议保持默认：

- 端口：`8080`
- WebSocket 路径：`/ws`

#### 3. 编译、烧录与串口监视

在 `iottest` 目录执行：

```powershell
pio run
pio run -t upload
pio device monitor
```

启动成功后，串口应出现类似信息：

```text
Wi-Fi 已连接，IP: 192.168.x.x
Gateway is serving at http://192.168.x.x:8080
```

#### 4. 独立验证网关接口

```powershell
Invoke-RestMethod http://<ESP_IP>:8080/gateway/status
Invoke-RestMethod http://<ESP_IP>:8080/telemetry/realtime
Invoke-RestMethod http://<ESP_IP>:8080/devices
```

#### 5. 验证命令链路

```powershell
$body = @{
  command = "TURN_ON"
  requestId = "cmd-test-001"
  params = @{ durationSec = 10 }
} | ConvertTo-Json

Invoke-RestMethod http://<ESP_IP>:8080/devices/pump-001/command -Method Post -ContentType "application/json" -Body $body
Invoke-RestMethod http://<ESP_IP>:8080/commands/cmd-test-001
```

#### 6. 启动 HarmonyOS 前端

使用 DevEco Studio 打开 `net/HarmoAgri`，运行 `entry` 模块到模拟器或真机。

应用内网关配置建议填写：

- `host`：`<ESP_IP>`
- `port`：`8080`
- `protocol`：`HTTP_WS`
- `wsPath`：`/ws`

### 模式 B：Mock Server 联调

适合场景：

- 没有 ESP32 板子
- 只验证前端页面与业务流
- 需要模拟异常和边界情况

#### 1. 启动 Mock Server

在 `net/HarmoAgri` 目录执行：

```powershell
node .\tools\esp-mock-server.js
```

默认监听：

- `http://127.0.0.1:8080`

如果端口冲突，可改用：

```powershell
$env:ESP_MOCK_PORT=8090
node .\tools\esp-mock-server.js
```

#### 2. 启动 HarmonyOS 前端

同样使用 DevEco Studio 打开 `net/HarmoAgri` 并运行 `entry` 模块。

配置方式：

- Windows 本机调试：`host = 127.0.0.1`
- HarmonyOS 模拟器访问电脑服务时，通常使用：`host = 10.0.2.2`
- `port` 填实际 Mock Server 端口
- `wsPath` 保持 `/ws`

#### 3. 模拟场景

Mock Server 支持：

- 网关离线
- 命令强制失败
- 发现设备为空
- 遥测冻结
- 人工修改土壤湿度、光照、CO2 等值

## 推荐联调顺序

1. 先确认 ESP32 或 Mock Server 是否正常启动
2. 再测 `/gateway/status`
3. 再测 `/telemetry/realtime`
4. 再测 `/devices`
5. 再测命令接口与命令查询
6. 最后进入 HarmonyOS 页面验证交互和状态回写

这个顺序能最快区分问题是在硬件、网络、网关接口还是前端消费层。

## 项目当前状态

### 已完成

- ESP32 本地网关服务可运行
- HTTP 接口已与 HarmonyOS 当前消费结构对齐
- WebSocket 推送主题已接入
- 水泵与补光继电器已独立模块化
- HarmonyOS 已完成田块化业务结构
- 新设备池、批量分配、告警与控制记录已落地
- Mock Server 已支持多种联调与故障注入场景

### 当前限制

- 光照值目前仍为映射值，不是真实 lux 传感器精确数据
- 自动灌溉、自动补光逻辑仍默认关闭，只保留扩展入口
- 继电器闭环已经打通，但仍建议继续做实际上板验证和安全保护
- 补光“档位”当前是业务记录值，不代表真实硬件调光

## 开发约定

- IoT 端新增硬件继续沿用“一个模块一对 `.h/.cpp`”
- 不把执行器状态假维护写回 `network_module.cpp`
- `main.cpp` 负责初始化、调度、汇总和命令落地
- `config.h` 统一管理 GPIO 和系统参数
- HarmonyOS 端网关配置走 Preferences，业务数据走 RDB
- 文档以本 README 与 `iottest/README.md` 为主，不再继续分散维护根目录多份旧说明

## 快速入口

- ESP32 说明：`iottest/README.md`
- Mock Server 说明：`net/HarmoAgri/tools/README_mock_server.md`
- HarmonyOS 前端入口：`net/HarmoAgri/entry/src/main/ets/pages/Index.ets`

## 一句话总结

这是一个已经具备真实联调能力的智慧农业项目：ESP32 负责采集与控制，HarmonyOS 负责田块化管理与展示，Mock Server 负责开发期替身，三者共同形成了从设备到业务页面的完整闭环。
