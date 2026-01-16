# QuickStart_Demo_ARM 客户端架构文档

## 1. 项目概述

### 1.1 项目简介
QuickStart_Demo_ARM 是一个基于火山引擎 RTC SDK 的 AI 语音交互客户端，运行在树莓派 5 (ARM64) 平台上。支持多种 AI 模式（闲聊、监督、教学），通过实时音视频与 AI 进行交互。

### 1.2 技术栈
- **平台**: 树莓派 5 (ARM64 Linux)
- **UI 框架**: Qt 5.15
- **RTC SDK**: 火山引擎 VolcEngineRTC
- **视频采集**: GStreamer (libcamera/v4l2)
- **音频采集/播放**: ALSA
- **构建系统**: CMake 3.10+
- **语言**: C++14

### 1.3 核心功能
- 实时音视频通话
- AI 语音交互（多模式）
- 实时字幕显示
- 摄像头切换（CSI/USB）
- 音量控制

---

## 2. 目录结构

```
QuickStart_Demo_ARM/
├── src/                          # 源代码
│   ├── main.cpp                  # 程序入口
│   ├── app/                      # 应用层
│   │   ├── RoomMainWidget.*      # 主窗口（UI 协调）
│   │   └── AIManager.*           # AI 管理器
│   ├── ui/                       # UI 层
│   │   ├── widgets/              # 业务 Widget
│   │   │   ├── LoginWidget.*     # 登录界面
│   │   │   ├── ModeWidget.*      # 模式选择
│   │   │   ├── OperateWidget.*   # 操作控制栏
│   │   │   └── ConversationWidget.* # 对话字幕
│   │   ├── components/           # 通用 UI 组件
│   │   │   ├── VideoRenderWidget.*   # CPU 视频渲染
│   │   │   ├── VideoRenderWidgetGL.* # GPU 视频渲染
│   │   │   └── VideoWidget.*     # 视频容器
│   │   └── styles/               # QSS 样式（待完善）
│   ├── core/                     # 核心业务层
│   │   ├── api/                  # API 模块
│   │   │   └── AIGCApi.*         # AIGC 服务 API
│   │   ├── rtc/                  # RTC 模块
│   │   │   ├── RoomManager.*     # 房间管理器
│   │   │   └── CustomVideoSink.* # 自定义视频渲染
│   │   └── media/                # 媒体模块
│   │       └── MediaManager.*    # 媒体管理器
│   ├── drivers/                  # 驱动层
│   │   ├── interfaces/           # 抽象接口
│   │   │   ├── IVideoSource.h    # 视频源接口
│   │   │   ├── IAudioSource.h    # 音频源接口
│   │   │   └── IAudioRender.h    # 音频渲染接口
│   │   └── impl/                 # 平台实现
│   │       ├── linux/            # Linux ARM 实现
│   │       │   ├── ExternalVideoSource.*  # GStreamer 视频采集
│   │       │   ├── ExternalAudioSource.*  # ALSA 音频采集
│   │       │   └── ExternalAudioRender.*  # ALSA 音频播放
│   │       └── mock/             # Mock 实现（待完善）
│   └── common/                   # 公共模块
│       ├── AIMode.h              # AI 模式枚举
│       ├── Constants.h           # 常量定义
│       └── TokenGenerator/       # Token 生成器
├── resources/                    # 资源文件
│   ├── app.qrc                   # Qt 资源文件
│   ├── images/                   # 图标资源
│   └── default.desktop           # 桌面快捷方式
├── 3rdparty/                     # 第三方库
│   └── VolcEngineRTC_arm/        # RTC SDK
├── scripts/                      # 脚本
│   └── run.sh                    # 运行脚本
├── docs/                         # 文档
├── tests/                        # 测试（待完善）
├── cmake/                        # CMake 模块（待完善）
└── CMakeLists.txt                # 构建配置
```

---

## 3. 架构设计

### 3.1 分层架构

```
┌─────────────────────────────────────────────────────────┐
│                      应用层 (app/)                       │
│  RoomMainWidget - UI 协调，事件分发                       │
│  AIManager - AI 模式管理，启动/停止                       │
├─────────────────────────────────────────────────────────┤
│                      UI 层 (ui/)                         │
│  widgets/ - 业务 Widget (Login/Mode/Operate/Conversation)│
│  components/ - 通用组件 (VideoRenderWidget)              │
├─────────────────────────────────────────────────────────┤
│                    核心业务层 (core/)                     │
│  api/AIGCApi - AIGC 服务调用                             │
│  rtc/RoomManager - RTC 房间管理                          │
│  media/MediaManager - 音视频设备管理                      │
├─────────────────────────────────────────────────────────┤
│                     驱动层 (drivers/)                    │
│  interfaces/ - 抽象接口 (IVideoSource/IAudioSource/...)  │
│  impl/linux/ - Linux ARM 实现 (GStreamer/ALSA)          │
├─────────────────────────────────────────────────────────┤
│                     公共层 (common/)                     │
│  Constants, AIMode, TokenGenerator                       │
└─────────────────────────────────────────────────────────┘
```

