#include "MediaManager.h"
#include "ExternalVideoSource.h"
#include "ExternalAudioSource.h"
#include "ExternalAudioRender.h"
#include "rtc/bytertc_audio_device_manager.h"
#include <QDebug>

MediaManager::MediaManager(QObject* parent)
    : QObject(parent)
{
}

MediaManager::~MediaManager()
{
    stopAll();
}

void MediaManager::initialize(bytertc::IRTCEngine* engine)
{
    m_engine = engine;
    
    if (!m_engine) {
        qWarning() << "MediaManager: Engine is null";
        return;
    }
    
    // 配置音频设备
    setupAudioDevices();
    
    // 设置视频编码参数
    bytertc::VideoEncoderConfig conf;
    conf.frame_rate = 15;
    conf.width = 360;
    conf.height = 640;
    m_engine->setVideoEncoderConfig(conf);
    
    // 使用外部视频源
    m_engine->setVideoSourceType(bytertc::kVideoSourceTypeExternal);
    qDebug() << "MediaManager: Set video source type to external";
    
    // 创建视频源
    if (!m_videoSource) {
        m_videoSource = new ExternalVideoSource(this);
        connect(m_videoSource, &ExternalVideoSource::cameraError, this, &MediaManager::cameraError);
    }
    m_videoSource->setRTCEngine(m_engine);
    
    // 尝试使用外部音频源
    int audioSourceRet = m_engine->setAudioSourceType(bytertc::kAudioSourceTypeExternal);
    qDebug() << "MediaManager: setAudioSourceType(External) ret:" << audioSourceRet;
    
    if (audioSourceRet == 0) {
        // 外部音频源可用
        if (!m_audioSource) {
            m_audioSource = new ExternalAudioSource(this);
        }
        m_audioSource->setRTCEngine(m_engine);
    } else {
        qDebug() << "MediaManager: External audio source not available, using internal";
    }
    
    // 创建音频渲染
    if (!m_audioRender) {
        m_audioRender = new ExternalAudioRender(this);
    }
    m_audioRender->setRTCEngine(m_engine);
    
    qDebug() << "MediaManager: Initialized";
}

void MediaManager::startAll()
{
    startVideoCapture();
    startAudioCapture();
    startAudioRender();
}

void MediaManager::stopAll()
{
    stopVideoCapture();
    stopAudioCapture();
    stopAudioRender();
}

void MediaManager::startVideoCapture()
{
    if (m_videoSource) {
        m_videoSource->startCapture();
        qDebug() << "MediaManager: Video capture started";
    }
}

void MediaManager::stopVideoCapture()
{
    if (m_videoSource) {
        m_videoSource->stopCapture();
        qDebug() << "MediaManager: Video capture stopped";
    }
}

bool MediaManager::isVideoCapturing() const
{
    return m_videoSource && m_videoSource->isCapturing();
}

void MediaManager::startAudioCapture()
{
    if (m_audioSource) {
        m_audioSource->startCapture();
        qDebug() << "MediaManager: Audio capture started";
    } else if (m_engine) {
        m_engine->startAudioCapture();
        qDebug() << "MediaManager: Internal audio capture started";
    }
}

void MediaManager::stopAudioCapture()
{
    if (m_audioSource) {
        m_audioSource->stopCapture();
        qDebug() << "MediaManager: Audio capture stopped";
    }
}

bool MediaManager::isAudioCapturing() const
{
    return m_audioSource && m_audioSource->isCapturing();
}

void MediaManager::startAudioRender()
{
    if (m_audioRender) {
        m_audioRender->startRender();
        qDebug() << "MediaManager: Audio render started";
    }
}

void MediaManager::stopAudioRender()
{
    if (m_audioRender) {
        m_audioRender->stopRender();
        qDebug() << "MediaManager: Audio render stopped";
    }
}

bool MediaManager::isAudioRendering() const
{
    return m_audioRender && m_audioRender->isRendering();
}

QList<CameraInfo> MediaManager::detectCameras()
{
    return ExternalVideoSource::detectCamerasStatic();
}

