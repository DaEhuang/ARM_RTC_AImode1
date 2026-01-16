#include "VideoRenderWidgetGL.h"
#include "CustomVideoSink.h"
#include <QDebug>

// I420 到 RGB 的 shader - 在 GPU 上进行颜色空间转换
static const char* vertexShaderSource = R"(
    attribute vec4 aPosition;
    attribute vec2 aTexCoord;
    varying vec2 vTexCoord;
    void main() {
        gl_Position = aPosition;
        vTexCoord = aTexCoord;
    }
)";

static const char* fragmentShaderSource = R"(
    varying highp vec2 vTexCoord;
    uniform sampler2D yTexture;
    uniform sampler2D uTexture;
    uniform sampler2D vTexture;
    void main() {
        highp float y = texture2D(yTexture, vTexCoord).r;
        highp float u = texture2D(uTexture, vTexCoord).r - 0.5;
        highp float v = texture2D(vTexture, vTexCoord).r - 0.5;
        
        // BT.601 YUV to RGB conversion
        highp float r = y + 1.402 * v;
        highp float g = y - 0.344 * u - 0.714 * v;
        highp float b = y + 1.772 * u;
        
        gl_FragColor = vec4(r, g, b, 1.0);
    }
)";

VideoRenderWidgetGL::VideoRenderWidgetGL(QWidget* parent)
    : QOpenGLWidget(parent) {
    setAttribute(Qt::WA_OpaquePaintEvent);
}

VideoRenderWidgetGL::~VideoRenderWidgetGL() {
    makeCurrent();
    deleteTextures();
    delete m_program;
    doneCurrent();
}

void VideoRenderWidgetGL::updateI420Frame(const uint8_t* yData, const uint8_t* uData, const uint8_t* vData,
                                          int width, int height, int yStride, int uStride, int vStride) {
    QMutexLocker locker(&m_mutex);
    
    // 复制数据
    int ySize = yStride * height;
    int uSize = uStride * (height / 2);
    int vSize = vStride * (height / 2);
    
    m_yData.resize(ySize);
    m_uData.resize(uSize);
    m_vData.resize(vSize);
    
    memcpy(m_yData.data(), yData, ySize);
    memcpy(m_uData.data(), uData, uSize);
    memcpy(m_vData.data(), vData, vSize);
    
    m_frameWidth = width;
    m_frameHeight = height;
    m_yStride = yStride;
    m_uStride = uStride;
    m_vStride = vStride;
    m_frameReady = true;
    
    // 触发重绘
    update();
}

void VideoRenderWidgetGL::initializeGL() {
    initializeOpenGLFunctions();
    
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    
    initShaders();
    
    qDebug() << "VideoRenderWidgetGL: OpenGL initialized, version:" << (const char*)glGetString(GL_VERSION);
}

