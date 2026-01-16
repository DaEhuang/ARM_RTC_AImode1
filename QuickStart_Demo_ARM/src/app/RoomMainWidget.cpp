#include <QMouseEvent>
#include "RoomMainWidget.h"
#include "LoginWidget.h"
#include "OperateWidget.h"
#include "Constants.h"
#include <QDebug>
#include <vector>
#include <QTimer>
#include "VideoWidget.h"
#include <QMessageBox>
#include "TokenGenerator/TokenGenerator.h"
#include "CustomVideoSink.h"
#include "VideoRenderWidget.h"
#include "VideoRenderWidgetGL.h"
#include "ExternalVideoSource.h"
#include "ExternalAudioSource.h"
#include "ExternalAudioRender.h"
#include "ConversationWidget.h"
#include "ModeWidget.h"
#include "rtc/bytertc_audio_device_manager.h"
#include <QPushButton>
#include <QLabel>
#include <QResizeEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

/**
 * VolcEngineRTC 视频通话的主页面
 * 本示例不限制房间内最大用户数；同时最多渲染四个用户的视频数据（自己和三个远端用户视频数据）；
 *
 * 包含如下简单功能：
 * - 创建引擎
 * - 设置视频发布参数
 * - 渲染自己的视频数据
 * - 创建房间
 * - 加入音视频通话房间
 * - 打开/关闭麦克风
 * - 打开/关闭摄像头
 * - 渲染远端用户的视频数据
 * - 离开房间
 * - 销毁引擎
 *
 * 实现一个基本的音视频通话的流程如下：
 * 1.创建 IRTCVideo 实例。 bytertc::IRTCVideo* bytertc::createRTCVideo(
 *                          const char* app_id,
 *                          bytertc::IRTCVideoEventHandler *event_handler,
 *                          const char* parameters)
 * 2.视频发布端设置推送多路流时各路流的参数，包括分辨率、帧率、码率、缩放模式、网络不佳时的回退策略等。
 *   bytertc::IRTCVideo::setVideoEncoderConfig(
 *       const VideoEncoderConfig& max_solution)
 * 3.开启本地视频采集。 bytertc::IRTCVideo::startVideoCapture()
 * 4.设置本地视频渲染时，使用的视图，并设置渲染模式。
 *   bytertc::IRTCVideo::setLocalVideoCanvas(
 *       StreamIndex index,
 *       const VideoCanvas& canvas)
 * 5.创建房间。IRTCRoom* bytertc::IRTCVideo::createRTCRoom(
 *               const char* room_id)
 * 6.加入音视频通话房间。bytertc::IRTCRoom::joinRoom(
 *                        const char* token,
 *                        const UserInfo& user_info,
 *                        const RTCRoomConfig& config)
 * 7.SDK 接收并解码远端视频流首帧后，设置用户的视频渲染视图。
 *   bytertc::IRTCVideo::setRemoteStreamVideoCanvas(
 *       RemoteStreamKey stream_key,
 *       const VideoCanvas& canvas)
 * 8.在用户离开房间之后移出用户的视频渲染视图。
 * 9.离开音视频通话房间。bytertc::IRTCRoom::leaveRoom()
 * 10.销毁房间。bytertc::IRTCRoom::destroy()
 * 11.销毁引擎。bytertc::destroyRTCVideo();
 *
 * 详细的API文档参见: https://www.volcengine.com/docs/6348/85515
 */

RoomMainWidget::RoomMainWidget(QWidget *parent)
        : QWidget(parent) {
    ui.setupUi(this);
    setWindowFlags(Qt::FramelessWindowHint | windowFlags());

    setupView();
    setupSignals();
    
    // 初始化 AI 管理器
    m_aiManager = new AIManager(this);
    connect(m_aiManager, &AIManager::configLoaded, this, &RoomMainWidget::slotOnAIConfigLoaded);
    connect(m_aiManager, &AIManager::configFailed, this, &RoomMainWidget::slotOnAIConfigFailed);
    connect(m_aiManager, &AIManager::aiStarted, this, &RoomMainWidget::slotOnAIStarted);
    connect(m_aiManager, &AIManager::aiFailed, this, &RoomMainWidget::slotOnAIFailed);
    connect(m_aiManager, &AIManager::aiStopped, this, &RoomMainWidget::slotOnAIStopped);
    
    // 自动获取场景配置
    qDebug() << "正在从 AIGC Server 获取配置...";
    m_aiManager->initialize("http://localhost:3001");
}

