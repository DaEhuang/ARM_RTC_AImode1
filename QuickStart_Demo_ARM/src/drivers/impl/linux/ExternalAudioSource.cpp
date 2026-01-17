#include "ExternalAudioSource.h"
#include <QDebug>
#include <QProcess>
#include <chrono>
#include <cstring>
#include <thread>

ExternalAudioSource::ExternalAudioSource(QObject* parent)
    : QThread(parent) {
}

ExternalAudioSource::~ExternalAudioSource() {
    stopCapture();
}

void ExternalAudioSource::setRTCEngine(bytertc::IRTCEngine* engine) {
    m_rtcEngine = engine;
}

void ExternalAudioSource::startCapture() {
    if (m_running) {
        return;
    }
    m_running = true;
    start();
}

void ExternalAudioSource::stopCapture() {
    m_running = false;
    if (isRunning()) {
        wait(3000);
    }
}

bool ExternalAudioSource::isCapturing() const {
    return m_running;
}

void ExternalAudioSource::setVolume(int volume) {
    m_volume = qBound(0, volume, 100);
}

int ExternalAudioSource::getVolume() const {
    return m_volume;
}

void ExternalAudioSource::run() {
    qDebug() << "ExternalAudioSource: starting audio capture...";
    
    if (!m_rtcEngine) {
        qDebug() << "ExternalAudioSource: RTC engine is null";
        return;
    }

    // 使用 arecord 从 USB 麦克风采集音频
    // 格式: 16000Hz, 单声道, 16-bit signed little-endian
    QProcess process;
    QStringList args;
    args << "-D" << "hw:1,0"      // USB 麦克风设备 (Yundea M1066)
         << "-f" << "S16_LE"      // 16-bit signed little-endian
         << "-r" << "16000"       // 16000 Hz 采样率
         << "-c" << "1"           // 单声道
         << "-t" << "raw"         // 原始 PCM 数据
         << "-q";                 // 静默模式
    
    process.start("arecord", args);
    
    if (!process.waitForStarted(5000)) {
        qDebug() << "ExternalAudioSource: failed to start arecord";
        return;
    }
    
    qDebug() << "ExternalAudioSource: arecord started";

    // 每 10ms 推送一次音频数据
    // 16000 Hz * 10ms = 160 samples
    // 160 samples * 2 bytes (16-bit) = 320 bytes
    const int sampleRate = 16000;
    const int channels = 1;
    const int samplesPerFrame = sampleRate / 100;  // 160 samples per 10ms
    const int bytesPerFrame = samplesPerFrame * channels * 2;  // 320 bytes
    
    QByteArray audioBuffer;
    audioBuffer.reserve(bytesPerFrame * 4);
    
    int frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();
    auto lastPushTime = startTime;

    while (m_running && process.state() == QProcess::Running) {
        // 读取音频数据
        if (process.waitForReadyRead(5)) {
            audioBuffer.append(process.readAllStandardOutput());
        }
        
        // 检查是否有足够的数据推送一帧
        while (audioBuffer.size() >= bytesPerFrame && m_running) {
            // 计算时间戳
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
            
            // 应用音量调节
            int volume = m_volume.load();
            if (volume != 100) {
                int16_t* samples = reinterpret_cast<int16_t*>(audioBuffer.data());
                int sampleCount = bytesPerFrame / 2;
                for (int i = 0; i < sampleCount; i++) {
                    int32_t sample = samples[i] * volume / 100;
                    // 防止溢出
                    if (sample > 32767) sample = 32767;
                    if (sample < -32768) sample = -32768;
                    samples[i] = static_cast<int16_t>(sample);
                }
            }
            
            // 构建音频帧
            bytertc::AudioFrameBuilder builder;
            builder.sample_rate = bytertc::kAudioSampleRate16000;
            builder.channel = bytertc::kAudioChannelMono;
            builder.timestamp_us = elapsed.count();
            builder.data = reinterpret_cast<uint8_t*>(audioBuffer.data());
            builder.data_size = bytesPerFrame;
            builder.deep_copy = true;
            
            bytertc::IAudioFrame* audioFrame = bytertc::buildAudioFrame(builder);
            if (audioFrame) {
                int ret = m_rtcEngine->pushExternalAudioFrame(audioFrame);
                audioFrame->release();
                
                frameCount++;
                if (frameCount % 100 == 0) {  // 每秒打印一次
                    qDebug() << "ExternalAudioSource: pushed audio frame" << frameCount << "ret:" << ret;
                }
            }
            
            // 移除已处理的数据
            audioBuffer.remove(0, bytesPerFrame);
            
            // 控制推送速率，每 10ms 推送一帧
            auto targetTime = lastPushTime + std::chrono::milliseconds(10);
            now = std::chrono::steady_clock::now();
            if (now < targetTime) {
                std::this_thread::sleep_until(targetTime);
            }
            lastPushTime = std::chrono::steady_clock::now();
        }
    }
    
    process.terminate();
    process.waitForFinished(1000);
    
    qDebug() << "ExternalAudioSource: capture stopped, total frames:" << frameCount;
}
