#pragma once

#include <QtWidgets/QMainWindow>
#include "ui_LoginWidget.h"

class LoginWidget : public QWidget {
    Q_OBJECT

public:
    LoginWidget(QWidget *parent = Q_NULLPTR);
    
    // 设置服务器配置
    void setServerConfig(const QString& roomId, const QString& userId, const QString& sceneName);
    void setConfigLoading(bool loading);
    void setConfigError(const QString& error);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void on_enterRoomBtn_clicked();
    
signals:
    void sigEnterRoom(const QString &roomId, const QString &userId);
    
private:
    Ui::LoginForm ui;
    bool m_useServerConfig = false;
    QString m_serverRoomId;
    QString m_serverUserId;
};
