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
#include "RoomManager.h"
#include "MediaManager.h"
#include "ConfigManager.h"
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
    m_aiManager->initialize(ConfigManager::instance()->serverUrl());
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
    
    // 待机动画 (覆盖在视频上)
    m_standbyLabel = new QLabel(ui.mainWidget);
    m_standbyLabel->setObjectName("standbyLabel");
    m_standbyLabel->setAlignment(Qt::AlignCenter);
    m_standbyLabel->setStyleSheet("background: transparent;");
    m_standbyMovie = new QMovie(":/QuickStart/images/normal.gif");
    m_standbyLabel->setMovie(m_standbyMovie);
    m_standbyLabel->setScaledContents(true);
    m_standbyMovie->start();
    m_standbyLabel->show();  // 默认显示待机动画
    
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
    
    // 待机动画填满整个区域
    if (m_standbyLabel) {
        m_standbyLabel->setGeometry(mainRect);
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

    // 创建 RoomManager
    if (!m_roomManager) {
        m_roomManager = new RoomManager(this);
    }
    
    // 创建引擎 - 使用服务器配置的 AppId 或本地配置
    QString appId = ConfigManager::instance()->appId();
    if (m_useServerConfig && m_aiManager && m_aiManager->hasConfig()) {
        appId = m_aiManager->getRtcConfig().appId;
        qDebug() << "Using Server AppId:" << appId;
    }
    
    if (!m_roomManager->createEngine(appId, this)) {
        qWarning() << "create engine failed";
        return;
    }
    
    bytertc::IRTCEngine* engine = m_roomManager->getEngine();

    // 初始化媒体管理器
    if (!m_mediaManager) {
        m_mediaManager = new MediaManager(this);
    }
    m_mediaManager->initialize(engine);
    
    // 如果是空的stream_id，RTC会自动生成
    std::string stream_id = "";
    std::string uidStr = userID.toStdString();
    
    // 设置自定义视频渲染器用于本地预览 (使用视频背景)
    if (m_useGPURendering && m_videoBackgroundGL) {
        setupCustomVideoSink(true, stream_id, uidStr, m_videoBackgroundGL);
    } else {
        setupCustomVideoSink(true, stream_id, uidStr, m_videoBackground);
    }
    
    // 启动所有媒体
    m_mediaManager->startAll();

    // 加入房间 - 使用服务器配置的 Token 或本地生成
    QString token;
    if (m_useServerConfig && m_aiManager && m_aiManager->hasConfig()) {
        token = m_aiManager->getRtcConfig().token;
        qDebug() << "Using Server Token";
    } else {
        std::string roomIdStr = roomID.toStdString();
        token = QString::fromStdString(TokenGenerator::generate(
            ConfigManager::instance()->appId().toStdString(),
            ConfigManager::instance()->appKey().toStdString(),
            roomIdStr, uidStr));
        qDebug() << "Using locally generated Token";
    }
    
    m_roomManager->joinRoom(roomID, userID, token, this);
    qDebug() << "joinroom, uid:" << userID << ",roomid:" << roomID;
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
    
    // 停止所有媒体
    if (m_mediaManager) {
        m_mediaManager->stopAll();
    }
    
    // 使用 RoomManager 离开房间和销毁引擎
    if (m_roomManager) {
        m_roomManager->destroyEngine();
    }
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
                bool isUser = (msgUserId == (m_roomManager ? m_roomManager->getUserId() : QString()));
                
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
    bytertc::IRTCEngine* engine = m_roomManager ? m_roomManager->getEngine() : nullptr;
    if (engine == nullptr) {
        qDebug() << "byte engine is null ptr";
    }

    bytertc::VideoCanvas canvas;
    canvas.view = view;
    canvas.render_mode = bytertc::RenderMode::kRenderModeFit;

    if (isLocal) {
        // 设置本地视频渲染视图
        engine->setLocalVideoCanvas(canvas);
    } else {
        // 设置远端用户视频渲染视图
        // bytertc::RemoteStreamKey remote_stream_key;
        // remote_stream_key.room_id = m_roomId.c_str();
        // remote_stream_key.user_id = user_id.c_str();
        // remote_stream_key.stream_index = bytertc::kStreamIndexMain;
        engine->setRemoteVideoCanvas(stream_id.c_str(), canvas);
    }
}

