#include "ExternalAudioRender.h"
#include <QDebug>
#include <chrono>
#include <cstring>
#include <thread>
#include <stdio.h>
#include <cstdlib>

ExternalAudioRender::ExternalAudioRender(QObject* parent)
    : QThread(parent) {
}

ExternalAudioRender::~ExternalAudioRender() {
    stopRender();
}

void ExternalAudioRender::setRTCEngine(bytertc::IRTCEngine* engine) {
    m_rtcEngine = engine;
}

void ExternalAudioRender::startRender() {
    if (m_running) {
        return;
    }
    
    if (!m_rtcEngine) {
        qDebug() << "ExternalAudioRender: RTC engine is null";
        return;
    }
    
    // 注册音频帧观察者
    int ret = m_rtcEngine->registerAudioFrameObserver(this);
    qDebug() << "ExternalAudioRender: registerAudioFrameObserver ret:" << ret;
    
    // 启用 Playback 回调（远端音频）
    bytertc::AudioFormat format;
    format.sample_rate = bytertc::kAudioSampleRate48000;
    format.channel = bytertc::kAudioChannelStereo;
    ret = m_rtcEngine->enableAudioFrameCallback(bytertc::AudioFrameCallbackMethod::kPlayback, format);
    qDebug() << "ExternalAudioRender: enableAudioFrameCallback(kPlayback) ret:" << ret;
    
    m_running = true;
    start();
}

void ExternalAudioRender::stopRender() {
    m_running = false;
    
    if (m_rtcEngine) {
        m_rtcEngine->disableAudioFrameCallback(bytertc::AudioFrameCallbackMethod::kPlayback);
        m_rtcEngine->registerAudioFrameObserver(nullptr);
    }
    
    if (isRunning()) {
        wait(3000);
    }
}

bool ExternalAudioRender::isRendering() const {
    return m_running;
}

void ExternalAudioRender::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);
}

int ExternalAudioRender::getVolume() const {
    return m_volume;
}

void ExternalAudioRender::setMute(bool mute) {
    m_muted = mute;
}

bool ExternalAudioRender::isMuted() const {
    return m_muted;
}

void ExternalAudioRender::onPlaybackAudioFrame(const bytertc::IAudioFrame& audio_frame) {
    // 在 SDK 回调线程中接收远端音频数据
    uint8_t* data = audio_frame.data();
    int dataSize = audio_frame.dataSize();
    
    if (data && dataSize > 0) {
        QMutexLocker locker(&m_mutex);
        
        // 防止队列过大
        while (m_audioQueue.size() >= MAX_QUEUE_SIZE) {
            m_audioQueue.dequeue();
        }
        
        // 复制音频数据到队列
        QByteArray audioData(reinterpret_cast<const char*>(data), dataSize);
        m_audioQueue.enqueue(audioData);
    }
}

void ExternalAudioRender::run() {
    qDebug() << "ExternalAudioRender: starting audio render thread...";

    // 使用 popen 直接写入 aplay
    // 格式: 48000Hz, 立体声, 16-bit signed little-endian
    FILE* aplayPipe = popen("aplay -D hw:1,0 -f S16_LE -r 48000 -c 2 -t raw -q 2>/dev/null", "w");
    
    if (!aplayPipe) {
        qDebug() << "ExternalAudioRender: failed to start aplay";
        return;
    }
    
    qDebug() << "ExternalAudioRender: aplay started";
    
    int frameCount = 0;
    int emptyCount = 0;

    while (m_running) {
        QByteArray audioData;
        
        {
            QMutexLocker locker(&m_mutex);
            if (!m_audioQueue.isEmpty()) {
                audioData = m_audioQueue.dequeue();
            }
        }
        
        if (!audioData.isEmpty()) {
            // 检查静音状态
            if (m_muted.load()) {
                // 静音时跳过播放
                frameCount++;
                emptyCount = 0;
                continue;
            }
            
            // 应用音量调节
            int volume = m_volume.load();
            if (volume != 100) {
                int16_t* samples = reinterpret_cast<int16_t*>(audioData.data());
                int sampleCount = audioData.size() / 2;
                for (int i = 0; i < sampleCount; i++) {
                    int32_t sample = samples[i] * volume / 100;
                    // 防止溢出
                    if (sample > 32767) sample = 32767;
                    if (sample < -32768) sample = -32768;
                    samples[i] = static_cast<int16_t>(sample);
                }
            }
            
            // 写入音频数据到 aplay
            size_t written = fwrite(audioData.data(), 1, audioData.size(), aplayPipe);
            fflush(aplayPipe);
            
            frameCount++;
            emptyCount = 0;
            
            if (frameCount % 50 == 0) {  // 每秒打印一次 (50 * 20ms = 1s)
                // 检查数据是否全为0
                int16_t* samples = reinterpret_cast<int16_t*>(audioData.data());
                int sampleCount = audioData.size() / 2;
                int maxSample = 0;
                for (int i = 0; i < sampleCount; i++) {
                    int absSample = abs(samples[i]);
                    if (absSample > maxSample) maxSample = absSample;
                }
                qDebug() << "ExternalAudioRender: played frame" << frameCount 
                         << "size:" << audioData.size() << "maxSample:" << maxSample;
            }
        } else {
            emptyCount++;
            if (emptyCount % 500 == 0) {  // 每10秒打印一次
                qDebug() << "ExternalAudioRender: no audio data in queue";
            }
        }
        
        // 等待 20ms（回调周期是 20ms）
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    }
    
    pclose(aplayPipe);
    
    qDebug() << "ExternalAudioRender: render stopped, total frames:" << frameCount;
}