void VideoRenderWidgetGL::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void VideoRenderWidgetGL::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    QMutexLocker locker(&m_mutex);
    
    if (!m_frameReady || m_frameWidth == 0 || m_frameHeight == 0) {
        return;
    }
    
    // 如果分辨率变化，重新创建纹理
    if (!m_texturesCreated || m_textureWidth != m_frameWidth || m_textureHeight != m_frameHeight) {
        createTextures(m_frameWidth, m_frameHeight);
    }
    
    if (!m_program || !m_texturesCreated) {
        return;
    }
    
    m_program->bind();
    
    // 更新 Y 纹理
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_yStride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_frameWidth, m_frameHeight, 
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, m_yData.constData());
    m_program->setUniformValue("yTexture", 0);
    
    // 更新 U 纹理
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_uStride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_frameWidth / 2, m_frameHeight / 2,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, m_uData.constData());
    m_program->setUniformValue("uTexture", 1);
    
    // 更新 V 纹理
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, m_vStride);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_frameWidth / 2, m_frameHeight / 2,
                    GL_LUMINANCE, GL_UNSIGNED_BYTE, m_vData.constData());
    m_program->setUniformValue("vTexture", 2);
    
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    
    // 计算填充整个屏幕的纹理坐标（裁剪超出部分）
    float widgetAspect = (float)width() / height();
    float videoAspect = (float)m_frameWidth / m_frameHeight;
    
    // 纹理坐标偏移，用于居中裁剪
    float texLeft = 0.0f, texRight = 1.0f;
    float texTop = 0.0f, texBottom = 1.0f;
    
    if (videoAspect > widgetAspect) {
        // 视频更宽，裁剪左右
        float scale = widgetAspect / videoAspect;
        float offset = (1.0f - scale) / 2.0f;
        texLeft = offset;
        texRight = 1.0f - offset;
    } else {
        // 视频更高，裁剪上下
        float scale = videoAspect / widgetAspect;
        float offset = (1.0f - scale) / 2.0f;
        texTop = offset;
        texBottom = 1.0f - offset;
    }
    
    // 顶点数据: 位置(x,y) + 纹理坐标(s,t) - 全屏填充
    GLfloat vertices[] = {
        -1.0f, -1.0f,  texLeft,  texBottom,  // 左下
         1.0f, -1.0f,  texRight, texBottom,  // 右下
        -1.0f,  1.0f,  texLeft,  texTop,     // 左上
         1.0f,  1.0f,  texRight, texTop,     // 右上
    };
    
    // 设置顶点属性
    int posLoc = m_program->attributeLocation("aPosition");
    int texLoc = m_program->attributeLocation("aTexCoord");
    
    glVertexAttribPointer(posLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices);
    glVertexAttribPointer(texLoc, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), vertices + 2);
    
    glEnableVertexAttribArray(posLoc);
    glEnableVertexAttribArray(texLoc);
    
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    glDisableVertexAttribArray(posLoc);
    glDisableVertexAttribArray(texLoc);
    
    m_program->release();
}

void VideoRenderWidgetGL::initShaders() {
    m_program = new QOpenGLShaderProgram(this);
    
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource)) {
        qDebug() << "VideoRenderWidgetGL: Failed to compile vertex shader:" << m_program->log();
        return;
    }
    
    if (!m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource)) {
        qDebug() << "VideoRenderWidgetGL: Failed to compile fragment shader:" << m_program->log();
        return;
    }
    
    if (!m_program->link()) {
        qDebug() << "VideoRenderWidgetGL: Failed to link shader program:" << m_program->log();
        return;
    }
    
    qDebug() << "VideoRenderWidgetGL: Shaders compiled and linked successfully";
}

void VideoRenderWidgetGL::createTextures(int width, int height) {
    deleteTextures();
    
    // 创建 Y 纹理 (全分辨率)
    glGenTextures(1, &m_textureY);
    glBindTexture(GL_TEXTURE_2D, m_textureY);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width, height, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 创建 U 纹理 (半分辨率)
    glGenTextures(1, &m_textureU);
    glBindTexture(GL_TEXTURE_2D, m_textureU);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    // 创建 V 纹理 (半分辨率)
    glGenTextures(1, &m_textureV);
    glBindTexture(GL_TEXTURE_2D, m_textureV);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, width / 2, height / 2, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    m_textureWidth = width;
    m_textureHeight = height;
    m_texturesCreated = true;
    
    qDebug() << "VideoRenderWidgetGL: Created textures for" << width << "x" << height;
}

void VideoRenderWidgetGL::clearFrame() {
    QMutexLocker locker(&m_mutex);
    m_frameReady = false;
    m_yData.clear();
    m_uData.clear();
    m_vData.clear();
    update();
}

void VideoRenderWidgetGL::deleteTextures() {
    if (m_textureY) {
        glDeleteTextures(1, &m_textureY);
        m_textureY = 0;
    }
    if (m_textureU) {
        glDeleteTextures(1, &m_textureU);
        m_textureU = 0;
    }
    if (m_textureV) {
        glDeleteTextures(1, &m_textureV);
        m_textureV = 0;
    }
    m_texturesCreated = false;
}