void RoomMainWidget::leaveRoom() {

}

void RoomMainWidget::setupView() {
    // 创建视频背景 - 优先使用 GPU 渲染
    if (m_useGPURendering) {
        m_videoBackgroundGL = new VideoRenderWidgetGL(ui.mainWidget);
        m_videoBackgroundGL->setObjectName("videoBackground");
        qDebug() << "Using GPU (OpenGL) video rendering";
    } else {
        m_videoBackground = new VideoRenderWidget(ui.mainWidget);
        m_videoBackground->setObjectName("videoBackground");
        qDebug() << "Using CPU video rendering";
    }
    
    // 创建对话字幕组件 (覆盖在视频上)
    m_conversationWidget = new ConversationWidget(ui.mainWidget);
    m_conversationWidget->setObjectName("conversationWidget");
    
    // 关闭按钮 (右上角)
    m_closeBtn = new QPushButton(ui.mainWidget);
    m_closeBtn->setObjectName("closeBtn");
    m_closeBtn->setFixedSize(40, 40);
    m_closeBtn->setStyleSheet(
        "QPushButton {"
        "  background: rgba(0, 0, 0, 0.5);"
        "  border: none;"
        "  border-radius: 20px;"
        "  color: white;"
        "  font-size: 20px;"
        "  font-weight: bold;"
        "}"
        "QPushButton:hover { background: rgba(255, 0, 0, 0.7); }"
    );
    m_closeBtn->setText("×");
    connect(m_closeBtn, &QPushButton::clicked, this, &RoomMainWidget::on_closeBtn_clicked);

    m_loginWidget = QSharedPointer<LoginWidget>::create(this);
    m_operateWidget = QSharedPointer<OperateWidget>::create(this);
    m_modeWidget = QSharedPointer<ModeWidget>::create(this);
    toggleShowFloatWidget(false);
    
    // 初始布局
    resizeEvent(nullptr);
}

void RoomMainWidget::on_closeBtn_clicked() {
    qDebug() << "receive close event";
    slotOnHangup();
    close();
}

void RoomMainWidget::mousePressEvent(QMouseEvent *event) {
    if (event->buttons() & Qt::LeftButton) {
        m_prevGlobalPoint = event->globalPos();
        m_bLeftBtnPressed = true;
    }
}

void RoomMainWidget::resizeEvent(QResizeEvent *event) {
    Q_UNUSED(event);
    
    if (!ui.mainWidget) return;
    
    QRect mainRect = ui.mainWidget->rect();
    
    // 视频背景填满整个区域
    if (m_useGPURendering && m_videoBackgroundGL) {
        m_videoBackgroundGL->setGeometry(mainRect);
    } else if (m_videoBackground) {
        m_videoBackground->setGeometry(mainRect);
    }
    
    // 对话字幕区域 - 适配4.3寸横屏 (800x480)
    if (m_conversationWidget) {
        int topMargin = 20;
        int bottomMargin = 60;  // 底部留给控制栏
        int sideMargin = 16;
        m_conversationWidget->setGeometry(
            sideMargin, 
            topMargin, 
            mainRect.width() - sideMargin * 2,
            mainRect.height() - topMargin - bottomMargin
        );
    }
    
    // 关闭按钮在右上角
    if (m_closeBtn) {
        m_closeBtn->move(mainRect.width() - 45, 5);
    }
}

void RoomMainWidget::mouseMoveEvent(QMouseEvent *event) {
    if (m_bLeftBtnPressed) {
        auto offset = event->globalPos() - m_prevGlobalPoint;
        m_prevGlobalPoint = event->globalPos();
        move(pos() + offset);
    }
}

