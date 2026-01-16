# ARM RTC AIGC Demo

基于树莓派 5 的实时 AI 音视频交互 Demo，使用火山引擎 RTC + AIGC 服务。

## 项目结构

```
├── QuickStart_Demo_ARM/    # Qt/C++ 客户端 (ARM64)
│   ├── sources/            # 源代码
│   ├── ui/                 # Qt UI 文件
│   └── VolcEngineRTC_arm/  # RTC SDK (ARM64)
├── Server/                 # Node.js 后端服务
│   ├── app.js              # 主服务
│   └── scenes/             # 场景配置
└── start-arm-aigc.sh       # 一键启动脚本
```

## 环境要求

- **硬件**: 树莓派 5 (ARM64)
- **系统**: Raspberry Pi OS (64-bit)
- **Node.js**: 16.0+
- **Qt**: 5.x

## 服务开通

开通 ASR、TTS、LLM、RTC 等服务，参考 [开通服务](https://www.volcengine.com/docs/6348/1315561?s=g)。

## 场景配置

编辑 `Server/scenes/Custom.json`（可参考 `Custom.json.template`）：

- **AccountConfig**: AK/SK，从 https://console.volcengine.com/iam/keymanage/ 获取
- **RTCConfig**: AppId/AppKey，从 https://console.volcengine.com/rtc/aigc/listRTC 获取
- **VoiceChat**: AIGC 配置，参考 https://www.volcengine.com/docs/6348/1558163

## 快速开始

### 1. 安装 Server 依赖

```bash
cd Server
npm install
```

### 2. 编译 Qt 客户端

```bash
cd QuickStart_Demo_ARM
mkdir -p build && cd build
cmake ..
make -j4
```

### 3. 一键启动

```bash
./start-arm-aigc.sh
```

## 功能特性

- **GPU 渲染**: 使用 OpenGL ES 进行视频渲染
- **外部音视频源**: 支持 libcamera + PulseAudio
- **AI 对话**: 实时语音对话，字幕显示
- **模式切换**: 闲聊、监督、教学、待机

## 相关文档

- [场景介绍](https://www.volcengine.com/docs/6348/1310537?s=g)
- [场景搭建方案](https://www.volcengine.com/docs/6348/1310560?s=g)