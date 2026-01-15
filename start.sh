#!/bin/bash

echo "正在关闭占用端口的进程..."

# 关闭 3000 端口（前端）
fuser -k 3000/tcp 2>/dev/null && echo "端口 3000 已释放" || echo "端口 3000 未被占用"

# 关闭 3001 端口（Server）
fuser -k 3001/tcp 2>/dev/null && echo "端口 3001 已释放" || echo "端口 3001 未被占用"

echo ""
echo "启动 AIGC Demo..."

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

# 启动 Server（后台运行）
echo "启动 Server (端口 3001)..."
node "$SCRIPT_DIR/Server/app.js" &
SERVER_PID=$!

# 等待 Server 启动
sleep 2

# 获取树莓派 IP 地址
IP_ADDR=$(hostname -I | awk '{print $1}')
echo ""
echo "========================================"
echo "前端地址: http://$IP_ADDR:3000"
echo "========================================"
echo ""

# 在树莓派桌面上打开浏览器 (后台运行，等待前端启动)
(
  sleep 8
  export DISPLAY=:0
  chromium-browser --start-fullscreen "http://localhost:3000" 2>/dev/null || \
  chromium --start-fullscreen "http://localhost:3000" 2>/dev/null || \
  firefox "http://localhost:3000" 2>/dev/null || \
  echo "无法打开浏览器，请手动访问 http://$IP_ADDR:3000"
) &

# 启动前端 (禁用 npm 自动打开浏览器)
echo "启动前端 (端口 3000)..."
cd "$SCRIPT_DIR" && BROWSER=none npm run start