### 3.2 核心类职责

| 类名 | 职责 | 所在层 |
|------|------|--------|
| `RoomMainWidget` | UI 协调，事件分发，RTC 回调处理 | 应用层 |
| `AIManager` | AI 模式管理，启动/停止 AI | 应用层 |
| `RoomManager` | RTC 引擎创建/销毁，房间加入/离开 | 核心层 |
| `MediaManager` | 音视频设备初始化、启动、停止、配置 | 核心层 |
| `AIGCApi` | AIGC 服务 API 调用 | 核心层 |
| `ExternalVideoSource` | GStreamer 视频采集 | 驱动层 |
| `ExternalAudioSource` | ALSA 音频采集 | 驱动层 |
| `ExternalAudioRender` | ALSA 音频播放 | 驱动层 |

### 3.3 数据流

```
┌──────────┐    ┌─────────────┐    ┌─────────────┐    ┌──────────┐
│ 摄像头   │───▶│ VideoSource │───▶│ RTC Engine  │───▶│ 远端用户 │
└──────────┘    └─────────────┘    └─────────────┘    └──────────┘
                                          │
┌──────────┐    ┌─────────────┐           │
│ 麦克风   │───▶│ AudioSource │───────────┘
└──────────┘    └─────────────┘
                                          │
┌──────────┐    ┌─────────────┐           │
│ 扬声器   │◀───│ AudioRender │◀──────────┘
└──────────┘    └─────────────┘

┌──────────┐    ┌─────────────┐    ┌─────────────┐
│ AI 服务  │◀──▶│  AIGCApi    │◀──▶│ AIManager   │
└──────────┘    └─────────────┘    └─────────────┘
```

---

## 4. 模块详解

### 4.1 AIManager

**职责**: 管理 AI 的生命周期和模式切换

**主要接口**:
```cpp
void initialize(const QString& serverUrl);  // 初始化
void setMode(AIMode mode);                  // 设置模式
void startAI();                             // 启动 AI
void stopAI();                              // 停止 AI
bool isAIStarted() const;                   // 查询状态
```

**信号**:
- `configLoaded(RTCConfig)` - 配置加载成功
- `aiStarted()` - AI 启动成功
- `aiFailed(QString)` - AI 启动失败
- `aiStopped()` - AI 停止
- `modeChanged(AIMode)` - 模式变化

### 4.2 RoomManager

**职责**: 管理 RTC 引擎和房间生命周期

**主要接口**:
```cpp
bool createEngine(const QString& appId, IRTCEngineEventHandler* handler);
void destroyEngine();
bool joinRoom(const QString& roomId, const QString& userId, 
              const QString& token, IRTCRoomEventHandler* handler);
void leaveRoom();
IRTCEngine* getEngine() const;
IRTCRoom* getRoom() const;
```

### 4.3 MediaManager

**职责**: 管理音视频设备

**主要接口**:
```cpp
void initialize(IRTCEngine* engine);        // 初始化
void startAll() / stopAll();                // 启动/停止所有媒体
void setCamera(const CameraInfo& camera);   // 切换摄像头
void setMicVolume(int volume);              // 麦克风音量
void setSpeakerVolume(int volume);          // 扬声器音量
void setSpeakerMute(bool mute);             // 扬声器静音
```

### 4.4 驱动层接口

**IVideoSource** (视频源接口):
```cpp
virtual void startCapture() = 0;
virtual void stopCapture() = 0;
virtual bool isCapturing() const = 0;
virtual QList<CameraInfo> detectCameras() = 0;
virtual void setCamera(const CameraInfo& camera) = 0;
```

**IAudioSource** (音频源接口):
```cpp
virtual void startCapture() = 0;
virtual void stopCapture() = 0;
virtual void setVolume(int volume) = 0;
```

**IAudioRender** (音频渲染接口):
```cpp
virtual void startRender() = 0;
virtual void stopRender() = 0;
virtual void setVolume(int volume) = 0;
virtual void setMute(bool mute) = 0;
```

---

## 5. 构建与运行

### 5.1 依赖项

