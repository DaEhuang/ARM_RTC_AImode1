#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>
#include <QLabel>
#include <atomic>
#include "rtc/bytertc_video_defines.h"
#include "rtc/bytertc_video_frame.h"

class VideoRenderWidgetGL;

/**
 * 自定义视频渲染器
 * 用于在 Wayland 环境下渲染 RTC 视频帧
 * 支持两种模式：
 * 1. CPU 模式：转换为 QImage 显示（兼容旧代码）
 * 2. GPU 模式：直接传递 I420 数据到 OpenGL Widget
 */
class CustomVideoSink : public bytertc::IVideoSink {
public:
    CustomVideoSink(QWidget* renderWidget);
    ~CustomVideoSink();

    // IVideoSink 接口实现
    bool onFrame(bytertc::IVideoFrame* video_frame) override;
    int getRenderElapse() override;
    void release() override;

    // 获取当前帧用于显示 (CPU 模式)
    QImage getCurrentFrame();
    
    // 清除当前帧
    void clearFrame();
    
    // 设置渲染目标 widget (CPU 模式)
    void setRenderWidget(QWidget* widget);
    
    // 设置 OpenGL 渲染 widget (GPU 模式)
    void setGLRenderWidget(VideoRenderWidgetGL* widget);
    
    // 启用/禁用 GPU 模式
    void setUseGPU(bool useGPU);
    bool isUsingGPU() const { return m_useGPU; }

private:
    void convertI420ToRGB(bytertc::IVideoFrame* frame, QImage& image);

private:
    QWidget* m_renderWidget = nullptr;
    VideoRenderWidgetGL* m_glRenderWidget = nullptr;
    QImage m_currentFrame;
    QMutex m_mutex;
    std::atomic<int> m_renderElapse{0};
    bool m_useGPU = false;
};