void MediaManager::setCamera(const CameraInfo& camera)
{
    if (!m_videoSource) {
        qDebug() << "MediaManager: Video source not initialized";
        return;
    }
    
    bool wasCapturing = m_videoSource->isCapturing();
    if (wasCapturing) {
        m_videoSource->stopCapture();
        QThread::msleep(100);
    }
    
    m_videoSource->setCamera(camera);
    
    if (wasCapturing) {
        m_videoSource->startCapture();
    }
    
    qDebug() << "MediaManager: Camera switched to:" << camera.name;
}

CameraInfo MediaManager::currentCamera() const
{
    if (m_videoSource) {
        return m_videoSource->currentCamera();
    }
    return CameraInfo();
}

void MediaManager::setMicVolume(int volume)
{
    if (m_audioSource) {
        m_audioSource->setVolume(volume);
        qDebug() << "MediaManager: Mic volume set to:" << volume;
    }
}

int MediaManager::getMicVolume() const
{
    return m_audioSource ? m_audioSource->getVolume() : 100;
}

void MediaManager::setSpeakerVolume(int volume)
{
    if (m_audioRender) {
        m_audioRender->setVolume(volume);
        qDebug() << "MediaManager: Speaker volume set to:" << volume;
    }
}

int MediaManager::getSpeakerVolume() const
{
    return m_audioRender ? m_audioRender->getVolume() : 100;
}

void MediaManager::setSpeakerMute(bool mute)
{
    if (m_audioRender) {
        m_audioRender->setMute(mute);
        qDebug() << "MediaManager: Speaker mute set to:" << mute;
    }
}

bool MediaManager::isSpeakerMuted() const
{
    return m_audioRender && m_audioRender->isMuted();
}

void MediaManager::setupAudioDevices()
{
    if (!m_engine) {
        qDebug() << "MediaManager::setupAudioDevices: RTC engine is null";
        return;
    }
    
    bytertc::IAudioDeviceManager* audioManager = m_engine->getAudioDeviceManager();
    if (!audioManager) {
        qDebug() << "MediaManager::setupAudioDevices: Failed to get audio device manager";
        return;
    }
    
    // 不跟随系统设置，手动选择设备
    audioManager->followSystemCaptureDevice(false);
    audioManager->followSystemPlaybackDevice(false);
    
    // 枚举并选择音频采集设备 (麦克风)
    bytertc::IDeviceCollection* captureDevices = audioManager->enumerateAudioCaptureDevices();
    if (captureDevices) {
        int count = captureDevices->getCount();
        qDebug() << "MediaManager: Found" << count << "audio capture devices";
        
        for (int i = 0; i < count; i++) {
            char deviceId[512] = {0};
            char deviceName[512] = {0};
            captureDevices->getDevice(i, deviceName, deviceId);
            qDebug() << "  Capture device" << i << ":" << deviceName << "(" << deviceId << ")";
            
            // 选择 USB 音频设备
            QString name = QString::fromUtf8(deviceName);
            if (name.contains("USB", Qt::CaseInsensitive) || 
                name.contains("Yundea", Qt::CaseInsensitive) ||
                name.contains("M1066", Qt::CaseInsensitive)) {
                audioManager->setAudioCaptureDevice(deviceId);
                qDebug() << "MediaManager: Selected capture device:" << deviceName;
                break;
            }
        }
        captureDevices->release();
    }
    
    // 枚举并选择音频播放设备 (扬声器)
    bytertc::IDeviceCollection* playbackDevices = audioManager->enumerateAudioPlaybackDevices();
    if (playbackDevices) {
        int count = playbackDevices->getCount();
        qDebug() << "MediaManager: Found" << count << "audio playback devices";
        
        for (int i = 0; i < count; i++) {
            char deviceId[512] = {0};
            char deviceName[512] = {0};
            playbackDevices->getDevice(i, deviceName, deviceId);
            qDebug() << "  Playback device" << i << ":" << deviceName << "(" << deviceId << ")";
            
            // 选择 USB 音频设备
            QString name = QString::fromUtf8(deviceName);
            if (name.contains("USB", Qt::CaseInsensitive) || 
                name.contains("Yundea", Qt::CaseInsensitive) ||
                name.contains("M1066", Qt::CaseInsensitive)) {
                audioManager->setAudioPlaybackDevice(deviceId);
                qDebug() << "MediaManager: Selected playback device:" << deviceName;
                break;
            }
        }
        playbackDevices->release();
    }
}
