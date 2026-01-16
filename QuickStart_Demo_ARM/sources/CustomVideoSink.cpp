#include "CustomVideoSink.h"
#include "VideoRenderWidgetGL.h"
#include <QDebug>
#include <QMetaObject>
#include <chrono>

CustomVideoSink::CustomVideoSink(QWidget* renderWidget)
    : m_renderWidget(renderWidget) {
}

CustomVideoSink::~CustomVideoSink() {
}

bool CustomVideoSink::onFrame(bytertc::IVideoFrame* video_frame) {
    if (!video_frame) {
        return false;
    }
    
    // 检查渲染目标
    if (!m_useGPU && !m_renderWidget) {
        return false;
    }
    if (m_useGPU && !m_glRenderWidget) {
        return false;
    }

    auto start = std::chrono::high_resolution_clock::now();

    int width = video_frame->width();
    int height = video_frame->height();
    
    static int frameCount = 0;
    if (frameCount++ % 30 == 0) {
        qDebug() << "CustomVideoSink::onFrame - received frame" << frameCount << "size:" << width << "x" << height
                 << "GPU:" << m_useGPU;
    }
    
    if (width <= 0 || height <= 0) {
        return false;
    }

    bytertc::VideoPixelFormat format = video_frame->pixelFormat();
    
    // GPU 模式：直接传递 I420 数据到 OpenGL Widget
    if (m_useGPU && format == bytertc::kVideoPixelFormatI420) {
        uint8_t* yPlane = video_frame->planeData(0);
        uint8_t* uPlane = video_frame->planeData(1);
        uint8_t* vPlane = video_frame->planeData(2);
        int yStride = video_frame->planeStride(0);
        int uStride = video_frame->planeStride(1);
        int vStride = video_frame->planeStride(2);
        
        // 在主线程更新 OpenGL 纹理
        QMetaObject::invokeMethod(m_glRenderWidget, [=]() {
            m_glRenderWidget->updateI420Frame(yPlane, uPlane, vPlane,
                                              width, height, yStride, uStride, vStride);
        }, Qt::QueuedConnection);
        
        auto end = std::chrono::high_resolution_clock::now();
        m_renderElapse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        return true;
    }

    // CPU 模式：转换为 QImage
    bytertc::VideoRotation rotation = video_frame->rotation();
    bool needSwap = (rotation == bytertc::kVideoRotation90 || rotation == bytertc::kVideoRotation270);
    int displayWidth = needSwap ? height : width;
    int displayHeight = needSwap ? width : height;

    QImage image(displayWidth, displayHeight, QImage::Format_RGB888);
    
    if (format == bytertc::kVideoPixelFormatI420) {
        convertI420ToRGB(video_frame, image);
    } else if (format == bytertc::kVideoPixelFormatRGBA) {
        // RGBA 格式直接复制
        uint8_t* rgba = video_frame->planeData(0);
        int stride = video_frame->planeStride(0);
        for (int y = 0; y < height; y++) {
            uint8_t* src = rgba + y * stride;
            uint8_t* dst = image.scanLine(y);
            for (int x = 0; x < width; x++) {
                dst[x * 3 + 0] = src[x * 4 + 0]; // R
                dst[x * 3 + 1] = src[x * 4 + 1]; // G
                dst[x * 3 + 2] = src[x * 4 + 2]; // B
            }
        }
    } else {
        qDebug() << "Unsupported pixel format:" << format;
        return false;
    }

    // 处理旋转
    if (rotation != bytertc::kVideoRotation0) {
        QTransform transform;
        transform.rotate(rotation);
        image = image.transformed(transform);
    }

    {
        QMutexLocker locker(&m_mutex);
        m_currentFrame = image;
    }

    // 在主线程更新 UI
    QMetaObject::invokeMethod(m_renderWidget, "update", Qt::QueuedConnection);

    auto end = std::chrono::high_resolution_clock::now();
    m_renderElapse = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();

    return true;
}

void CustomVideoSink::convertI420ToRGB(bytertc::IVideoFrame* frame, QImage& image) {
    int width = frame->width();
    int height = frame->height();
    
    uint8_t* yPlane = frame->planeData(0);
    uint8_t* uPlane = frame->planeData(1);
    uint8_t* vPlane = frame->planeData(2);
    
    int yStride = frame->planeStride(0);
    int uStride = frame->planeStride(1);
    int vStride = frame->planeStride(2);

    for (int y = 0; y < height; y++) {
        uint8_t* dst = image.scanLine(y);
        for (int x = 0; x < width; x++) {
            int yIdx = y * yStride + x;
            int uvIdx_u = (y / 2) * uStride + (x / 2);
            int uvIdx_v = (y / 2) * vStride + (x / 2);
            
            int Y = yPlane[yIdx];
            int U = uPlane[uvIdx_u] - 128;
            int V = vPlane[uvIdx_v] - 128;
            
            // YUV to RGB conversion
            int R = Y + 1.402 * V;
            int G = Y - 0.344 * U - 0.714 * V;
            int B = Y + 1.772 * U;
            
            // Clamp values
            R = qBound(0, R, 255);
            G = qBound(0, G, 255);
            B = qBound(0, B, 255);
            
            dst[x * 3 + 0] = R;
            dst[x * 3 + 1] = G;
            dst[x * 3 + 2] = B;
        }
    }
}

int CustomVideoSink::getRenderElapse() {
    return m_renderElapse.load();
}

void CustomVideoSink::release() {
    QMutexLocker locker(&m_mutex);
    m_currentFrame = QImage();
}

QImage CustomVideoSink::getCurrentFrame() {
    QMutexLocker locker(&m_mutex);
    return m_currentFrame;
}

void CustomVideoSink::setRenderWidget(QWidget* widget) {
    m_renderWidget = widget;
}

void CustomVideoSink::setGLRenderWidget(VideoRenderWidgetGL* widget) {
    m_glRenderWidget = widget;
}

void CustomVideoSink::setUseGPU(bool useGPU) {
    m_useGPU = useGPU;
    qDebug() << "CustomVideoSink: GPU mode" << (useGPU ? "enabled" : "disabled");
}