void RoomMainWidget::setupCustomVideoSink(bool isLocal, const std::string &stream_id, const std::string &user_id, QWidget* renderWidget) {
    bytertc::IRTCEngine* engine = m_roomManager ? m_roomManager->getEngine() : nullptr;
    if (engine == nullptr || renderWidget == nullptr) {
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
        int ret = engine->setLocalVideoSink(videoSink, config);
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
        engine->setRemoteVideoSink(stream_id.c_str(), videoSink, config);
        
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

void RoomMainWidget::setupSignals() {
    connect(this, &RoomMainWidget::sigUserEnter, this, [=](const QString &streamID, const QString &userID) {
        qDebug() << "Remote user entered:" << userID;
        // 新 UI 只显示本地视频背景，远端用户通过 AI 语音交互
    });

    connect(this, &RoomMainWidget::sigUserLeave, this, [=](const QString &userID) {
        qDebug() << "Remote user left:" << userID;
    });

    connect(m_operateWidget.get(), &OperateWidget::sigMuteAudio, this, [this](bool bMute) {
        bytertc::IRTCRoom* room = m_roomManager ? m_roomManager->getRoom() : nullptr;
        if (room) {
            if (bMute) {
                // 关闭本地音频发送
                room->publishStreamAudio(false);
            }
            else {
                // 开启本地音频发送
                room->publishStreamAudio(true);
            }
        }
    });

    connect(m_operateWidget.get(), &OperateWidget::sigMuteVideo, this, [this](bool bMute) {
        bytertc::IRTCEngine* engine = m_roomManager ? m_roomManager->getEngine() : nullptr;
        if (engine) {
            if (bMute) {
                // 关闭视频采集
                engine->stopVideoCapture();
            } else {
                // 开启视频采集
                engine->startVideoCapture();
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
        if (m_mediaManager) {
            m_mediaManager->setSpeakerMute(bMute);
        }
    });

    connect(this, &RoomMainWidget::sigError, this, [this](int errorCode) {
        QString errorInfo = "error:";
        errorInfo += QString::number(errorCode);
        QMessageBox::warning(this, QStringLiteral(u"提示"),errorInfo ,QStringLiteral(u"确定"));
    });
    
    // 音量控制信号
    connect(m_operateWidget.get(), &OperateWidget::sigMicVolumeChanged, this, [this](int volume) {
        if (m_mediaManager) {
            m_mediaManager->setMicVolume(volume);
        }
    });
    
    connect(m_operateWidget.get(), &OperateWidget::sigSpeakerVolumeChanged, this, [this](int volume) {
        if (m_mediaManager) {
            m_mediaManager->setSpeakerVolume(volume);
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
    
    // 待机模式显示动画，其他模式显示摄像头
    showStandbyAnimation(mode == AIMode::Standby);
    
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

void RoomMainWidget::slotOnAIConfigFailed(const AppError& error) {
    qDebug() << "AIGC 配置获取失败:" << error.toString();
    m_useServerConfig = false;
    
    // 通知 LoginWidget 切换到手动模式
    if (m_loginWidget) {
        m_loginWidget->setConfigError(error.message());
    }
}

void RoomMainWidget::slotOnAIStarted() {
    qDebug() << "AI 启动成功！";
    
    // 更新 AI 准备状态
    if (m_conversationWidget) {
        m_conversationWidget->setAIReady(true);
    }
}

void RoomMainWidget::slotOnAIFailed(const AppError& error) {
    qDebug() << "AI 启动失败:" << error.toString();
    
    // 切换回待机模式
    if (m_modeWidget) {
        m_modeWidget->setMode(AIMode::Standby);
    }
    
    QMessageBox::warning(this, QStringLiteral(u"AI 启动失败"), 
        error.message(), 
        QStringLiteral(u"确定"));
}

void RoomMainWidget::slotOnAIStopped() {
    qDebug() << "AI 已停止";
}

// ==================== 摄像头切换槽函数 ====================

void RoomMainWidget::slotOnCameraChanged(const CameraInfo& camera) {
    qDebug() << "RoomMainWidget: switching camera to" << camera.name << "type:" << camera.type;
    
    if (!m_mediaManager) {
        qDebug() << "MediaManager not initialized";
        return;
    }
    
    m_mediaManager->setCamera(camera);
    qDebug() << "Camera switched to:" << camera.name;
}

// ==================== 待机动画控制 ====================

void RoomMainWidget::showStandbyAnimation(bool show) {
    if (m_standbyLabel) {
        if (show) {
            m_standbyLabel->show();
            m_standbyLabel->raise();  // 置于顶层
            if (m_standbyMovie) {
                m_standbyMovie->start();
            }
        } else {
            m_standbyLabel->hide();
            if (m_standbyMovie) {
                m_standbyMovie->stop();
            }
        }
    }
    
    // 显示/隐藏视频背景
    if (m_useGPURendering && m_videoBackgroundGL) {
        m_videoBackgroundGL->setVisible(!show);
    } else if (m_videoBackground) {
        m_videoBackground->setVisible(!show);
    }
    
    qDebug() << "Standby animation:" << (show ? "shown" : "hidden");
}
