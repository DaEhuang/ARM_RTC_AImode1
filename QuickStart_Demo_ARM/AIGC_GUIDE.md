# QuickStart_Demo_ARM AIGC 集成指南

## 概述

本指南介绍如何将 Linux ARM 版本的 RTC Demo 升级为 AIGC 实时 AI 视频流应用，与 `Server` 服务端配合使用。

## 架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        火山引擎云端                              │
│  ┌─────────────┐    ┌─────────────┐    ┌─────────────┐         │
│  │     ASR     │ →  │     LLM     │ →  │     TTS     │         │
│  │  语音识别   │    │   大模型    │    │  语音合成   │         │
│  └─────────────┘    └─────────────┘    └─────────────┘         │
│         ↑                                     ↓                 │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                    RTC 房间                              │   │
│  │     用户音频流 ──────────────→ AI 音频流                 │   │
│  └─────────────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────────────┘
         ↑                                     ↓
    ┌─────────┐                          ┌─────────┐
    │ ARM设备 │                          │ AI Bot  │
    │(树莓派) │                          │ (云端)  │
    └─────────┘                          └─────────┘
```

## 新增文件

### 1. AIGCApi.h / AIGCApi.cpp

AIGC API 调用模块，负责与 Server 通信：

```cpp
// 主要接口
class AIGCApi : public QObject {
public:
    // 获取场景配置（RTC 配置）
    void getScenes();
    
    // 启动 AI 对话
    void startVoiceChat();
    
    // 停止 AI 对话
    void stopVoiceChat();
    
    // 获取 RTC 配置
    RTCConfig getRtcConfig() const;

signals:
    void getScenesSuccess(const RTCConfig& config);
    void getScenesFailed(const QString& error);
    void startVoiceChatSuccess();
    void startVoiceChatFailed(const QString& error);
    void stopVoiceChatSuccess();
    void stopVoiceChatFailed(const QString& error);
};
```

## 修改的文件

### 1. OperateWidget.h / OperateWidget.cpp

添加了 AI 控制按钮：

```cpp
// 新增信号
signals:
    void sigStartAI();
    void sigStopAI();

// 新增方法
void setAIStarted(bool started);
void setAILoading(bool loading);
void setAIEnabled(bool enabled);
```

### 2. RoomMainWidget.h / RoomMainWidget.cpp

集成 AIGC 功能：

```cpp
// 新增成员
AIGCApi* m_aigcApi;
bool m_aiStarted;
bool m_useServerConfig;

// 新增槽函数
void slotOnStartAI();
void slotOnStopAI();
void slotOnGetScenesSuccess(const AIGCApi::RTCConfig& config);
void slotOnGetScenesFailed(const QString& error);
void slotOnStartVoiceChatSuccess();
void slotOnStartVoiceChatFailed(const QString& error);
void slotOnStopVoiceChatSuccess();
void slotOnStopVoiceChatFailed(const QString& error);
```

## 使用流程

### 1. 启动 Server

```bash
cd Server
yarn install
yarn dev
```

Server 默认运行在 `http://localhost:3001`

### 2. 配置场景

确保 `Server/scenes/Custom.json` 已正确配置：
- `AccountConfig`: AK/SK 密钥
- `RTCConfig`: AppId, AppKey
- `VoiceChat`: AI 配置参数

### 3. 编译运行 ARM 客户端

```bash
cd QuickStart_Demo_ARM
mkdir build && cd build
cmake ..
make -j4
./QuickStart
```

### 4. 使用 AIGC 功能

1. 程序启动后自动从 Server 获取配置
2. 输入房间号和用户ID，点击"进入房间"
3. 进入房间后，点击绑色的"🤖 启动AI"按钮
4. AI 启动后开始对话
5. 点击红色的"⏹ 停止AI"按钮停止 AI

## UI 变化

操作栏新增了 AI 控制按钮：

```
┌────────────────────────────────────────────────────────┐
│ [🤖 启动AI] [🎤] [📹] [🔊] ──────── [📞挂断]           │
└────────────────────────────────────────────────────────┘
```

- **绿色按钮**: 启动 AI 对话
- **红色按钮**: 停止 AI 对话
- **灰色按钮**: 正在加载中

## 关键配置

### Server 地址配置

默认连接 `localhost:3001`，如需修改：

```cpp
// 在 RoomMainWidget 构造函数中
m_aigcApi = new AIGCApi(this);
m_aigcApi->setServerUrl("http://your-server-ip:3001");
```

### 场景 ID 配置

默认使用 `Custom` 场景，如需修改：

```cpp
m_aigcApi->setSceneId("YourSceneID");
```

## 常见问题

### Q: AI 按钮显示灰色无法点击？
- 检查 Server 是否已启动
- 检查网络连接是否正常
- 查看控制台日志中的错误信息

### Q: AI 启动后没有声音？
- 确保 `isAutoSubscribeAudio` 设置为 `true`
- 检查外部音频渲染是否正常工作
- 确认喇叭没有被静音

### Q: 报错 "The task has been started"？
- 说明之前的 AI 任务还在运行
- 程序会自动尝试停止旧任务再启动新任务

### Q: 如何开启视觉能力？
在 `Server/scenes/Custom.json` 中配置：

```json
"LLMConfig": {
  "VisionConfig": {
    "Enable": true
  }
}
```

## 依赖

- Qt5 Network 模块（用于 HTTP 请求）
- Server 服务端运行中

## 参考

- [React 版本 AIGC 指南](../QuickStart_React_4.66_20/AIGC_GUIDE.md)
- [Server README](../Server/README.md)
- [火山引擎 RTC 文档](https://www.volcengine.com/docs/6348/66812)
- [AIGC 实时对话文档](https://www.volcengine.com/docs/6348/1310537)
