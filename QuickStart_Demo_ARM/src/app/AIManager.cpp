#include "AIManager.h"
#include <QDebug>
#include <QTimer>

AIManager::AIManager(QObject* parent)
    : QObject(parent)
{
}

AIManager::~AIManager()
{
    if (m_aiStarted) {
        stopAI();
    }
}

void AIManager::initialize(const QString& serverUrl)
{
    m_aigcApi = new AIGCApi(this);
    m_aigcApi->setServerUrl(serverUrl);
    
    connect(m_aigcApi, &AIGCApi::getScenesSuccess, this, &AIManager::onGetScenesSuccess);
    connect(m_aigcApi, &AIGCApi::getScenesFailed, this, &AIManager::onGetScenesFailed);
    connect(m_aigcApi, &AIGCApi::startVoiceChatSuccess, this, &AIManager::onStartVoiceChatSuccess);
    connect(m_aigcApi, &AIGCApi::startVoiceChatFailed, this, &AIManager::onStartVoiceChatFailed);
    connect(m_aigcApi, &AIGCApi::stopVoiceChatSuccess, this, &AIManager::onStopVoiceChatSuccess);
    connect(m_aigcApi, &AIGCApi::stopVoiceChatFailed, this, &AIManager::onStopVoiceChatFailed);
    
    m_aigcApi->getScenes();
}

void AIManager::setRoomInfo(const QString& roomId, const QString& userId)
{
    m_roomId = roomId;
    m_userId = userId;
}

void AIManager::setMode(AIMode mode)
{
    if (m_currentMode == mode) {
        return;
    }
    
    AIMode oldMode = m_currentMode;
    m_currentMode = mode;
    
    qDebug() << "AIManager: Mode changed from" << static_cast<int>(oldMode) 
             << "to" << static_cast<int>(mode);
    
    // 如果当前 AI 已启动，先停止
    if (m_aiStarted) {
        stopAI();
    }
    
    // 如果不是待机模式，启动对应的 AI
    if (mode != AIMode::Standby) {
        QString sceneId = getSceneIdForMode(mode);
        if (!sceneId.isEmpty() && m_aigcApi) {
            m_aigcApi->setSceneId(sceneId);
            
            // 短暂延迟后启动 AI
            QTimer::singleShot(800, this, [this]() {
                startAI();
            });
        }
    }
    
    emit modeChanged(mode);
}

void AIManager::startAI()
{
    if (!m_aigcApi || !m_aigcApi->hasConfig()) {
        emit aiFailed(QStringLiteral("AIGC 配置未加载"));
        return;
    }
    
    qDebug() << "AIManager: Starting AI...";
    
    // 先尝试停止可能存在的旧任务
    m_aigcApi->stopVoiceChat();
    
    // 短暂延迟后启动新任务
    QTimer::singleShot(500, this, [this]() {
        m_aigcApi->startVoiceChat();
    });
}

void AIManager::stopAI()
{
    if (!m_aigcApi) {
        return;
    }
    
    qDebug() << "AIManager: Stopping AI...";
    m_aigcApi->stopVoiceChat();
}

bool AIManager::hasConfig() const
{
    return m_aigcApi && m_aigcApi->hasConfig();
}

AIGCApi::RTCConfig AIManager::getRtcConfig() const
{
    if (m_aigcApi) {
        return m_aigcApi->getRtcConfig();
    }
    return AIGCApi::RTCConfig();
}

QString AIManager::getSceneIdForMode(AIMode mode) const
{
    switch (mode) {
        case AIMode::Chat:
            return "Chat";
        case AIMode::Supervise:
            return "Custom";  // 暂时使用 Custom
        case AIMode::Teach:
            return "Custom";
        case AIMode::Standby:
        default:
            return "";
    }
}

void AIManager::onGetScenesSuccess(const AIGCApi::RTCConfig& config)
{
    qDebug() << "AIManager: Config loaded successfully";
    emit configLoaded(config);
}

void AIManager::onGetScenesFailed(const QString& error)
{
    qDebug() << "AIManager: Config failed:" << error;
    emit configFailed(error);
}

void AIManager::onStartVoiceChatSuccess()
{
    qDebug() << "AIManager: AI started successfully";
    m_aiStarted = true;
    emit aiStarted();
}

void AIManager::onStartVoiceChatFailed(const QString& error)
{
    qDebug() << "AIManager: AI start failed:" << error;
    m_aiStarted = false;
    emit aiFailed(error);
}

void AIManager::onStopVoiceChatSuccess()
{
    qDebug() << "AIManager: AI stopped successfully";
    m_aiStarted = false;
    emit aiStopped();
}

void AIManager::onStopVoiceChatFailed(const QString& error)
{
    qDebug() << "AIManager: AI stop failed:" << error;
}