void RoomMainWidget::mouseReleaseEvent(QMouseEvent *event) {
    m_bLeftBtnPressed = false;
}

void RoomMainWidget::slotOnEnterRoom(const QString &roomID, const QString &userID) {
    m_uid = userID.toStdString();
    m_roomId = roomID.toStdString();
    toggleShowFloatWidget(true);
    
    // 设置对话组件的用户名
    if (m_conversationWidget) {
        m_conversationWidget->setUserName("我");
        if (m_aiManager && m_aiManager->hasConfig()) {
            m_conversationWidget->setAIName(m_aiManager->getRtcConfig().sceneName);
        } else {
            m_conversationWidget->setAIName("AI");
        }
    }

    // 创建引擎 - 使用服务器配置的 AppId 或本地配置
    bytertc::EngineConfig config;
    std::string appId = Constants::APP_ID;
    if (m_useServerConfig && m_aiManager && m_aiManager->hasConfig()) {
        appId = m_aiManager->getRtcConfig().appId.toStdString();
        qDebug() << "Using Server AppId:" << appId.c_str();
    }
    config.app_id = appId.c_str();
    config.parameters = "";
    m_rtc_video = bytertc::IRTCEngine::createRTCEngine(config, this);
    if (m_rtc_video == nullptr) {
        qWarning() << "create engine failed";
        return;
    }

    // 配置音频设备 - 选择 USB 音频设备 (Yundea M1066)
    setupAudioDevices();

    bytertc::VideoEncoderConfig conf;
    conf.frame_rate = 15;
    conf.width = 360;
    conf.height = 640;
    // 设置视频发布参数
    m_rtc_video->setVideoEncoderConfig(conf);

    // 如果是空的stream_id，RTC会自动生成
    std::string stream_id = "";
    
    // 使用外部视频源 (libcamera) 替代内部采集
    // 树莓派5的摄像头使用libcamera系统，SDK无法直接访问
    m_rtc_video->setVideoSourceType(bytertc::kVideoSourceTypeExternal);
    qDebug() << "Set video source type to external";
    
    // 设置自定义视频渲染器用于本地预览 (使用视频背景)
    if (m_useGPURendering && m_videoBackgroundGL) {
        setupCustomVideoSink(true, stream_id, m_uid, m_videoBackgroundGL);
    } else {
        setupCustomVideoSink(true, stream_id, m_uid, m_videoBackground);
    }
    
    // 启动外部视频源捕获
    if (!m_externalVideoSource) {
        m_externalVideoSource = new ExternalVideoSource(this);
    }
    m_externalVideoSource->setRTCEngine(m_rtc_video);
    m_externalVideoSource->startCapture();
    qDebug() << "External video source started";
    
    // 尝试使用外部音频源 (SDK 在 Linux ARM 上无法枚举音频设备)
    int audioSourceRet = m_rtc_video->setAudioSourceType(bytertc::kAudioSourceTypeExternal);
    qDebug() << "setAudioSourceType(External) ret:" << audioSourceRet;
    
    if (audioSourceRet == 0) {
        // 外部音频源可用，启动外部音频采集
        if (!m_externalAudioSource) {
            m_externalAudioSource = new ExternalAudioSource(this);
        }
        m_externalAudioSource->setRTCEngine(m_rtc_video);
        m_externalAudioSource->startCapture();
        qDebug() << "External audio source started";
    } else {
        // 外部音频源不可用，尝试内部采集
        qDebug() << "External audio source not available, trying internal capture";
        m_rtc_video->startAudioCapture();
    }
    
    // 使用 IAudioFrameObserver 回调方式获取远端音频并播放
    if (!m_externalAudioRender) {
        m_externalAudioRender = new ExternalAudioRender(this);
    }
    m_externalAudioRender->setRTCEngine(m_rtc_video);
    m_externalAudioRender->startRender();
    qDebug() << "External audio render started (callback mode)";

    m_rtc_room = m_rtc_video->createRTCRoom(m_roomId.c_str());
    m_rtc_room->setRTCRoomEventHandler(this);
    bytertc::UserInfo userInfo;
    userInfo.uid = m_uid.c_str();
    userInfo.extra_info = nullptr;

    bytertc::RTCRoomConfig roomConfig;
    roomConfig.stream_id = stream_id.c_str();
    roomConfig.is_auto_publish_audio = true;
    roomConfig.is_auto_publish_video = true;
    roomConfig.is_auto_subscribe_audio = true;
    roomConfig.is_auto_subscribe_video = true;
    roomConfig.room_profile_type = bytertc::kRoomProfileTypeCommunication;
    // 加入房间 - 使用服务器配置的 Token 或本地生成
    std::string token;
    if (m_useServerConfig && m_aiManager && m_aiManager->hasConfig()) {
        token = m_aiManager->getRtcConfig().token.toStdString();
        qDebug() << "Using Server Token";
    } else {
        token = TokenGenerator::generate(Constants::APP_ID, Constants::APP_KEY, m_roomId, m_uid);
        qDebug() << "Using locally generated Token";
    }
    m_rtc_room->joinRoom(token.c_str(), userInfo, true, roomConfig);

    qDebug() << "joinroom, token:" << token.c_str() << ",uid:" << userInfo.uid << ",roomid:" << m_roomId.c_str();
}

