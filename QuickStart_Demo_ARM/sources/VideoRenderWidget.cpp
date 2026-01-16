#include "VideoRenderWidget.h"
#include "CustomVideoSink.h"
#include <QPainter>

VideoRenderWidget::VideoRenderWidget(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
}

VideoRenderWidget::~VideoRenderWidget() {
}

void VideoRenderWidget::setVideoSink(CustomVideoSink* sink) {
    m_videoSink = sink;
}

CustomVideoSink* VideoRenderWidget::getVideoSink() const {
    return m_videoSink;
}

void VideoRenderWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    if (m_videoSink) {
        QImage frame = m_videoSink->getCurrentFrame();
        if (!frame.isNull()) {
            // 使用 KeepAspectRatioByExpanding 填充整个 widget，可能会裁剪
            QImage scaled = frame.scaled(size(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            
            // 居中绘制（超出部分会被裁剪）
            int x = (width() - scaled.width()) / 2;
            int y = (height() - scaled.height()) / 2;
            
            painter.setClipRect(rect());
            painter.fillRect(rect(), Qt::black);
            painter.drawImage(x, y, scaled);
            return;
        }
    }
    
    // 没有视频帧时显示黑色背景
    painter.fillRect(rect(), Qt::black);
}
