#pragma once

#include <QObject>

/**
 * 音频渲染接口
 * 抽象音频播放功能，支持不同平台实现
 */
class IAudioRender : public QObject {
    Q_OBJECT

public:
    explicit IAudioRender(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAudioRender() = default;

    // 渲染控制
    virtual void startRender() = 0;
    virtual void stopRender() = 0;
    virtual bool isRendering() const = 0;
    
    // 音量控制 (0-100)
    virtual void setVolume(int volume) = 0;
    virtual int getVolume() const = 0;
    
    // 静音控制
    virtual void setMute(bool mute) = 0;
    virtual bool isMuted() const = 0;
    
    // 推送音频数据
    virtual void pushAudioData(const QByteArray& data, int sampleRate, int channels) = 0;

signals:
    void audioError(const QString& error);
};
