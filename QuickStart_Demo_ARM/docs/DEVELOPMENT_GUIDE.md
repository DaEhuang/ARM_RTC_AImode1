# QuickStart_Demo_ARM 开发规范指南

本文档定义了项目的开发规范，所有开发人员必须遵守。

---

## 1. 日志规范

### 1.1 基本原则

- **禁止** 直接使用 `qDebug()`, `qWarning()`, `qCritical()`
- **必须** 使用 `Logger` 类统一输出日志

### 1.2 使用方法

```cpp
// 1. 在文件开头定义模块名
#define LOG_MODULE "ModuleName"

// 2. 包含头文件
#include "Logger.h"

// 3. 使用宏输出日志
LOG_DEBUG("Debug message");     // 调试信息
LOG_INFO("Info message");       // 一般信息
LOG_WARN("Warning message");    // 警告
LOG_ERROR("Error message");     // 错误

// 4. 带参数的日志
LOG_INFO(QString("Video config: %1x%2@%3fps")
    .arg(width).arg(height).arg(fps));
```

### 1.3 日志级别说明

| 级别 | 用途 | 示例 |
|------|------|------|
| `Debug` | 调试信息，生产环境可关闭 | 变量值、流程跟踪 |
| `Info` | 重要状态变化 | 初始化完成、启动、停止 |
| `Warning` | 可恢复的问题 | 配置缺失使用默认值 |
| `Error` | 错误，需要处理 | 引擎创建失败、连接断开 |

### 1.4 输出格式

```
[2026-01-16 22:12:22.525] [INFO ] [AIManager      ] AI started successfully
[2026-01-16 22:12:22.530] [ERROR] [RoomManager    ] Failed to create engine
```

### 1.5 模块名命名

| 模块 | LOG_MODULE |
|------|------------|
| 主程序 | `"Main"` |
| AI 管理 | `"AIManager"` |
| 房间管理 | `"RoomManager"` |
| 媒体管理 | `"MediaManager"` |
| 配置管理 | `"ConfigManager"` |
| 视频源 | `"VideoSource"` |
| 音频源 | `"AudioSource"` |
| 音频渲染 | `"AudioRender"` |

---

## 2. 错误处理规范

### 2.1 基本原则

- **禁止** 使用 `QString` 传递错误信息
- **必须** 使用 `ErrorCode` 枚举和 `AppError` 类

### 2.2 错误码分段

```cpp
enum class ErrorCode {
    // 通用错误 (0-99)
    Success = 0,
    Unknown = 1,
    InvalidParameter = 2,
    NotInitialized = 3,
    
    // RTC 错误 (100-199)
    RtcEngineCreateFailed = 100,
    RtcEngineNotCreated = 101,
    RtcEngineAlreadyExists = 102,
    RtcRoomCreateFailed = 110,
    RtcRoomJoinFailed = 111,
    
    // AI 错误 (200-299)
    AiConfigNotLoaded = 200,
    AiStartFailed = 201,
    AiStopFailed = 202,
    AiServerError = 203,
    
    // 媒体错误 (300-399)
    MediaEngineNull = 300,
    MediaCameraNotFound = 301,
    MediaAudioDeviceNotFound = 302,
    
    // 配置错误 (400-499)
    ConfigFileNotFound = 400,
    ConfigParseError = 401,
};
```

### 2.3 使用方法

```cpp
#include "ErrorCode.h"

// 错误检查和发送
if (!m_engine) {
    LOG_ERROR("Engine not created");
    emit errorOccurred(makeError(ErrorCode::RtcEngineNotCreated));
    return false;
}

// 带详情的错误
emit errorOccurred(makeError(ErrorCode::AiStartFailed, serverResponse));

// 信号定义（使用 AppError，不是 QString）
signals:
    void errorOccurred(const AppError& error);

// 槽函数接收
void MyWidget::onError(const AppError& error) {
    LOG_ERROR(error.toString());
    QMessageBox::warning(this, "错误", error.message());
}
```

### 2.4 添加新错误码

1. 在 `src/common/ErrorCode.h` 中添加枚举值（按分段）
2. 在 `AppError::getDefaultMessage()` 中添加中文消息
3. 更新本文档的错误码表

---

