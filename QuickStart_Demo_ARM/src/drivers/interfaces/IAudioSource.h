#pragma once

/**
 * 音频源接口
 * 抽象音频采集功能，支持不同平台实现
 * 
 * 注意：这是纯抽象接口，不继承 QObject 以避免菱形继承
 * 实现类需要自己继承 QObject/QThread 并定义信号
 */
class IAudioSource {
public:
    virtual ~IAudioSource() = default;

    // 采集控制
    virtual void startCapture() = 0;
    virtual void stopCapture() = 0;
    virtual bool isCapturing() const = 0;
    
    // 音量控制 (0-100)
    virtual void setVolume(int volume) = 0;
    virtual int getVolume() const = 0;
};
