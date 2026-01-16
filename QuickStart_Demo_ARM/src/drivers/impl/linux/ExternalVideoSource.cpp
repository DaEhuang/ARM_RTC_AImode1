#include "ExternalVideoSource.h"
#include <QDebug>
#include <QProcess>
#include <QRegularExpression>
#include <chrono>
#include <cstring>

// 静态成员初始化
bool ExternalVideoSource::s_gstInitialized = false;

ExternalVideoSource::ExternalVideoSource(QObject* parent)
    : QThread(parent) {
    // 默认使用 CSI 摄像头
    m_currentCamera.id = "CSI";
    m_currentCamera.name = "CSI 摄像头";
    m_currentCamera.type = "CSI";
    m_currentCamera.deviceIndex = -1;
    
    // 初始化 GStreamer (只需要一次)
    if (!s_gstInitialized) {
        gst_init(nullptr, nullptr);
        s_gstInitialized = true;
        qDebug() << "GStreamer initialized, version:" << gst_version_string();
    }
}

ExternalVideoSource::~ExternalVideoSource() {
    stopCapture();
    cleanupGStreamer();
}

void ExternalVideoSource::setRTCEngine(bytertc::IRTCEngine* engine) {
    m_rtcEngine = engine;
}

void ExternalVideoSource::startCapture() {
    if (m_running) {
        return;
    }
    m_running = true;
    start();
}

void ExternalVideoSource::stopCapture() {
    m_running = false;
    
    // 停止 GStreamer pipeline
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
    }
    
    if (isRunning()) {
        wait(3000);
    }
    
    cleanupGStreamer();
}

bool ExternalVideoSource::isCapturing() const {
    return m_running;
}

void ExternalVideoSource::setCamera(const CameraInfo& camera) {
    QMutexLocker locker(&m_mutex);
    m_currentCamera = camera;
    qDebug() << "ExternalVideoSource: camera set to" << camera.name << "type:" << camera.type;
}

CameraInfo ExternalVideoSource::currentCamera() const {
    return m_currentCamera;
}

QList<CameraInfo> ExternalVideoSource::detectCamerasStatic() {
    QList<CameraInfo> cameras;
    
    // 确保 GStreamer 已初始化
    if (!s_gstInitialized) {
        gst_init(nullptr, nullptr);
        s_gstInitialized = true;
    }
    
    // 1. 检测 CSI 摄像头 (通过 GStreamer libcamerasrc)
    GstElement* testPipeline = gst_parse_launch("libcamerasrc ! fakesink", nullptr);
    if (testPipeline) {
        GstStateChangeReturn ret = gst_element_set_state(testPipeline, GST_STATE_READY);
        if (ret != GST_STATE_CHANGE_FAILURE) {
            CameraInfo csi;
            csi.id = "CSI";
            csi.name = "CSI 摄像头 (libcamera)";
            csi.type = "CSI";
            csi.deviceIndex = -1;
            cameras.append(csi);
            qDebug() << "ExternalVideoSource: detected CSI camera via GStreamer";
        }
        gst_element_set_state(testPipeline, GST_STATE_NULL);
        gst_object_unref(testPipeline);
    }
    
    // 2. 检测 USB 摄像头 (使用 v4l2-ctl)
    QProcess v4l2Check;
    v4l2Check.start("v4l2-ctl", QStringList() << "--list-devices");
    if (v4l2Check.waitForFinished(5000)) {
        QString output = v4l2Check.readAllStandardOutput();
        QStringList lines = output.split('\n');
        
        QString currentDevice;
        bool firstVideoFound = false;
        
        for (const QString& line : lines) {
            // 设备名称行（不以空白开头）
            if (!line.isEmpty() && !line.startsWith('\t') && !line.startsWith(' ')) {
                currentDevice = line.trimmed();
                firstVideoFound = false;
            }
            // video 设备路径行
            else if (line.trimmed().startsWith("/dev/video") && !firstVideoFound) {
                QString videoPath = line.trimmed();
                
                // 排除非USB摄像头设备
                if (!currentDevice.isEmpty()) {
                    // 排除: CSI摄像头(rp1-cfe)、PISP后端(pispbe)、解码器(hevc)
                    QStringList skipKeywords = {"rp1-cfe", "pispbe", "hevc-dec", "bcm2835"};
                    bool skip = false;
                    for (const QString& kw : skipKeywords) {
                        if (currentDevice.toLower().contains(kw)) {
                            skip = true;
                            break;
                        }
                    }
                    
                    if (!skip) {
                        // 提取 video 索引
                        QRegularExpression re("/dev/video(\\d+)");
                        QRegularExpressionMatch match = re.match(videoPath);
                        if (match.hasMatch()) {
                            int videoNum = match.captured(1).toInt();
                            
                            CameraInfo usb;
                            usb.id = QString("USB:%1").arg(videoNum);
                            
                            // 提取设备名称
                            QString deviceName = currentDevice.split('(').first().trimmed();
                            if (deviceName.contains(':')) {
                                deviceName = deviceName.split(':').first().trimmed();
                            }
                            usb.name = QString("%1 (video%2)").arg(deviceName).arg(videoNum);
                            usb.type = "USB";
                            usb.deviceIndex = videoNum;
                            
                            cameras.append(usb);
                            firstVideoFound = true;
                            qDebug() << "ExternalVideoSource: detected USB camera:" << usb.name;
                        }
                    }
                }
            }
        }
    }
    
    qDebug() << "ExternalVideoSource: total cameras detected:" << cameras.size();
    return cameras;
}

QList<CameraInfo> ExternalVideoSource::detectCameras() {
    return detectCamerasStatic();
}

