#pragma once

#include <QtWidgets/QWidget>
#include <QPushButton>
#include <QButtonGroup>
#include "ui_ModeWidget.h"

enum class AIMode {
    Chat = 0,      // 闲聊
    Supervise,     // 监督
    Teach,         // 教学
    Standby        // 待机
};

class ModeWidget : public QWidget {
    Q_OBJECT

public:
    ModeWidget(QWidget *parent = Q_NULLPTR);
    
    AIMode currentMode() const { return m_currentMode; }
    void setMode(AIMode mode);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

signals:
    void sigModeChanged(AIMode mode);

private slots:
    void onModeButtonClicked(int id);

private:
    void setupButtonGroup();
    
    Ui::ModeForm ui;
    QButtonGroup* m_buttonGroup = nullptr;
    AIMode m_currentMode = AIMode::Standby;
};
