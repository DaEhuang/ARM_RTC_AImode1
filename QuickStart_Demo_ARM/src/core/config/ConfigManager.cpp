#include "ConfigManager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>
#include <QDir>

ConfigManager* ConfigManager::s_instance = nullptr;

ConfigManager* ConfigManager::instance()
{
    if (!s_instance) {
        s_instance = new ConfigManager();
    }
    return s_instance;
}

ConfigManager::ConfigManager(QObject* parent)
    : QObject(parent)
{
    loadDefaults();
}

void ConfigManager::loadDefaults()
{
    // 默认 RTC 配置
    m_appId = "68a569ad8f31e0018b7fbd8d";
    m_appKey = "011cde79231e42ae9e89fefd777c2604";
    m_serverUrl = "http://localhost:3001";
    m_inputRegex = "^[a-zA-Z0-9@._-]{1,128}$";
    
    // 默认媒体配置
    m_preferredAudioDevice = "USB";
    m_videoWidth = 640;
    m_videoHeight = 480;
    m_videoFrameRate = 15;
    
    // 默认 UI 配置
    m_useGPURendering = true;
}

bool ConfigManager::loadFromFile(const QString& path)
{
    QFile file(path);
    if (!file.exists()) {
        qDebug() << "ConfigManager: Config file not found:" << path;
        qDebug() << "ConfigManager: Using default configuration";
        m_loaded = true;
        emit configLoaded();
        return true;  // 使用默认配置
    }
    
    if (!file.open(QIODevice::ReadOnly)) {
        QString error = QString("Failed to open config file: %1").arg(path);
        qWarning() << "ConfigManager:" << error;
        emit configError(error);
        return false;
    }
    
    QByteArray data = file.readAll();
    file.close();
    
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        QString error = QString("JSON parse error: %1").arg(parseError.errorString());
        qWarning() << "ConfigManager:" << error;
        emit configError(error);
        return false;
    }
    
    QJsonObject root = doc.object();
    
    // 解析 RTC 配置
    if (root.contains("rtc")) {
        QJsonObject rtc = root["rtc"].toObject();
        if (rtc.contains("appId")) {
            m_appId = rtc["appId"].toString();
        }
        if (rtc.contains("appKey")) {
            m_appKey = rtc["appKey"].toString();
        }
        if (rtc.contains("serverUrl")) {
            m_serverUrl = rtc["serverUrl"].toString();
        }
        if (rtc.contains("inputRegex")) {
            m_inputRegex = rtc["inputRegex"].toString();
        }
    }
    
    // 解析媒体配置
    if (root.contains("media")) {
        QJsonObject media = root["media"].toObject();
        if (media.contains("preferredAudioDevice")) {
            m_preferredAudioDevice = media["preferredAudioDevice"].toString();
        }
        if (media.contains("video")) {
            QJsonObject video = media["video"].toObject();
            if (video.contains("width")) {
                m_videoWidth = video["width"].toInt();
            }
            if (video.contains("height")) {
                m_videoHeight = video["height"].toInt();
            }
            if (video.contains("frameRate")) {
                m_videoFrameRate = video["frameRate"].toInt();
            }
        }
    }
    
    // 解析 UI 配置
    if (root.contains("ui")) {
        QJsonObject ui = root["ui"].toObject();
        if (ui.contains("useGPURendering")) {
            m_useGPURendering = ui["useGPURendering"].toBool();
        }
    }
    
    m_configPath = path;
    m_loaded = true;
    
    qDebug() << "ConfigManager: Configuration loaded from" << path;
    qDebug() << "  AppId:" << m_appId;
    qDebug() << "  ServerUrl:" << m_serverUrl;
    qDebug() << "  Video:" << m_videoWidth << "x" << m_videoHeight << "@" << m_videoFrameRate << "fps";
    qDebug() << "  GPU Rendering:" << m_useGPURendering;
    
    emit configLoaded();
    return true;
}

bool ConfigManager::saveToFile(const QString& path)
{
    QJsonObject root;
    
    // RTC 配置
    QJsonObject rtc;
    rtc["appId"] = m_appId;
    rtc["appKey"] = m_appKey;
    rtc["serverUrl"] = m_serverUrl;
    rtc["inputRegex"] = m_inputRegex;
    root["rtc"] = rtc;
    
    // 媒体配置
    QJsonObject media;
    media["preferredAudioDevice"] = m_preferredAudioDevice;
    QJsonObject video;
    video["width"] = m_videoWidth;
    video["height"] = m_videoHeight;
    video["frameRate"] = m_videoFrameRate;
    media["video"] = video;
    root["media"] = media;
    
    // UI 配置
    QJsonObject ui;
    ui["useGPURendering"] = m_useGPURendering;
    root["ui"] = ui;
    
    QJsonDocument doc(root);
    
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QString error = QString("Failed to write config file: %1").arg(path);
        qWarning() << "ConfigManager:" << error;
        emit configError(error);
        return false;
    }
    
    file.write(doc.toJson(QJsonDocument::Indented));
    file.close();
    
    m_configPath = path;
    qDebug() << "ConfigManager: Configuration saved to" << path;
    return true;
}

void ConfigManager::setAppId(const QString& appId)
{
    if (m_appId != appId) {
        m_appId = appId;
        emit configChanged();
    }
}

void ConfigManager::setAppKey(const QString& appKey)
{
    if (m_appKey != appKey) {
        m_appKey = appKey;
        emit configChanged();
    }
}

void ConfigManager::setServerUrl(const QString& url)
{
    if (m_serverUrl != url) {
        m_serverUrl = url;
        emit configChanged();
    }
}
