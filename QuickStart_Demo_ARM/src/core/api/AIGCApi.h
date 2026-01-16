#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>
#include <QJsonObject>

/**
 * AIGC API 调用模块
 * 通过 AIGC Demo 的 Server 代理调用火山引擎 API
 * 
 * 参考 React 版本的 api.ts 实现
 */
class AIGCApi : public QObject {
    Q_OBJECT

public:
    struct RTCConfig {
        QString appId;
        QString roomId;
        QString userId;
        QString token;
        QString sceneName;
        QString botName;
        bool isVision = false;
        
        bool isValid() const {
            return !appId.isEmpty() && !roomId.isEmpty() && !userId.isEmpty() && !token.isEmpty();
        }
    };

    explicit AIGCApi(QObject* parent = nullptr);
    ~AIGCApi();

    // 设置服务器地址
    void setServerUrl(const QString& url);
    QString getServerUrl() const { return m_serverUrl; }

    // 设置场景ID
    void setSceneId(const QString& sceneId);
    QString getSceneId() const { return m_sceneId; }

    // 获取场景配置（包含 RTC 配置）
    void getScenes();

    // 启动 AI 对话
    void startVoiceChat();

    // 停止 AI 对话
    void stopVoiceChat();

    // 获取当前 RTC 配置
    RTCConfig getRtcConfig() const { return m_rtcConfig; }

    // 检查是否已获取配置
    bool hasConfig() const { return m_rtcConfig.isValid(); }

signals:
    // 获取场景配置成功
    void getScenesSuccess(const RTCConfig& config);
    // 获取场景配置失败
    void getScenesFailed(const QString& error);

    // 启动 AI 成功
    void startVoiceChatSuccess();
    // 启动 AI 失败
    void startVoiceChatFailed(const QString& error);

    // 停止 AI 成功
    void stopVoiceChatSuccess();
    // 停止 AI 失败
    void stopVoiceChatFailed(const QString& error);

private slots:
    void onGetScenesReply();
    void onStartVoiceChatReply();
    void onStopVoiceChatReply();

private:
    QNetworkAccessManager* m_networkManager;
    QString m_serverUrl;
    QString m_sceneId;
    RTCConfig m_rtcConfig;

    QNetworkReply* m_getScenesReply = nullptr;
    QNetworkReply* m_startReply = nullptr;
    QNetworkReply* m_stopReply = nullptr;

    void parseError(QNetworkReply* reply, QString& errorMsg);
};
