#pragma once

#include <QObject>
#include <QList>
#include <QString>

/**
 * 摄像头信息结构
 */
struct CameraInfo {
    QString id;         // 唯一标识: "CSI" 或 "USB:0", "USB:2" 等
    QString name;       // 显示名称
    QString type;       // "CSI" 或 "USB"
    int deviceIndex;    // USB摄像头的 /dev/video 索引
    
    bool operator==(const CameraInfo& other) const {
        return id == other.id;
    }
};

/**
 * 视频源接口
 * 抽象视频采集功能，支持不同平台实现
 */
class IVideoSource : public QObject {
    Q_OBJECT

public:
    explicit IVideoSource(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IVideoSource() = default;

    // 采集控制
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    
    // 摄像头管理
    virtual QList<CameraInfo> detectCameras() = 0;
    virtual void setCamera(const CameraInfo& camera) = 0;
    virtual CameraInfo currentCamera() const = 0;

signals:
    void cameraError(const QString& error);
    void frameReady(const QByteArray& frame, int width, int height);
};
