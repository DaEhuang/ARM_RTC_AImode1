#pragma once

#include <QtWidgets/QMainWindow>
#include <QSharedPointer>
#include "ui_RoomMainWidget.h"
#include "bytertc_engine.h"
#include "bytertc_room.h"
#include "bytertc_room_event_handler.h"
#include "AIGCApi.h"
#include "ExternalVideoSource.h"

class LoginWidget;
class OperateWidget;
class CustomVideoSink;
class ExternalVideoSource;
class ExternalAudioSource;
class ExternalAudioRender;
class ConversationWidget;
class VideoRenderWidget;
class VideoRenderWidgetGL;
class QPushButton;

class RoomMainWidget : public QWidget, public bytertc::IRTCRoomEventHandler, public bytertc::IRTCEngineEventHandler {
    Q_OBJECT

public:
    RoomMainWidget(QWidget *parent = Q_NULLPTR);

private
    slots:
            void on_closeBtn_clicked();
protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

protected:
    void onRoomStateChanged(
            const char* room_id, const char* uid, int state, const char* extra_info) override;
    void onError(int err) override;

    void onUserJoined(const bytertc::UserInfo &user_info) override;

    void onUserLeave(const char *uid, bytertc::UserOfflineReason reason) override;
    
    // 接收房间二进制消息 (AI 字幕)
    void onRoomBinaryMessageReceived(const char* uid, int size, const uint8_t* message) override;

    void onFirstLocalVideoFrameCaptured(bytertc::IVideoSource* video_source, const bytertc::VideoFrameInfo& info) override;

    void onFirstRemoteVideoFrameDecoded(const char* stream_id, const bytertc::StreamInfo& stream_info, const bytertc::VideoFrameInfo& info) override;

public
    slots:
            void slotOnEnterRoom(
    const QString &roomID,
    const QString &userID
    );

    void slotOnHangup();
    
    // AIGC 相关槽
    void slotOnStartAI();
    void slotOnStopAI();
    void slotOnGetScenesSuccess(const AIGCApi::RTCConfig& config);
    void slotOnGetScenesFailed(const QString& error);
    void slotOnStartVoiceChatSuccess();
    void slotOnStartVoiceChatFailed(const QString& error);
    void slotOnStopVoiceChatSuccess();
    void slotOnStopVoiceChatFailed(const QString& error);
    
    // 摄像头切换槽
    void slotOnCameraChanged(const CameraInfo& camera);

    signals:
            void sigJoinChannelSuccess(std::string
    channel,
    std::string uid,
    int elapsed
    );

    void sigJoinChannelFailed(std::string room_id, std::string uid, int error_code);

    void sigRoomJoinChannelSuccess(std::string channel, std::string uid, int elapsed);

    void sigUserEnter(const QString &stream_id, const QString &uid);

    void sigUserLeave(const QString &uid);

    void sigError(int errorCode);
private:
    void setupView();

    void setupSignals();

    void toggleShowFloatWidget(bool isEnterRoom);

    void leaveRoom();

    void setRenderCanvas(bool isLocal, void *view, const std::string &stream_id, const std::string &id);
    void setupCustomVideoSink(bool isLocal, const std::string &stream_id, const std::string &user_id, QWidget* renderWidget);
    void setupAudioDevices();
    void clearVideoView();

private:
    Ui::RoomMainForm ui;
    bool m_bLeftBtnPressed = false;
    QPoint m_prevGlobalPoint;
    QSharedPointer <LoginWidget> m_loginWidget;
    QSharedPointer <OperateWidget> m_operateWidget;
    bytertc::IRTCEngine* m_rtc_video = nullptr;
    bytertc::IRTCRoom* m_rtc_room = nullptr;
    std::string m_uid;
    std::string m_roomId;
    
    // 新 UI: 视频背景 + 对话字幕
    VideoRenderWidget* m_videoBackground = nullptr;        // CPU 渲染 (备用)
    VideoRenderWidgetGL* m_videoBackgroundGL = nullptr;    // GPU 渲染 (默认)
    ConversationWidget* m_conversationWidget = nullptr;
    QPushButton* m_closeBtn = nullptr;
    bool m_useGPURendering = true;  // 是否使用 GPU 渲染
    
    // 自定义视频渲染器 (Wayland兼容)
    CustomVideoSink* m_localVideoSink = nullptr;
    QMap<QString, CustomVideoSink*> m_remoteVideoSinks;
    
    // 外部视频源 (libcamera)
    ExternalVideoSource* m_externalVideoSource = nullptr;
    
    // 外部音频源和渲染 (ALSA)
    ExternalAudioSource* m_externalAudioSource = nullptr;
    ExternalAudioRender* m_externalAudioRender = nullptr;
    
    // AIGC API
    AIGCApi* m_aigcApi = nullptr;
    bool m_aiStarted = false;
    bool m_useServerConfig = false;  // 是否使用服务器配置
};
