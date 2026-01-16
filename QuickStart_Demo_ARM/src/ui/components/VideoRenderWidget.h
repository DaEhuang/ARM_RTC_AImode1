#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>

class CustomVideoSink;

/**
 * 视频渲染 Widget
 * 用于显示 CustomVideoSink 接收到的视频帧
 */
class VideoRenderWidget : public QWidget {
    Q_OBJECT

public:
    VideoRenderWidget(QWidget* parent = nullptr);
    ~VideoRenderWidget();

    void setVideoSink(CustomVideoSink* sink);
    CustomVideoSink* getVideoSink() const;
    
    // 清除画面（显示黑屏）
    void clearFrame();

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    CustomVideoSink* m_videoSink = nullptr;
};