void RoomMainWidget::toggleShowFloatWidget(bool isEnterRoom) {
    m_loginWidget->setVisible(!isEnterRoom);
    m_operateWidget->setVisible(isEnterRoom);
    m_modeWidget->setVisible(isEnterRoom);
    if (m_closeBtn) {
        m_closeBtn->setVisible(isEnterRoom);
    }
    if (m_conversationWidget) {
        m_conversationWidget->setVisible(isEnterRoom);
    }
}

void RoomMainWidget::slotOnHangup() {
    toggleShowFloatWidget(false);
    
    // 如果 AI 已启动，先停止 AI
    if (m_aiManager && m_aiManager->isAIStarted()) {
        m_aiManager->stopAI();
    }
    
    // 停止外部视频源
    if (m_externalVideoSource) {
        m_externalVideoSource->stopCapture();
    }
    
    // 停止外部音频源和渲染
    if (m_externalAudioSource) {
        m_externalAudioSource->stopCapture();
    }
    if (m_externalAudioRender) {
        m_externalAudioRender->stopRender();
    }
    
    if (m_rtc_room) {
        // 离开房间
        m_rtc_room->leaveRoom();
        m_rtc_room->destroy();
        m_rtc_room = nullptr;
    }
    // 销毁引擎
    bytertc::IRTCEngine::destroyRTCEngine();
    m_rtc_video = nullptr;
    if (m_operateWidget)
    {
        m_operateWidget->reset();
    }

    clearVideoView();
}

void RoomMainWidget::onRoomStateChanged(
            const char* room_id, const char* uid, int state, const char* extra_info){
    qDebug() << "onRoomStateChanged,roomid:" << room_id << ",uid:" << uid << ",state:" << state;

}

void RoomMainWidget::onError(int err) {
    qDebug() << "bytertc::OnError err" << err;
    emit sigError(err);
}

void RoomMainWidget::onUserJoined(const bytertc::UserInfo &user_info) {
    qDebug() << "bytertc::OnUserJoined " << user_info.uid;
}

void RoomMainWidget::onUserLeave(const char *uid, bytertc::UserOfflineReason reason) {
    qDebug() << "user leave id = " << uid;
    emit sigUserLeave(uid);
}

