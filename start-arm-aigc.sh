#!/bin/bash

echo "正在关闭占用端口的进程..."

# 关闭 3001 端口（Server）
fuser -k 3001/tcp 2>/dev/null && echo "端口 3001 已释放" || echo "端口 3001 未被占用"

echo ""
echo "启动 ARM AIGC Demo..."

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ARM_DIR="$SCRIPT_DIR/QuickStart_Demo_ARM"
BUILD_DIR="$ARM_DIR/build"

# 检查是否已编译
if [ ! -f "$BUILD_DIR/QuickStart" ]; then
    echo "QuickStart 未编译，正在编译..."
    mkdir -p "$BUILD_DIR"
    cd "$BUILD_DIR"
    cmake .. && make -j4
    if [ $? -ne 0 ]; then
        echo "编译失败！"
        exit 1
    fi
    echo "编译完成"
fi

# 启动 AIGC Server（后台运行）
echo ""
echo "启动 AIGC Server (端口 3001)..."
cd "$SCRIPT_DIR/Server"

# 检查 node_modules
if [ ! -d "node_modules" ]; then
    echo "安装 Server 依赖..."
    yarn install || npm install
fi

node app.js &
SERVER_PID=$!

# 等待 Server 启动
sleep 2

# 检查 Server 是否启动成功
if ! kill -0 $SERVER_PID 2>/dev/null; then
    echo "Server 启动失败！"
    exit 1
fi

echo "Server 已启动 (PID: $SERVER_PID)"

# 获取 IP 地址
IP_ADDR=$(hostname -I | awk '{print $1}')
echo ""
echo "========================================"
echo "AIGC Server: http://$IP_ADDR:3001"
echo "========================================"
echo ""

# 设置环境变量
export LD_LIBRARY_PATH="$ARM_DIR/VolcEngineRTC_arm/lib:$LD_LIBRARY_PATH"

# 检测显示环境
if [ -z "$DISPLAY" ]; then
    # 尝试设置默认 DISPLAY
    export DISPLAY=:0
    echo "设置 DISPLAY=:0"
fi

# 检测是否在 Wayland 环境
if [ -n "$WAYLAND_DISPLAY" ]; then
    echo "检测到 Wayland 环境，使用 wayland 平台"
    export QT_QPA_PLATFORM=wayland
else
    echo "使用 XCB (X11) 平台"
    export QT_QPA_PLATFORM=xcb
fi

# 启动 QuickStart ARM 客户端
echo "启动 QuickStart ARM 客户端..."
cd "$BUILD_DIR"
./QuickStart 2>&1 | tee /tmp/quickstart-arm.log

# 客户端退出后清理
echo ""
echo "正在清理..."
kill $SERVER_PID 2>/dev/null
echo "已停止 Server"