## 3. 配置管理规范

### 3.1 基本原则

- **禁止** 硬编码配置项（AppId、ServerUrl 等）
- **必须** 使用 `ConfigManager` 获取配置

### 3.2 使用方法

```cpp
#include "ConfigManager.h"

// 获取配置
QString appId = ConfigManager::instance()->appId();
QString serverUrl = ConfigManager::instance()->serverUrl();
int videoWidth = ConfigManager::instance()->videoWidth();
int videoHeight = ConfigManager::instance()->videoHeight();
int frameRate = ConfigManager::instance()->videoFrameRate();
QString audioDevice = ConfigManager::instance()->preferredAudioDevice();
bool useGPU = ConfigManager::instance()->useGPURendering();
```

### 3.3 配置文件结构

配置文件位置：`config/config.json`

```json
{
    "rtc": {
        "appId": "68a569ad8f31e0018b7fbd8d",
        "appKey": "011cde79231e42ae9e89fefd777c2604",
        "serverUrl": "http://localhost:3001",
        "inputRegex": "^[a-zA-Z0-9@._-]{1,128}$"
    },
    "media": {
        "preferredAudioDevice": "USB",
        "video": {
            "width": 640,
            "height": 480,
            "frameRate": 15
        }
    },
    "ui": {
        "useGPURendering": true
    }
}
```

### 3.4 添加新配置项

1. 在 `ConfigManager.h` 中添加成员变量和 getter 方法
2. 在 `ConfigManager::loadDefaults()` 中设置默认值
3. 在 `ConfigManager::loadFromFile()` 中解析 JSON
4. 更新 `config/config.json` 模板
5. 更新本文档

---

## 4. 样式规范

### 4.1 基本原则

- 新组件**应该**使用 QSS 文件定义样式
- 使用 `objectName` 选择器区分组件
- 颜色使用统一的颜色常量

### 4.2 样式文件

样式文件位置：`src/ui/styles/dark.qss`

### 4.3 使用方法

```cpp
#include "StyleManager.h"

// 应用全局样式
StyleManager::instance()->applyToWidget(this);

// 设置 objectName 以便 QSS 选择
m_slider->setObjectName("micVolumeSlider");
m_button->setObjectName("modeButton");
```

### 4.4 QSS 选择器示例

```css
/* 通过 objectName 选择 */
QSlider#micVolumeSlider::handle:horizontal {
    background: #4CAF50;
}

QPushButton#modeButton:checked {
    background-color: rgba(99, 91, 255, 0.8);
}

/* 通过类名选择 */
QLabel#statusLabel {
    color: rgba(255, 255, 255, 0.7);
    font-size: 16px;
}
```

### 4.5 颜色常量

| 名称 | 颜色值 | 用途 |
|------|--------|------|
| Primary | `#4CAF50` | 主色，绿色，麦克风相关 |
| Secondary | `#2196F3` | 次色，蓝色，扬声器相关 |
| Accent | `#635BFF` | 强调色，紫色，AI 相关 |
| Background | `rgba(0, 0, 0, 0.7)` | 半透明背景 |
| Text | `#FFFFFF` | 主文字色 |
| TextSecondary | `rgba(255, 255, 255, 0.7)` | 次要文字色 |

### 4.6 代码中获取颜色

```cpp
QString primary = StyleManager::primaryColor();     // #4CAF50
QString secondary = StyleManager::secondaryColor(); // #2196F3
QString accent = StyleManager::accentColor();       // #635BFF
```

---

## 5. 代码风格规范

### 5.1 命名规范

| 类型 | 规范 | 示例 |
|------|------|------|
| 类名 | PascalCase | `RoomManager`, `VideoSource` |
| 成员变量 | m_camelCase | `m_engine`, `m_isRunning` |
| 局部变量 | camelCase | `videoWidth`, `frameCount` |
| 函数名 | camelCase | `startCapture()`, `getVolume()` |
| 常量 | UPPER_SNAKE_CASE | `MAX_RETRY_COUNT` |
| 枚举值 | PascalCase | `ErrorCode::Success` |

### 5.2 文件规范

- 头文件使用 `#pragma once`
- 一个类一个文件（.h + .cpp）
- 文件名与类名一致