void RoomMainWidget::onRoomBinaryMessageReceived(const char* uid, int size, const uint8_t* message) {
    if (size < 8) {
        qDebug() << "Binary message too short:" << size;
        return;
    }
    
    // 解析 TLV 格式: | type (4 bytes) | length (4 bytes, big-endian) | value |
    // Type
    QString type;
    for (int i = 0; i < 4; i++) {
        type += QChar(message[i]);
    }
    
    // Length (big-endian)
    uint32_t length = (message[4] << 24) | (message[5] << 16) | (message[6] << 8) | message[7];
    
    if (size < 8 + (int)length) {
        qDebug() << "Binary message incomplete, expected:" << (8 + length) << "got:" << size;
        return;
    }
    
    // Value
    QByteArray valueData((const char*)(message + 8), length);
    QString value = QString::fromUtf8(valueData);
    
    qDebug() << "Received binary message type:" << type << "length:" << length;
    
    // 解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(valueData);
    if (doc.isNull()) {
        qDebug() << "Failed to parse JSON:" << value;
        return;
    }
    
    QJsonObject obj = doc.object();
    
    if (type == "subv") {
        // 字幕消息
        QJsonArray dataArray = obj["data"].toArray();
        if (!dataArray.isEmpty()) {
            QJsonObject data = dataArray[0].toObject();
            QString text = data["text"].toString();
            QString msgUserId = data["userId"].toString();
            bool definite = data["definite"].toBool();
            int paragraph = data["paragraph"].toInt();
            
            if (!text.isEmpty() && m_conversationWidget) {
                // 判断是用户还是 AI
                bool isUser = (msgUserId == QString::fromStdString(m_uid));
                
                qDebug() << "Subtitle:" << (isUser ? "User" : "AI") << text << "definite:" << definite;
                
                // 使用 QMetaObject::invokeMethod 确保在 UI 线程执行
                QMetaObject::invokeMethod(this, [this, text, msgUserId, isUser, definite]() {
                    m_conversationWidget->handleMessage(text, msgUserId, isUser, definite);
                }, Qt::QueuedConnection);
            }
        }
    } else if (type == "conv") {
        // 状态变化消息
        QJsonObject stage = obj["Stage"].toObject();
        int code = stage["Code"].toInt();
        QString description = stage["Description"].toString();
        qDebug() << "Agent state:" << code << description;
        
        // 状态码: 1=LISTENING, 2=THINKING, 3=SPEAKING, 4=INTERRUPTED, 5=FINISHED
        if (code == 4 && m_conversationWidget) {
            // 被打断
            QMetaObject::invokeMethod(this, [this]() {
                m_conversationWidget->setInterrupted();
            }, Qt::QueuedConnection);
        }
    }
}

void RoomMainWidget::onFirstLocalVideoFrameCaptured(bytertc::IVideoSource* video_source, const bytertc::VideoFrameInfo& info) {
    qDebug() << "first local video frame is captured";
    // 使用外部视频源时，此回调可能不会触发，视频 sink 已在 slotOnEnterRoom 中设置
}

void RoomMainWidget::onFirstRemoteVideoFrameDecoded(const char* stream_id, const bytertc::StreamInfo& stream_info, const bytertc::VideoFrameInfo& info) {
    qDebug() << "first remote video frame is decoded stream_id = " << stream_id << ", user_id = " << stream_info.user_id;
    emit sigUserEnter(stream_id, stream_info.user_id);
}

void RoomMainWidget::setRenderCanvas(bool isLocal, void *view, const std::string &stream_id, const std::string &user_id) {
    if (m_rtc_video == nullptr) {
        qDebug() << "byte engine is null ptr";
    }

    bytertc::VideoCanvas canvas;
    canvas.view = view;
    canvas.render_mode = bytertc::RenderMode::kRenderModeFit;

    if (isLocal) {
        // 设置本地视频渲染视图
        m_rtc_video->setLocalVideoCanvas(canvas);
    } else {
        // 设置远端用户视频渲染视图
        // bytertc::RemoteStreamKey remote_stream_key;
        // remote_stream_key.room_id = m_roomId.c_str();
        // remote_stream_key.user_id = user_id.c_str();
        // remote_stream_key.stream_index = bytertc::kStreamIndexMain;
        m_rtc_video->setRemoteVideoCanvas(stream_id.c_str(), canvas);
    }
}

