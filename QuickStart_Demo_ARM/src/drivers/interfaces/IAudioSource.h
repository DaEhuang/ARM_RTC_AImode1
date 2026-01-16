#pragma once

#include <QObject>

/**
 * 音频源接口
 * 抽象音频采集功能，支持不同平台实现
 */
class IAudioSource : public QObject {
    Q_OBJECT

public:
    explicit IAudioSource(QObject* parent = nullptr) : QObject(parent) {}
    virtual ~IAudioSource() = default;

    // 采集控制
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    
    // 音量控制 (0-100)
    virtual void setVolume(int volume) = 0;
    virtual int getVolume() const = 0;

signals:
    void audioError(const QString& error);
    void audioReady(const QByteArray& data, int sampleRate, int channels);
};
