#!/bin/bash

# 获取脚本所在目录
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"

export LD_LIBRARY_PATH="$SCRIPT_DIR/VolcEngineRTC_arm/lib:$LD_LIBRARY_PATH"
# 强制Qt使用XCB(X11)平台，通过XWayland运行
export QT_QPA_PLATFORM=xcb

cd "$SCRIPT_DIR/build"
./QuickStart 2>&1 | tee /tmp/quickstart.log