void RoomMainWidget::setupCustomVideoSink(bool isLocal, const std::string &stream_id, const std::string &user_id, QWidget* renderWidget) {
    if (m_rtc_video == nullptr || renderWidget == nullptr) {
        qDebug() << "setupCustomVideoSink: invalid parameters";
        return;
    }

    // 创建自定义视频渲染器
    CustomVideoSink* videoSink = new CustomVideoSink(renderWidget);
    
    // 检查是否是 OpenGL 渲染 widget
    VideoRenderWidgetGL* glWidget = qobject_cast<VideoRenderWidgetGL*>(renderWidget);
    if (glWidget) {
        // GPU 模式
        videoSink->setGLRenderWidget(glWidget);
        videoSink->setUseGPU(true);
        glWidget->setVideoSink(videoSink);
        qDebug() << "VideoRenderWidgetGL (GPU) sink set successfully";
    } else {
        // CPU 模式
        VideoRenderWidget* videoRenderWidget = qobject_cast<VideoRenderWidget*>(renderWidget);
        qDebug() << "setupCustomVideoSink: renderWidget=" << renderWidget << "videoRenderWidget=" << videoRenderWidget;
        if (videoRenderWidget) {
            videoRenderWidget->setVideoSink(videoSink);
            qDebug() << "VideoRenderWidget (CPU) sink set successfully";
        } else {
            qDebug() << "WARNING: qobject_cast to VideoRenderWidget failed!";
        }
    }

    // 配置视频 sink
    if (isLocal) {
        // 本地视频
        bytertc::LocalVideoSinkConfig config;
        config.pixel_format = bytertc::kVideoPixelFormatI420;
        int ret = m_rtc_video->setLocalVideoSink(videoSink, config);
        qDebug() << "setLocalVideoSink returned:" << ret;
        
        // 保存引用
        if (m_localVideoSink) {
            m_localVideoSink->release();
            delete m_localVideoSink;
        }
        m_localVideoSink = videoSink;
        qDebug() << "Local video sink setup completed";
    } else {
        // 远端视频
        bytertc::RemoteVideoSinkConfig config;
        config.pixel_format = bytertc::kVideoPixelFormatI420;
        m_rtc_video->setRemoteVideoSink(stream_id.c_str(), videoSink, config);
        
        // 保存引用
        QString qUserId = QString::fromStdString(user_id);
        if (m_remoteVideoSinks.contains(qUserId)) {
            m_remoteVideoSinks[qUserId]->release();
            delete m_remoteVideoSinks[qUserId];
        }
        m_remoteVideoSinks[qUserId] = videoSink;
        qDebug() << "Remote video sink setup completed for user:" << qUserId;
    }
}