```bash
# Qt 5
sudo apt install qt5-default qtbase5-dev

# GStreamer
sudo apt install libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev

# ALSA
sudo apt install libasound2-dev

# OpenGL
sudo apt install libgl1-mesa-dev
```

### 5.2 构建

```bash
cd QuickStart_Demo_ARM
mkdir build && cd build
cmake ..
make -j4
```

### 5.3 运行

```bash
# 方式1: 使用启动脚本（同时启动 Server）
cd /path/to/rtc-aigc-demo-main
./start-arm-aigc.sh

# 方式2: 单独运行客户端
cd QuickStart_Demo_ARM
./scripts/run.sh
```

---

## 6. 开发规范

### 6.1 代码风格

- **命名**: 类名 `PascalCase`，成员变量 `m_camelCase`，函数 `camelCase`
- **缩进**: 4 空格
- **头文件**: 使用 `#pragma once`
- **注释**: 中文注释，关键逻辑需要说明

### 6.2 Git 提交规范

```
<type>: <subject>

<body>
```

**Type**:
- `feat`: 新功能
- `fix`: 修复 bug
- `refactor`: 重构
- `docs`: 文档
- `style`: 代码格式
- `test`: 测试

### 6.3 分支策略

- `master`: 主分支，稳定版本
- `dev`: 开发分支
- `feature/*`: 功能分支
- `bugfix/*`: 修复分支

---

## 7. 后续计划

### 7.1 已完成

- [x] Phase 1: 目录重组
- [x] Phase 2: 抽取驱动层接口 (IVideoSource/IAudioSource/IAudioRender)
- [x] Phase 3: 抽取 AIManager
- [x] 任务 1: 集成 AIManager 到 RoomMainWidget
- [x] 任务 2: 抽取 RoomManager
- [x] 任务 3: 抽取 MediaManager
- [x] 任务 4: Linux 实现类继承接口 (ExternalVideoSource/AudioSource/AudioRender)
- [x] 任务 5: 创建 ConfigManager 配置管理器
- [x] 任务 6: 创建 Logger 日志系统
- [x] 任务 7: 创建 ErrorCode 和 AppError 统一错误处理
- [x] 任务 8: 创建 StyleManager 样式管理器

### 7.2 待完成

| 优先级 | 任务 | 说明 |
|--------|------|------|
| 中 | 单元测试 | 在 `tests/` 添加 Manager 测试 |
| 低 | Mock 实现 | 在 `drivers/impl/mock/` 添加模拟实现（接口已就绪） |
| 低 | 跨平台支持 | 添加 Windows/macOS 驱动实现（接口已就绪） |

### 7.3 代码行数统计

| 模块 | 重构前 | 重构后 | 变化 |
|------|--------|--------|------|
| RoomMainWidget | ~877 行 | ~665 行 | -212 行 |
| AIManager | - | ~150 行 | +150 行 |
| RoomManager | - | ~115 行 | +115 行 |
| MediaManager | - | ~280 行 | +280 行 |
| ConfigManager | - | ~200 行 | +200 行 |
| Logger | - | ~150 行 | +150 行 |
| ErrorCode | - | ~170 行 | +170 行 |
| StyleManager | - | ~70 行 | +70 行 |

**总结**: 通过抽取 Manager 类和基础设施，RoomMainWidget 代码量减少约 24%，职责更加清晰，同时增加了配置管理、日志、错误处理、样式管理等基础设施。

---

## 8. 开发规范（重要）

### 8.1 日志规范

使用 `Logger` 类统一输出日志，**禁止直接使用 `qDebug()`**。

```cpp
// 在文件开头定义模块名
#define LOG_MODULE "ModuleName"

// 使用宏输出日志
LOG_DEBUG("Debug message");                    // 调试信息
LOG_INFO("Info message");                       // 一般信息
LOG_WARN("Warning message");                    // 警告
LOG_ERROR("Error message");                     // 错误

// 带参数
LOG_INFO(QString("Video config: %1x%2").arg(width).arg(height));
```

**日志级别**:
- `Debug`: 调试信息，生产环境可关闭
- `Info`: 重要状态变化（初始化、启动、停止）
- `Warning`: 可恢复的问题
- `Error`: 错误，需要处理

**输出格式**:
```
[2026-01-16 22:12:22.525] [INFO ] [AIManager      ] AI started successfully
```

### 8.2 错误处理规范

使用 `ErrorCode` 枚举和 `AppError` 类统一错误处理。

