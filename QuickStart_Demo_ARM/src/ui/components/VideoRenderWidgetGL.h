#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QMutex>

class CustomVideoSink;

/**
 * OpenGL 视频渲染 Widget
 * 使用 GPU 进行 I420→RGB 转换和显示
 * 相比 QImage+QPainter 方式，CPU 负载接近零
 */
class VideoRenderWidgetGL : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT

public:
    VideoRenderWidgetGL(QWidget* parent = nullptr);
    ~VideoRenderWidgetGL();

    void setVideoSink(CustomVideoSink* sink) { m_videoSink = sink; }
    CustomVideoSink* getVideoSink() const { return m_videoSink; }
    
    // 直接更新 I420 数据（避免 CPU 转换）
    void updateI420Frame(const uint8_t* yData, const uint8_t* uData, const uint8_t* vData,
                         int width, int height, int yStride, int uStride, int vStride);

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    void initShaders();
    void createTextures(int width, int height);
    void deleteTextures();

private:
    CustomVideoSink* m_videoSink = nullptr;
    
    // OpenGL 资源
    QOpenGLShaderProgram* m_program = nullptr;
    GLuint m_textureY = 0;
    GLuint m_textureU = 0;
    GLuint m_textureV = 0;
    
    // 视频帧数据
    QMutex m_mutex;
    QByteArray m_yData;
    QByteArray m_uData;
    QByteArray m_vData;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
    int m_yStride = 0;
    int m_uStride = 0;
    int m_vStride = 0;
    bool m_frameReady = false;
    bool m_texturesCreated = false;
    int m_textureWidth = 0;
    int m_textureHeight = 0;
};
