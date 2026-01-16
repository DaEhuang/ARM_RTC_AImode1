#pragma once

#include <QObject>
#include <QString>
#include <QJsonObject>

/**
 * 配置管理器
 * 统一管理应用程序的所有配置项
 * 支持从 JSON 文件加载配置
 */
class ConfigManager : public QObject {
    Q_OBJECT

public:
    static ConfigManager* instance();
    
    // 加载/保存配置
    bool loadFromFile(const QString& path);
    bool saveToFile(const QString& path);
    bool isLoaded() const { return m_loaded; }
    
    // RTC 配置
    QString appId() const { return m_appId; }
    QString appKey() const { return m_appKey; }
    QString serverUrl() const { return m_serverUrl; }
    QString inputRegex() const { return m_inputRegex; }
    
    // 媒体配置
    QString preferredAudioDevice() const { return m_preferredAudioDevice; }
    int videoWidth() const { return m_videoWidth; }
    int videoHeight() const { return m_videoHeight; }
    int videoFrameRate() const { return m_videoFrameRate; }
    
    // UI 配置
    bool useGPURendering() const { return m_useGPURendering; }
    
    // 运行时修改
    void setAppId(const QString& appId);
    void setAppKey(const QString& appKey);
    void setServerUrl(const QString& url);
    
signals:
    void configChanged();
    void configLoaded();
    void configError(const QString& error);

private:
    explicit ConfigManager(QObject* parent = nullptr);
    ~ConfigManager() = default;
    
    // 禁止拷贝
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
    
    void loadDefaults();
    
    static ConfigManager* s_instance;
    bool m_loaded = false;
    QString m_configPath;
    
    // RTC 配置
    QString m_appId;
    QString m_appKey;
    QString m_serverUrl;
    QString m_inputRegex;
    
    // 媒体配置
    QString m_preferredAudioDevice;
    int m_videoWidth = 640;
    int m_videoHeight = 480;
    int m_videoFrameRate = 15;
    
    // UI 配置
    bool m_useGPURendering = true;
};
