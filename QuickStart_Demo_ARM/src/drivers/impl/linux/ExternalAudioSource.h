#pragma once

#include <QThread>
#include <QMutex>
#include <atomic>
#include "bytertc_engine.h"
#include "rtc/bytertc_audio_frame.h"

/**
 * 外部音频源
 * 使用 ALSA 直接采集音频并推送给 RTC SDK
 * 解决树莓派上 SDK 无法枚举音频设备的问题
 */
class ExternalAudioSource : public QThread {
    Q_OBJECT

public:
    ExternalAudioSource(QObject* parent = nullptr);
    ~ExternalAudioSource();

    void setRTCEngine(bytertc::IRTCEngine* engine);
    void startCapture();
    void stopCapture();
    bool isCapturing() const;
    
    // 音量控制 (0-100)
    void setVolume(int volume);
    int getVolume() const;

protected:
    void run() override;

private:
    bytertc::IRTCEngine* m_rtcEngine = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_volume{100};  // 0-100
    QMutex m_mutex;
};
