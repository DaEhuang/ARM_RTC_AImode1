#!/bin/bash

# 资源监控脚本 - 监控 AIGC Demo 相关进程的资源使用情况
# 使用原地更新，不滚动屏幕
# CPU% 使用与任务管理器一致的计算方式（相对于总 CPU）

# 监控间隔（秒）
INTERVAL=${1:-2}

# 获取 CPU 核心数
CPU_CORES=$(nproc)

# 隐藏光标
tput civis

# 退出时恢复光标
trap 'tput cnorm; echo ""; exit 0' INT TERM

# 清屏一次
clear

# 固定行数
LINES=20

while true; do
    # 移动光标到开头
    tput cup 0 0
    
    # 获取数据
    TIME_NOW=$(date '+%H:%M:%S')
    CPU_USAGE=$(top -bn1 | grep "Cpu(s)" | awk '{printf "%.1f", $2}')
    MEM_INFO=$(free -m | awk '/^Mem:/{printf "%d/%dMB (%.0f%%)", $3, $2, $3/$2*100}')
    
    # CPU 温度
    TEMP_C="N/A"
    if [ -f /sys/class/thermal/thermal_zone0/temp ]; then
        TEMP=$(cat /sys/class/thermal/thermal_zone0/temp)
        TEMP_C=$(echo "scale=1; $TEMP/1000" | bc)"°C"
    fi
    
    # 打印标题
    printf "\033[1;32m%-60s\033[0m\n" "═══ AIGC Demo 资源监控 [$TIME_NOW] ═══"
    printf "\033[1;34mCPU:\033[0m %-8s \033[1;34m内存:\033[0m %-20s \033[1;34m温度:\033[0m %-8s \033[1;34m核心:\033[0m %d\n" "${CPU_USAGE}%" "$MEM_INFO" "$TEMP_C" "$CPU_CORES"
    echo "────────────────────────────────────────────────────────────"
    printf "%-16s %7s %7s %9s %8s\n" "进程" "CPU%" "内存%" "内存MB" "PID"
    echo "────────────────────────────────────────────────────────────"
    
    # 进程信息 - 固定5行
    # CPU% 除以核心数，与任务管理器一致
    PROC_COUNT=0
    ps aux --sort=-%cpu | grep -E "(QuickStart|node.*app|arecord|aplay)" | grep -v grep | head -5 | while read line; do
        PID=$(echo $line | awk '{print $2}')
        CPU_RAW=$(echo $line | awk '{print $3}')
        # 除以核心数得到相对于总 CPU 的百分比
        CPU=$(echo "scale=1; $CPU_RAW / $CPU_CORES" | bc)
        MEM=$(echo $line | awk '{print $4}')
        MEM_MB=$(echo $line | awk '{printf "%.1f", $6/1024}')
        
        if echo "$line" | grep -q "QuickStart"; then
            PNAME="QuickStart"
        elif echo "$line" | grep -q "node"; then
            PNAME="AIGC Server"
        elif echo "$line" | grep -q "arecord"; then
            PNAME="音频采集(arecord)"
        elif echo "$line" | grep -q "aplay"; then
            PNAME="音频播放(aplay)"
        else
            PNAME="其他"
        fi
        
        printf "%-16s %7s %7s %9s %8s\n" "$PNAME" "$CPU" "$MEM" "$MEM_MB" "$PID"
    done
    
    # 填充空行保持格式
    for i in $(seq $(ps aux | grep -E "(QuickStart|node.*app|arecord|aplay)" | grep -v grep | wc -l) 4); do
        printf "%-16s %7s %7s %9s %8s\n" "-" "-" "-" "-" "-"
    done
    
    echo "────────────────────────────────────────────────────────────"
    
    # 网络流量
    IFACE=$(ip route | grep default | awk '{print $5}' | head -1)
    if [ -n "$IFACE" ] && [ -f "/sys/class/net/$IFACE/statistics/rx_bytes" ]; then
        RX=$(cat /sys/class/net/$IFACE/statistics/rx_bytes)
        TX=$(cat /sys/class/net/$IFACE/statistics/tx_bytes)
        
        if [ -n "$LAST_RX" ]; then
            RX_RATE=$(echo "scale=1; ($RX-$LAST_RX)/$INTERVAL/1024" | bc)
            TX_RATE=$(echo "scale=1; ($TX-$LAST_TX)/$INTERVAL/1024" | bc)
            printf "\033[1;34m网络(%s):\033[0m 下载 %6s KB/s | 上传 %6s KB/s\n" "$IFACE" "$RX_RATE" "$TX_RATE"
        else
            printf "\033[1;34m网络(%s):\033[0m 计算中...\n" "$IFACE"
        fi
        LAST_RX=$RX
        LAST_TX=$TX
    fi
    
    # GPU 信息
    if command -v vcgencmd &> /dev/null; then
        GPU_MEM=$(vcgencmd get_mem gpu 2>/dev/null | cut -d'=' -f2)
        printf "\033[1;34mGPU内存:\033[0m %-10s\n" "$GPU_MEM"
    fi
    
    echo "────────────────────────────────────────────────────────────"
    printf "\033[1;33m刷新: %d秒 | Ctrl+C 退出\033[0m\n" "$INTERVAL"
    
    # 清除剩余行
    tput el
    
    sleep $INTERVAL
done