void RoomMainWidget::setupAudioDevices() {
    if (!m_rtc_video) {
        qDebug() << "setupAudioDevices: RTC engine is null";
        return;
    }
    
    bytertc::IAudioDeviceManager* audioManager = m_rtc_video->getAudioDeviceManager();
    if (!audioManager) {
        qDebug() << "setupAudioDevices: Failed to get audio device manager";
        return;
    }
    
    // 不跟随系统设置，手动选择设备
    audioManager->followSystemCaptureDevice(false);
    audioManager->followSystemPlaybackDevice(false);
    
    // 枚举并选择音频采集设备 (麦克风)
    bytertc::IAudioDeviceCollection* captureDevices = audioManager->enumerateAudioCaptureDevices();
    if (captureDevices) {
        int count = captureDevices->getCount();
        qDebug() << "Found" << count << "audio capture devices";
        
        char deviceName[bytertc::MAX_DEVICE_ID_LENGTH];
        char deviceId[bytertc::MAX_DEVICE_ID_LENGTH];
        
        for (int i = 0; i < count; i++) {
            if (captureDevices->getDevice(i, deviceName, deviceId) == 0) {
                qDebug() << "  Capture device" << i << ":" << deviceName << "ID:" << deviceId;
                // 选择包含 "USB" 或 "M1066" 的设备
                QString name = QString::fromUtf8(deviceName);
                if (name.contains("USB", Qt::CaseInsensitive) || name.contains("M1066", Qt::CaseInsensitive)) {
                    int ret = audioManager->setAudioCaptureDevice(deviceId);
                    qDebug() << "Selected capture device:" << deviceName << "ret:" << ret;
                    break;
                }
            }
        }
        captureDevices->release();
    }
    
    // 枚举并选择音频播放设备 (喇叭)
    bytertc::IAudioDeviceCollection* playbackDevices = audioManager->enumerateAudioPlaybackDevices();
    if (playbackDevices) {
        int count = playbackDevices->getCount();
        qDebug() << "Found" << count << "audio playback devices";
        
        char deviceName[bytertc::MAX_DEVICE_ID_LENGTH];
        char deviceId[bytertc::MAX_DEVICE_ID_LENGTH];
        
        for (int i = 0; i < count; i++) {
            if (playbackDevices->getDevice(i, deviceName, deviceId) == 0) {
                qDebug() << "  Playback device" << i << ":" << deviceName << "ID:" << deviceId;
                // 选择包含 "USB" 或 "M1066" 的设备
                QString name = QString::fromUtf8(deviceName);
                if (name.contains("USB", Qt::CaseInsensitive) || name.contains("M1066", Qt::CaseInsensitive)) {
                    int ret = audioManager->setAudioPlaybackDevice(deviceId);
                    qDebug() << "Selected playback device:" << deviceName << "ret:" << ret;
                    break;
                }
            }
        }
        playbackDevices->release();
    }
}

void RoomMainWidget::setupSignals() {
    connect(this, &RoomMainWidget::sigUserEnter, this, [=](const QString &streamID, const QString &userID) {
        qDebug() << "Remote user entered:" << userID;
        // 新 UI 只显示本地视频背景，远端用户通过 AI 语音交互
    });

    connect(this, &RoomMainWidget::sigUserLeave, this, [=](const QString &userID) {
        qDebug() << "Remote user left:" << userID;
    });

    connect(m_operateWidget.get(), &OperateWidget::sigMuteAudio, this, [this](bool bMute) {
        if (m_rtc_room) {
            if (bMute) {
                // 关闭本地音频发送
                m_rtc_room->publishStreamAudio(false);
            }
            else {
                // 开启本地音频发送
                m_rtc_room->publishStreamAudio(true);
            }
        }
    });

    connect(m_operateWidget.get(), &OperateWidget::sigMuteVideo, this, [this](bool bMute) {
        if (m_rtc_video) {
            if (bMute) {
                // 关闭视频采集
                m_rtc_video->stopVideoCapture();
            } else {
                // 开启视频采集
                m_rtc_video->startVideoCapture();
            }
            QTimer::singleShot(10, this, [=] {
                if (m_useGPURendering && m_videoBackgroundGL) {
                    m_videoBackgroundGL->update();
                } else if (m_videoBackground) {
                    m_videoBackground->update();
                }
            });
        }
    });

    connect(m_operateWidget.get(), &OperateWidget::sigMuteSpeaker, this, [this](bool bMute) {
        qDebug() << "sigMuteSpeaker received, mute:" << bMute;
        if (m_externalAudioRender) {
            m_externalAudioRender->setMute(bMute);
            qDebug() << "Speaker mute set to:" << bMute;
        }
    });

    connect(this, &RoomMainWidget::sigError, this, [this](int errorCode) {
        QString errorInfo = "error:";
        errorInfo += QString::number(errorCode);
        QMessageBox::warning(this, QStringLiteral(u"提示"),errorInfo ,QStringLiteral(u"确定"));
    });
    
    // 音量控制信号
    connect(m_operateWidget.get(), &OperateWidget::sigMicVolumeChanged, this, [this](int volume) {
        if (m_externalAudioSource) {
            m_externalAudioSource->setVolume(volume);
            qDebug() << "Mic volume set to:" << volume;
        }
    });
    
    connect(m_operateWidget.get(), &OperateWidget::sigSpeakerVolumeChanged, this, [this](int volume) {
        if (m_externalAudioRender) {
            m_externalAudioRender->setVolume(volume);
            qDebug() << "Speaker volume set to:" << volume;
        }
    });
    
    // 模式切换信号连接
    connect(m_modeWidget.get(), &ModeWidget::sigModeChanged, this, &RoomMainWidget::slotOnModeChanged);
    
    // 摄像头切换信号连接
    connect(m_operateWidget.get(), &OperateWidget::sigCameraChanged, this, &RoomMainWidget::slotOnCameraChanged);
}