```cpp
// 错误码分段
enum class ErrorCode {
    // 通用错误 (0-99)
    Success = 0,
    Unknown = 1,
    
    // RTC 错误 (100-199)
    RtcEngineCreateFailed = 100,
    RtcEngineNotCreated = 101,
    
    // AI 错误 (200-299)
    AiConfigNotLoaded = 200,
    AiStartFailed = 201,
    
    // 媒体错误 (300-399)
    MediaEngineNull = 300,
    
    // 配置错误 (400-499)
    ConfigFileNotFound = 400,
};

// 使用示例
if (!m_engine) {
    LOG_ERROR("Engine not created");
    emit errorOccurred(makeError(ErrorCode::RtcEngineNotCreated));
    return false;
}

// 信号定义
signals:
    void errorOccurred(const AppError& error);  // 不是 QString
```

**添加新错误码**:
1. 在 `src/common/ErrorCode.h` 中添加枚举值
2. 在 `getDefaultMessage()` 中添加默认消息

### 8.3 配置管理规范

使用 `ConfigManager` 统一管理配置，**禁止硬编码配置项**。

```cpp
// 获取配置
QString appId = ConfigManager::instance()->appId();
QString serverUrl = ConfigManager::instance()->serverUrl();
int videoWidth = ConfigManager::instance()->videoWidth();

// 配置文件: config/config.json
{
    "rtc": {
        "appId": "xxx",
        "appKey": "xxx",
        "serverUrl": "http://localhost:3001"
    },
    "media": {
        "preferredAudioDevice": "USB",
        "video": { "width": 640, "height": 480, "frameRate": 15 }
    },
    "ui": {
        "useGPURendering": true
    }
}
```

**添加新配置项**:
1. 在 `ConfigManager.h` 中添加成员变量和 getter
2. 在 `loadFromFile()` 中解析 JSON
3. 在 `loadDefaults()` 中设置默认值
4. 更新 `config/config.json` 模板

### 8.4 样式规范

使用 `StyleManager` 管理样式，新组件应使用 QSS 文件。

```cpp
// 应用全局样式
StyleManager::instance()->applyToWidget(this);

// 使用 objectName 选择器
m_slider->setObjectName("micVolumeSlider");

// 在 dark.qss 中定义样式
QSlider#micVolumeSlider::handle:horizontal {
    background: #4CAF50;
}
```

**样式文件位置**: `src/ui/styles/dark.qss`

**颜色常量**:
- 主色 (Primary): `#4CAF50` 绿色
- 次色 (Secondary): `#2196F3` 蓝色
- 强调色 (Accent): `#635BFF` 紫色
- 背景色: `rgba(0, 0, 0, 0.7)`
- 文字色: `#FFFFFF`

### 8.5 代码风格

- **命名**: 类名 `PascalCase`，成员变量 `m_camelCase`，函数 `camelCase`
- **缩进**: 4 空格
- **头文件**: 使用 `#pragma once`
- **注释**: 中文注释，关键逻辑需要说明

### 8.6 Git 提交规范

```
<type>: <subject>

<body>
```

**Type**:
- `feat`: 新功能
- `fix`: 修复 bug
- `refactor`: 重构
- `docs`: 文档
- `style`: 代码格式
- `test`: 测试

---

## 9. 协作指南

### 9.1 模块负责人

| 模块 | 负责人 | 说明 |
|------|--------|------|
| app/ | - | 应用层协调 |
| ui/widgets/ | - | 业务 UI |
| ui/components/ | - | 通用组件 |
| core/api/ | - | AIGC API |
| core/rtc/ | - | RTC 相关 |
| core/media/ | - | 媒体管理 |
| drivers/ | - | 硬件驱动 |

### 9.2 开发流程

1. 从 `dev` 分支创建功能分支
2. 开发并测试
3. 提交 PR 到 `dev`
4. Code Review
5. 合并到 `dev`
6. 定期合并 `dev` 到 `master`

### 9.3 联调测试

1. 启动 Server: `cd Server && npm start`
2. 启动客户端: `./scripts/run.sh`
3. 测试各模式切换
4. 测试摄像头切换
5. 测试音量控制

---

## 10. 常见问题

### Q1: 摄像头无法启动
检查 GStreamer 是否正确安装，运行 `gst-launch-1.0 libcamerasrc ! fakesink` 测试。

### Q2: 音频无声音
检查 ALSA 设备，运行 `aplay -l` 查看可用设备。

### Q3: AI 无法启动
检查 Server 是否运行，确认 `http://localhost:3001` 可访问。

### Q4: 编译报错找不到头文件
确认 CMakeLists.txt 中 include 路径正确，运行 `cmake ..` 重新配置。

---

*文档版本: 2.0*
*更新日期: 2026-01-16*
*工程化重构完成: 任务 1-8*
