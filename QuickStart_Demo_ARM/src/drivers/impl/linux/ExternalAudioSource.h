#pragma once

#include <QThread>
#include <QMutex>
#include <atomic>
#include "bytertc_engine.h"
#include "rtc/bytertc_audio_frame.h"
#include "drivers/interfaces/IAudioSource.h"

/**
 * 外部音频源
 * 使用 ALSA 直接采集音频并推送给 RTC SDK
 * 解决树莓派上 SDK 无法枚举音频设备的问题
 * 
 * 实现 IAudioSource 接口
 */
class ExternalAudioSource : public QThread, public IAudioSource {
    Q_OBJECT

public:
    ExternalAudioSource(QObject* parent = nullptr);
    ~ExternalAudioSource() override;

    void setRTCEngine(bytertc::IRTCEngine* engine);
    
    // IAudioSource 接口实现
    void startCapture() override;
    void stopCapture() override;
    bool isCapturing() const override;
    void setVolume(int volume) override;
    int getVolume() const override;

protected:
    void run() override;

private:
    bytertc::IRTCEngine* m_rtcEngine = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_volume{100};  // 0-100
    QMutex m_mutex;
};
