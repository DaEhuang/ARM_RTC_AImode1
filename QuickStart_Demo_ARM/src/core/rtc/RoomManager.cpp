#include "RoomManager.h"
#include <QDebug>

RoomManager::RoomManager(QObject* parent)
    : QObject(parent)
{
}

RoomManager::~RoomManager()
{
    if (m_room) {
        leaveRoom();
    }
    if (m_engine) {
        destroyEngine();
    }
}

bool RoomManager::createEngine(const QString& appId, bytertc::IRTCEngineEventHandler* engineHandler)
{
    if (m_engine) {
        qWarning() << "RoomManager: Engine already exists";
        return false;
    }
    
    bytertc::EngineConfig config;
    std::string appIdStr = appId.toStdString();
    config.app_id = appIdStr.c_str();
    config.parameters = "";
    
    m_engine = bytertc::IRTCEngine::createRTCEngine(config, engineHandler);
    if (!m_engine) {
        qWarning() << "RoomManager: Failed to create engine";
        emit error("Failed to create RTC engine");
        return false;
    }
    
    qDebug() << "RoomManager: Engine created with AppId:" << appId;
    emit engineCreated();
    return true;
}

void RoomManager::destroyEngine()
{
    if (m_room) {
        leaveRoom();
    }
    
    if (m_engine) {
        bytertc::IRTCEngine::destroyRTCEngine();
        m_engine = nullptr;
        qDebug() << "RoomManager: Engine destroyed";
        emit engineDestroyed();
    }
}

bool RoomManager::joinRoom(const QString& roomId, const QString& userId, const QString& token,
                           bytertc::IRTCRoomEventHandler* roomHandler)
{
    if (!m_engine) {
        qWarning() << "RoomManager: Engine not created";
        emit error("Engine not created");
        return false;
    }
    
    if (m_room) {
        qWarning() << "RoomManager: Already in a room";
        return false;
    }
    
    m_roomId = roomId;
    m_userId = userId;
    
    std::string roomIdStr = roomId.toStdString();
    std::string userIdStr = userId.toStdString();
    std::string tokenStr = token.toStdString();
    
    m_room = m_engine->createRTCRoom(roomIdStr.c_str());
    if (!m_room) {
        qWarning() << "RoomManager: Failed to create room";
        emit error("Failed to create RTC room");
        return false;
    }
    
    m_room->setRTCRoomEventHandler(roomHandler);
    
    bytertc::UserInfo userInfo;
    userInfo.uid = userIdStr.c_str();
    userInfo.extra_info = nullptr;
    
    bytertc::RTCRoomConfig roomConfig;
    roomConfig.stream_id = "";
    roomConfig.is_auto_publish_audio = true;
    roomConfig.is_auto_publish_video = true;
    roomConfig.is_auto_subscribe_audio = true;
    roomConfig.is_auto_subscribe_video = true;
    roomConfig.room_profile_type = bytertc::kRoomProfileTypeCommunication;
    
    m_room->joinRoom(tokenStr.c_str(), userInfo, true, roomConfig);
    
    qDebug() << "RoomManager: Joining room" << roomId << "as user" << userId;
    emit roomJoined(roomId, userId);
    return true;
}

void RoomManager::leaveRoom()
{
    if (m_room) {
        m_room->leaveRoom();
        m_room->destroy();
        m_room = nullptr;
        
        qDebug() << "RoomManager: Left room" << m_roomId;
        emit roomLeft();
        
        m_roomId.clear();
        m_userId.clear();
    }
}
