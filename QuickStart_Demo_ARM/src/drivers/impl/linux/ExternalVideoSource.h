#pragma once

#include <QThread>
#include <QMutex>
#include <QList>
#include <QPair>
#include <atomic>
#include <string>
#include "bytertc_engine.h"
#include "rtc/bytertc_video_frame.h"
#include "drivers/interfaces/IVideoSource.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

/**
 * 外部视频源 (GStreamer 版本)
 * 支持 CSI 摄像头 (libcamerasrc) 和 USB 摄像头 (v4l2src)
 * 使用 GStreamer 统一管道架构
 * 
 * 实现 IVideoSource 接口
 */
class ExternalVideoSource : public QThread {
    Q_OBJECT

public:
    ExternalVideoSource(QObject* parent = nullptr);
    ~ExternalVideoSource();

    void setRTCEngine(bytertc::IRTCEngine* engine);
    
    // IVideoSource 接口实现
    void startCapture();
    void stopCapture();
    bool isCapturing() const;
    
    // 摄像头管理
    static QList<CameraInfo> detectCameras();
    void setCamera(const CameraInfo& camera);
    CameraInfo currentCamera() const;
    
signals:
    void cameraError(const QString& error);

protected:
    void run() override;

private:
    bool initGStreamer();
    void cleanupGStreamer();
    QString buildPipelineString();
    
    bytertc::IRTCEngine* m_rtcEngine = nullptr;
    std::atomic<bool> m_running{false};
    QMutex m_mutex;
    CameraInfo m_currentCamera;
    
    // GStreamer
    GstElement* m_pipeline = nullptr;
    GstElement* m_appsink = nullptr;
    static bool s_gstInitialized;
};