QString ExternalVideoSource::buildPipelineString() {
    QString pipeline;
    
    if (m_currentCamera.type == "USB") {
        // USB 摄像头使用 v4l2src
        // 不指定严格的 framerate，让 v4l2src 自动选择
        // 使用 videorate 转换到目标帧率
        pipeline = QString(
            "v4l2src device=/dev/video%1 ! "
            "video/x-raw,width=640,height=480 ! "
            "videorate ! video/x-raw,framerate=15/1 ! "
            "videoconvert ! "
            "video/x-raw,format=I420 ! "
            "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true"
        ).arg(m_currentCamera.deviceIndex);
    } else {
        // CSI 摄像头使用 libcamerasrc
        // libcamerasrc 默认输出较大分辨率，需要 videoscale 缩放
        pipeline = QString(
            "libcamerasrc ! "
            "videoconvert ! "
            "videoscale ! video/x-raw,width=640,height=480 ! "
            "videorate ! video/x-raw,framerate=15/1 ! "
            "videoconvert ! "
            "video/x-raw,format=I420 ! "
            "appsink name=sink emit-signals=true sync=false max-buffers=2 drop=true"
        );
    }
    
    return pipeline;
}

bool ExternalVideoSource::initGStreamer() {
    QString pipelineStr = buildPipelineString();
    qDebug() << "ExternalVideoSource: creating pipeline:" << pipelineStr;
    
    GError* error = nullptr;
    m_pipeline = gst_parse_launch(pipelineStr.toUtf8().constData(), &error);
    
    if (error) {
        qDebug() << "ExternalVideoSource: failed to create pipeline:" << error->message;
        emit cameraError(QString("GStreamer 管道创建失败: %1").arg(error->message));
        g_error_free(error);
        return false;
    }
    
    if (!m_pipeline) {
        qDebug() << "ExternalVideoSource: pipeline is null";
        emit cameraError("GStreamer 管道为空");
        return false;
    }
    
    // 获取 appsink
    m_appsink = gst_bin_get_by_name(GST_BIN(m_pipeline), "sink");
    if (!m_appsink) {
        qDebug() << "ExternalVideoSource: failed to get appsink";
        emit cameraError("无法获取 appsink");
        cleanupGStreamer();
        return false;
    }
    
    // 启动管道
    GstStateChangeReturn ret = gst_element_set_state(m_pipeline, GST_STATE_PLAYING);
    if (ret == GST_STATE_CHANGE_FAILURE) {
        qDebug() << "ExternalVideoSource: failed to start pipeline";
        emit cameraError("无法启动摄像头");
        cleanupGStreamer();
        return false;
    }
    
    qDebug() << "ExternalVideoSource: GStreamer pipeline started";
    return true;
}

void ExternalVideoSource::cleanupGStreamer() {
    if (m_appsink) {
        gst_object_unref(m_appsink);
        m_appsink = nullptr;
    }
    
    if (m_pipeline) {
        gst_element_set_state(m_pipeline, GST_STATE_NULL);
        gst_object_unref(m_pipeline);
        m_pipeline = nullptr;
    }
}

void ExternalVideoSource::run() {
    qDebug() << "ExternalVideoSource: starting GStreamer capture for" << m_currentCamera.name;
    
    if (!m_rtcEngine) {
        qDebug() << "ExternalVideoSource: RTC engine is null";
        emit cameraError("RTC engine is null");
        return;
    }
    
    if (!initGStreamer()) {
        return;
    }
    
    const int width = 640;
    const int height = 480;
    
    int frameCount = 0;
    auto startTime = std::chrono::steady_clock::now();
    
    while (m_running) {
        // 从 appsink 拉取样本
        GstSample* sample = gst_app_sink_try_pull_sample(GST_APP_SINK(m_appsink), 100 * GST_MSECOND);
        
        if (!sample) {
            // 检查是否到达 EOS 或出错
            if (gst_app_sink_is_eos(GST_APP_SINK(m_appsink))) {
                qDebug() << "ExternalVideoSource: EOS reached";
                break;
            }
            continue;
        }
        
        GstBuffer* buffer = gst_sample_get_buffer(sample);
        if (!buffer) {
            gst_sample_unref(sample);
            continue;
        }
        
        GstMapInfo map;
        if (!gst_buffer_map(buffer, &map, GST_MAP_READ)) {
            gst_sample_unref(sample);
            continue;
        }
        
        // 计算 I420 帧各平面大小
        const int ySize = width * height;
        const int uvSize = ySize / 4;
        
        // 构建视频帧数据
        bytertc::VideoFrameData frame;
        frame.buffer_type = bytertc::kVideoBufferTypeRawMemory;
        frame.pixel_format = bytertc::kVideoPixelFormatI420;
        frame.width = width;
        frame.height = height;
        frame.rotation = bytertc::kVideoRotation0;
        
        // 设置时间戳
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
        frame.timestamp_us = elapsed.count();
        
        // 设置平面数据
        frame.number_of_planes = 3;
        frame.plane_data[0] = map.data;                    // Y
        frame.plane_data[1] = map.data + ySize;            // U
        frame.plane_data[2] = map.data + ySize + uvSize;   // V
        frame.plane_stride[0] = width;
        frame.plane_stride[1] = width / 2;
        frame.plane_stride[2] = width / 2;
        
        // 推送帧到 SDK
        int ret = m_rtcEngine->pushExternalVideoFrame(frame);
        
        frameCount++;
        if (frameCount % 30 == 0) {
            qDebug() << "ExternalVideoSource: pushed frame" << frameCount 
                     << "size:" << map.size << "ret:" << ret;
        }
        
        gst_buffer_unmap(buffer, &map);
        gst_sample_unref(sample);
    }
    
    cleanupGStreamer();
    qDebug() << "ExternalVideoSource: capture stopped, total frames:" << frameCount;
}
