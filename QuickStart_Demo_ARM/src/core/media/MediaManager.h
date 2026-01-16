#pragma once

#include <QObject>
#include "bytertc_engine.h"
#include "drivers/interfaces/IVideoSource.h"

class ExternalVideoSource;
class ExternalAudioSource;
class ExternalAudioRender;

/**
 * 媒体管理器
 * 负责音视频设备的初始化、启动、停止
 */
class MediaManager : public QObject {
    Q_OBJECT

public:
    explicit MediaManager(QObject* parent = nullptr);
    ~MediaManager();

    // 初始化（需要 RTC 引擎）
    void initialize(bytertc::IRTCEngine* engine);
    
    // 启动/停止所有媒体
    void startAll();
    void stopAll();
    
    // 视频控制
    void startVideoCapture();
    void stopVideoCapture();
    bool isVideoCapturing() const;
    
    // 音频采集控制
    void startAudioCapture();
    void stopAudioCapture();
    bool isAudioCapturing() const;
    
    // 音频播放控制
    void startAudioRender();
    void stopAudioRender();
    bool isAudioRendering() const;
    
    // 摄像头管理
    QList<CameraInfo> detectCameras();
    void setCamera(const CameraInfo& camera);
    CameraInfo currentCamera() const;
    
    // 音量控制
    void setMicVolume(int volume);
    int getMicVolume() const;
    void setSpeakerVolume(int volume);
    int getSpeakerVolume() const;
    void setSpeakerMute(bool mute);
    bool isSpeakerMuted() const;
    
    // 获取组件（供外部使用）
    ExternalVideoSource* getVideoSource() const { return m_videoSource; }
    ExternalAudioSource* getAudioSource() const { return m_audioSource; }
    ExternalAudioRender* getAudioRender() const { return m_audioRender; }

signals:
    void cameraError(const QString& error);
    void audioError(const QString& error);

private:
    void setupAudioDevices();
    
    bytertc::IRTCEngine* m_engine = nullptr;
    ExternalVideoSource* m_videoSource = nullptr;
    ExternalAudioSource* m_audioSource = nullptr;
    ExternalAudioRender* m_audioRender = nullptr;
};
