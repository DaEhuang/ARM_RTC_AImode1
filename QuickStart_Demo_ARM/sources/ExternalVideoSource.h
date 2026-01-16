#pragma once

#include <QThread>
#include <QMutex>
#include <QList>
#include <QPair>
#include <atomic>
#include <string>
#include "bytertc_engine.h"
#include "rtc/bytertc_video_frame.h"

#include <gst/gst.h>
#include <gst/app/gstappsink.h>

/**
 * 摄像头信息结构
 */
struct CameraInfo {
    QString id;         // 唯一标识: "CSI" 或 "USB:0", "USB:2" 等
    QString name;       // 显示名称
    QString type;       // "CSI" 或 "USB"
    int deviceIndex;    // USB摄像头的 /dev/video 索引
};

/**
 * 外部视频源 (GStreamer 版本)
 * 支持 CSI 摄像头 (libcamerasrc) 和 USB 摄像头 (v4l2src)
 * 使用 GStreamer 统一管道架构
 */
class ExternalVideoSource : public QThread {
    Q_OBJECT

public:
    ExternalVideoSource(QObject* parent = nullptr);
    ~ExternalVideoSource();

    void setRTCEngine(bytertc::IRTCEngine* engine);
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
