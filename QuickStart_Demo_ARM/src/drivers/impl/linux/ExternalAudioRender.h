#pragma once

#include <QThread>
#include <QMutex>
#include <QQueue>
#include <atomic>
#include "bytertc_engine.h"
#include "rtc/bytertc_audio_frame.h"
#include "drivers/interfaces/IAudioRender.h"

/**
 * 外部音频渲染
 * 通过 IAudioFrameObserver 回调获取远端音频并通过 ALSA 播放
 * 解决树莓派上 SDK 无法枚举音频设备的问题
 * 
 * 实现 IAudioRender 接口
 */
class ExternalAudioRender : public QThread, public IAudioRender, public bytertc::IAudioFrameObserver {
    Q_OBJECT

public:
    ExternalAudioRender(QObject* parent = nullptr);
    ~ExternalAudioRender() override;

    void setRTCEngine(bytertc::IRTCEngine* engine);
    
    // IAudioRender 接口实现
    void startRender() override;
    void stopRender() override;
    bool isRendering() const override;
    void setVolume(int volume) override;
    int getVolume() const override;
    void setMute(bool mute) override;
    bool isMuted() const override;

    // IAudioFrameObserver 回调
    void onRecordAudioFrameOriginal(const bytertc::IAudioFrame& audio_frame) override {}
    void onRecordAudioFrame(const bytertc::IAudioFrame& audio_frame) override {}
    void onPlaybackAudioFrame(const bytertc::IAudioFrame& audio_frame) override;
    void onRemoteUserAudioFrame(const char* stream_id, const bytertc::StreamInfo& stream_info, const bytertc::IAudioFrame& audio_frame) override {}
    void onMixedAudioFrame(const bytertc::IAudioFrame& audio_frame) override {}
    void onCaptureMixedAudioFrame(const bytertc::IAudioFrame& audio_frame) override {}

protected:
    void run() override;

private:
    bytertc::IRTCEngine* m_rtcEngine = nullptr;
    std::atomic<bool> m_running{false};
    std::atomic<int> m_volume{100};  // 0-100
    std::atomic<bool> m_muted{false};
    QMutex m_mutex;
    QQueue<QByteArray> m_audioQueue;
    static const int MAX_QUEUE_SIZE = 50;  // 最大缓冲 500ms
};