### 5.3 缩进与格式

- 缩进：4 空格（不使用 Tab）
- 大括号：同行风格

```cpp
if (condition) {
    // code
} else {
    // code
}
```

### 5.4 注释规范

- 使用中文注释
- 关键逻辑需要说明
- 类和公共方法需要文档注释

```cpp
/**
 * 媒体管理器
 * 负责音视频设备的初始化、启动、停止
 */
class MediaManager : public QObject {
    // ...
};
```

---

## 6. Git 提交规范

### 6.1 提交格式

```
<type>: <subject>

<body>
```

### 6.2 Type 类型

| Type | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | `feat: 添加摄像头切换功能` |
| `fix` | 修复 bug | `fix: 修复音频无声问题` |
| `refactor` | 重构 | `refactor: 抽取 MediaManager` |
| `docs` | 文档 | `docs: 更新开发规范` |
| `style` | 代码格式 | `style: 统一缩进` |
| `test` | 测试 | `test: 添加 RoomManager 测试` |
| `chore` | 构建/工具 | `chore: 更新 CMakeLists` |

### 6.3 提交示例

```
refactor: 任务5 - 创建 ConfigManager 配置管理器

- 创建 ConfigManager 单例类管理所有配置
- 支持从 JSON 文件加载配置
- RTC 配置: appId, appKey, serverUrl
- 媒体配置: preferredAudioDevice, video width/height/frameRate
- 修改 RoomMainWidget 使用 ConfigManager
```

---

## 7. 分支策略

| 分支 | 用途 | 说明 |
|------|------|------|
| `master` | 主分支 | 稳定版本，可部署 |
| `dev` | 开发分支 | 日常开发 |
| `feature/*` | 功能分支 | 新功能开发 |
| `bugfix/*` | 修复分支 | Bug 修复 |
| `release/*` | 发布分支 | 版本发布准备 |

### 7.1 开发流程

1. 从 `dev` 创建功能分支：`git checkout -b feature/xxx dev`
2. 开发并测试
3. 提交 PR 到 `dev`
4. Code Review
5. 合并到 `dev`
6. 定期合并 `dev` 到 `master`

---

## 8. 目录结构规范

```
src/
├── main.cpp              # 程序入口
├── app/                  # 应用层
│   ├── RoomMainWidget.*  # 主窗口
│   └── AIManager.*       # AI 管理器
├── ui/                   # UI 层
│   ├── widgets/          # 业务 Widget
│   ├── components/       # 通用组件
│   └── styles/           # QSS 样式
├── core/                 # 核心业务层
│   ├── api/              # API 模块
│   ├── rtc/              # RTC 模块
│   ├── media/            # 媒体模块
│   └── config/           # 配置模块
├── drivers/              # 驱动层
│   ├── interfaces/       # 抽象接口
│   └── impl/             # 平台实现
│       ├── linux/        # Linux ARM
│       └── mock/         # Mock 实现
└── common/               # 公共模块
    ├── Logger.*          # 日志
    ├── ErrorCode.h       # 错误码
    ├── AIMode.h          # AI 模式枚举
    └── Constants.h       # 常量（已废弃，使用 ConfigManager）
```

### 8.1 新增文件规则

| 类型 | 位置 |
|------|------|
| 新 Manager | `src/core/xxx/` 或 `src/app/` |
| 新 Widget | `src/ui/widgets/` |
| 新通用组件 | `src/ui/components/` |
| 新驱动接口 | `src/drivers/interfaces/` |
| 新驱动实现 | `src/drivers/impl/linux/` |
| 新公共类 | `src/common/` |

---

## 9. 快速检查清单

开发新功能时，请检查：

- [ ] 日志使用 `LOG_*` 宏，不是 `qDebug()`
- [ ] 错误使用 `ErrorCode` + `AppError`，不是 `QString`
- [ ] 配置从 `ConfigManager` 获取，不是硬编码
- [ ] 样式使用 `objectName` + QSS，不是内联 `setStyleSheet()`
- [ ] 类名和文件名使用 PascalCase
- [ ] 成员变量使用 `m_` 前缀
- [ ] 提交信息符合规范

---

*文档版本: 1.0*
*创建日期: 2026-01-16*
