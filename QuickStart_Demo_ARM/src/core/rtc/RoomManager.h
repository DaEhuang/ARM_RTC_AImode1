#pragma once

#include <QObject>
#include <QString>
#include "bytertc_engine.h"
#include "bytertc_room.h"

/**
 * RTC 房间管理器
 * 负责 RTC 引擎创建、房间加入/离开
 */
class RoomManager : public QObject {
    Q_OBJECT

public:
    explicit RoomManager(QObject* parent = nullptr);
    ~RoomManager();

    // 初始化引擎
    bool createEngine(const QString& appId, bytertc::IRTCEngineEventHandler* engineHandler);
    void destroyEngine();
    
    // 房间管理
    bool joinRoom(const QString& roomId, const QString& userId, const QString& token,
                  bytertc::IRTCRoomEventHandler* roomHandler);
    void leaveRoom();
    
    // 获取引擎和房间实例
    bytertc::IRTCEngine* getEngine() const { return m_engine; }
    bytertc::IRTCRoom* getRoom() const { return m_room; }
    
    // 状态查询
    bool isInRoom() const { return m_room != nullptr; }
    QString getRoomId() const { return m_roomId; }
    QString getUserId() const { return m_userId; }

signals:
    void engineCreated();
    void engineDestroyed();
    void roomJoined(const QString& roomId, const QString& userId);
    void roomLeft();
    void error(const QString& message);

private:
    bytertc::IRTCEngine* m_engine = nullptr;
    bytertc::IRTCRoom* m_room = nullptr;
    QString m_roomId;
    QString m_userId;
};