void RoomMainWidget::clearVideoView() {
    // 清空对话记录
    if (m_conversationWidget) {
        m_conversationWidget->clearMessages();
    }
}

// ==================== AIManager 相关槽函数 ====================

void RoomMainWidget::slotOnModeChanged(AIMode mode) {
    qDebug() << "Mode changed to:" << static_cast<int>(mode);
    
    // 清空聊天记录
    if (m_conversationWidget) {
        m_conversationWidget->clearMessages();
        m_conversationWidget->setAIReady(false);
    }
    
    // 委托给 AIManager 处理模式切换
    if (m_aiManager) {
        m_aiManager->setMode(mode);
    }
}

void RoomMainWidget::slotOnAIConfigLoaded(const AIGCApi::RTCConfig& config) {
    qDebug() << "AIGC 配置获取成功:";
    qDebug() << "  场景名称:" << config.sceneName;
    qDebug() << "  AppId:" << config.appId;
    qDebug() << "  RoomId:" << config.roomId;
    qDebug() << "  UserId:" << config.userId;
    qDebug() << "  Token:" << config.token.left(20) << "...";
    qDebug() << "  是否支持视觉:" << config.isVision;
    
    m_useServerConfig = true;
    
    // 自动进入房间（跳过登录页）
    qDebug() << "自动进入房间...";
    slotOnEnterRoom(config.roomId, config.userId);
}

void RoomMainWidget::slotOnAIConfigFailed(const QString& error) {
    qDebug() << "AIGC 配置获取失败:" << error;
    m_useServerConfig = false;
    
    // 通知 LoginWidget 切换到手动模式
    if (m_loginWidget) {
        m_loginWidget->setConfigError(error);
    }
}

void RoomMainWidget::slotOnAIStarted() {
    qDebug() << "AI 启动成功！";
    
    // 更新 AI 准备状态
    if (m_conversationWidget) {
        m_conversationWidget->setAIReady(true);
    }
}

void RoomMainWidget::slotOnAIFailed(const QString& error) {
    qDebug() << "AI 启动失败:" << error;
    
    // 切换回待机模式
    if (m_modeWidget) {
        m_modeWidget->setMode(AIMode::Standby);
    }
    
    QMessageBox::warning(this, QStringLiteral(u"AI 启动失败"), 
        error, 
        QStringLiteral(u"确定"));
}

void RoomMainWidget::slotOnAIStopped() {
    qDebug() << "AI 已停止";
}

// ==================== 摄像头切换槽函数 ====================

void RoomMainWidget::slotOnCameraChanged(const CameraInfo& camera) {
    qDebug() << "RoomMainWidget: switching camera to" << camera.name << "type:" << camera.type;
    
    if (!m_externalVideoSource) {
        qDebug() << "ExternalVideoSource not initialized";
        return;
    }
    
    // 停止当前摄像头
    bool wasCapturing = m_externalVideoSource->isCapturing();
    if (wasCapturing) {
        m_externalVideoSource->stopCapture();
        // 等待线程完全停止
        QThread::msleep(100);
    }
    
    // 切换摄像头
    m_externalVideoSource->setCamera(camera);
    
    // 如果之前在采集，重新启动
    if (wasCapturing) {
        m_externalVideoSource->startCapture();
    }
    
    qDebug() << "Camera switched to:" << camera.name;
}
