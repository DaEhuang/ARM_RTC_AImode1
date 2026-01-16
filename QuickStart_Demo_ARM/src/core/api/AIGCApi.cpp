#include "AIGCApi.h"
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonArray>
#include <QDebug>
#include <QUrl>
#include <QUrlQuery>

AIGCApi::AIGCApi(QObject* parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serverUrl("http://localhost:3001")
    , m_sceneId("Custom")
{
}

AIGCApi::~AIGCApi()
{
    if (m_getScenesReply) {
        m_getScenesReply->abort();
        m_getScenesReply->deleteLater();
    }
    if (m_startReply) {
        m_startReply->abort();
        m_startReply->deleteLater();
    }
    if (m_stopReply) {
        m_stopReply->abort();
        m_stopReply->deleteLater();
    }
}

void AIGCApi::setServerUrl(const QString& url)
{
    m_serverUrl = url;
    qDebug() << "AIGC Server URL set to:" << url;
}

void AIGCApi::setSceneId(const QString& sceneId)
{
    m_sceneId = sceneId;
    qDebug() << "AIGC Scene ID set to:" << sceneId;
}

void AIGCApi::getScenes()
{
    qDebug() << "Fetching scenes from:" << m_serverUrl;

    QUrl url(m_serverUrl + "/getScenes");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    m_getScenesReply = m_networkManager->post(request, QByteArray("{}"));
    connect(m_getScenesReply, &QNetworkReply::finished, this, &AIGCApi::onGetScenesReply);
}

void AIGCApi::startVoiceChat()
{
    qDebug() << "Starting VoiceChat...";

    QUrl url(m_serverUrl + "/proxy");
    QUrlQuery query;
    query.addQueryItem("Action", "StartVoiceChat");
    query.addQueryItem("Version", "2024-12-01");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["SceneID"] = m_sceneId;
    // 传入当前客户端所在的 RoomId 和 UserId，确保 AI 加入同一房间
    body["RoomId"] = m_rtcConfig.roomId;
    body["UserId"] = m_rtcConfig.userId;

    m_startReply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(m_startReply, &QNetworkReply::finished, this, &AIGCApi::onStartVoiceChatReply);
}

void AIGCApi::stopVoiceChat()
{
    qDebug() << "Stopping VoiceChat...";

    QUrl url(m_serverUrl + "/proxy");
    QUrlQuery query;
    query.addQueryItem("Action", "StopVoiceChat");
    query.addQueryItem("Version", "2024-12-01");
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject body;
    body["SceneID"] = m_sceneId;

    m_stopReply = m_networkManager->post(request, QJsonDocument(body).toJson());
    connect(m_stopReply, &QNetworkReply::finished, this, &AIGCApi::onStopVoiceChatReply);
}

void AIGCApi::onGetScenesReply()
{
    if (!m_getScenesReply) return;

    QString errorMsg;
    if (m_getScenesReply->error() != QNetworkReply::NoError) {
        parseError(m_getScenesReply, errorMsg);
        qDebug() << "getScenes failed:" << errorMsg;
        emit getScenesFailed(errorMsg);
        m_getScenesReply->deleteLater();
        m_getScenesReply = nullptr;
        return;
    }

    QByteArray responseData = m_getScenesReply->readAll();
    qDebug() << "getScenes response:" << responseData;

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    // 检查是否有 Result 包装
    QJsonObject result = root.contains("Result") ? root["Result"].toObject() : root;

    QJsonArray scenes = result["scenes"].toArray();
    if (scenes.isEmpty()) {
        emit getScenesFailed("No scenes found");
        m_getScenesReply->deleteLater();
        m_getScenesReply = nullptr;
        return;
    }

    // 获取第一个场景
    QJsonObject scene = scenes[0].toObject();
    QJsonObject rtc = scene["rtc"].toObject();
    QJsonObject sceneConfig = scene["scene"].toObject();

    m_rtcConfig.appId = rtc["AppId"].toString();
    m_rtcConfig.roomId = rtc["RoomId"].toString();
    m_rtcConfig.userId = rtc["UserId"].toString();
    m_rtcConfig.token = rtc["Token"].toString();
    m_rtcConfig.sceneName = sceneConfig["name"].toString();
    m_rtcConfig.botName = sceneConfig["botName"].toString();
    m_rtcConfig.isVision = sceneConfig["isVision"].toBool();

    qDebug() << "RTC Config loaded:";
    qDebug() << "  AppId:" << m_rtcConfig.appId;
    qDebug() << "  RoomId:" << m_rtcConfig.roomId;
    qDebug() << "  UserId:" << m_rtcConfig.userId;
    qDebug() << "  SceneName:" << m_rtcConfig.sceneName;
    qDebug() << "  BotName:" << m_rtcConfig.botName;
    qDebug() << "  IsVision:" << m_rtcConfig.isVision;

    emit getScenesSuccess(m_rtcConfig);

    m_getScenesReply->deleteLater();
    m_getScenesReply = nullptr;
}

void AIGCApi::onStartVoiceChatReply()
{
    if (!m_startReply) return;

    QString errorMsg;
    if (m_startReply->error() != QNetworkReply::NoError) {
        parseError(m_startReply, errorMsg);
        qDebug() << "startVoiceChat network error:" << errorMsg;
        emit startVoiceChatFailed(errorMsg);
        m_startReply->deleteLater();
        m_startReply = nullptr;
        return;
    }

    QByteArray responseData = m_startReply->readAll();
    qDebug() << "startVoiceChat response:" << responseData;

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    // 检查 API 错误
    if (root.contains("ResponseMetadata")) {
        QJsonObject metadata = root["ResponseMetadata"].toObject();
        if (metadata.contains("Error")) {
            QJsonObject error = metadata["Error"].toObject();
            errorMsg = error["Message"].toString();
            if (errorMsg.isEmpty()) {
                errorMsg = error["Code"].toString();
            }
            qDebug() << "startVoiceChat API error:" << errorMsg;
            emit startVoiceChatFailed(errorMsg);
            m_startReply->deleteLater();
            m_startReply = nullptr;
            return;
        }
    }

    qDebug() << "VoiceChat started successfully";
    emit startVoiceChatSuccess();

    m_startReply->deleteLater();
    m_startReply = nullptr;
}

void AIGCApi::onStopVoiceChatReply()
{
    if (!m_stopReply) return;

    QString errorMsg;
    if (m_stopReply->error() != QNetworkReply::NoError) {
        parseError(m_stopReply, errorMsg);
        qDebug() << "stopVoiceChat network error:" << errorMsg;
        emit stopVoiceChatFailed(errorMsg);
        m_stopReply->deleteLater();
        m_stopReply = nullptr;
        return;
    }

    QByteArray responseData = m_stopReply->readAll();
    qDebug() << "stopVoiceChat response:" << responseData;

    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    QJsonObject root = doc.object();

    // 检查 API 错误
    if (root.contains("ResponseMetadata")) {
        QJsonObject metadata = root["ResponseMetadata"].toObject();
        if (metadata.contains("Error")) {
            QJsonObject error = metadata["Error"].toObject();
            errorMsg = error["Message"].toString();
            if (errorMsg.isEmpty()) {
                errorMsg = error["Code"].toString();
            }
            qDebug() << "stopVoiceChat API error:" << errorMsg;
            emit stopVoiceChatFailed(errorMsg);
            m_stopReply->deleteLater();
            m_stopReply = nullptr;
            return;
        }
    }

    qDebug() << "VoiceChat stopped successfully";
    emit stopVoiceChatSuccess();

    m_stopReply->deleteLater();
    m_stopReply = nullptr;
}

void AIGCApi::parseError(QNetworkReply* reply, QString& errorMsg)
{
    if (!reply) {
        errorMsg = "Reply is null";
        return;
    }

    switch (reply->error()) {
    case QNetworkReply::ConnectionRefusedError:
        errorMsg = "无法连接到 AIGC Server，请确保 Server 已启动 (http://localhost:3001)";
        break;
    case QNetworkReply::HostNotFoundError:
        errorMsg = "服务器地址无效";
        break;
    case QNetworkReply::TimeoutError:
        errorMsg = "请求超时";
        break;
    default:
        errorMsg = reply->errorString();
        break;
    }
}
