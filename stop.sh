#!/bin/bash

echo "正在关闭 AIGC Demo 服务..."

# 关闭 3000 端口（前端）
fuser -k 3000/tcp 2>/dev/null && echo "前端 (端口 3000) 已关闭" || echo "端口 3000 未运行"

# 关闭 3001 端口（Server）
fuser -k 3001/tcp 2>/dev/null && echo "Server (端口 3001) 已关闭" || echo "端口 3001 未运行"

echo ""
echo "所有服务已停止"
