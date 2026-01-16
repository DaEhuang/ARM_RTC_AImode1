#pragma once

#include <QObject>
#include <QString>
#include "AIGCApi.h"
#include "common/AIMode.h"

/**
 * AI 管理器
 * 负责 AI 的启动、停止、模式切换
 */
class AIManager : public QObject {
    Q_OBJECT

public:
    explicit AIManager(QObject* parent = nullptr);
    ~AIManager();

    // 初始化
    void initialize(const QString& serverUrl);
    
    // 设置当前房间信息（用于 AI 加入同一房间）
    void setRoomInfo(const QString& roomId, const QString& userId);
    
    // 模式切换
    void setMode(AIMode mode);
    AIMode currentMode() const { return m_currentMode; }
    
    // AI 控制
    void startAI();
    void stopAI();
    bool isAIStarted() const { return m_aiStarted; }
    
    // 获取配置
    bool hasConfig() const;
    AIGCApi::RTCConfig getRtcConfig() const;

signals:
    // 配置获取
    void configLoaded(const AIGCApi::RTCConfig& config);
    void configFailed(const QString& error);
    
    // AI 状态
    void aiStarted();
    void aiFailed(const QString& error);
    void aiStopped();
    
    // 模式变化
    void modeChanged(AIMode mode);

private slots:
    void onGetScenesSuccess(const AIGCApi::RTCConfig& config);
    void onGetScenesFailed(const QString& error);
    void onStartVoiceChatSuccess();
    void onStartVoiceChatFailed(const QString& error);
    void onStopVoiceChatSuccess();
    void onStopVoiceChatFailed(const QString& error);

private:
    QString getSceneIdForMode(AIMode mode) const;
    
    AIGCApi* m_aigcApi = nullptr;
    AIMode m_currentMode = AIMode::Standby;
    bool m_aiStarted = false;
    QString m_roomId;
    QString m_userId;
};
