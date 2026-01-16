#pragma once

/**
 * 音频渲染接口
 * 抽象音频播放功能，支持不同平台实现
 * 
 * 注意：这是纯抽象接口，不继承 QObject 以避免菱形继承
 * 实现类需要自己继承 QObject/QThread 并定义信号
 */
class IAudioRender {
public:
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
};
