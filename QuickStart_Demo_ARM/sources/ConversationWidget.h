#pragma once

#include <QWidget>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QList>
#include <QString>

struct ChatMessage {
    QString user;       // 用户ID或"AI"
    QString content;    // 消息内容
    bool isUser;        // 是否是用户消息
    bool definite;      // 消息是否完整
    bool isInterrupted; // AI消息是否被打断
};

/**
 * 对话字幕组件
 * 显示用户和AI的对话记录，覆盖在视频上方
 */
class ConversationWidget : public QWidget {
    Q_OBJECT

public:
    explicit ConversationWidget(QWidget* parent = nullptr);
    ~ConversationWidget();

    // 统一的消息接口 - 参考 React 实现
    void handleMessage(const QString& text, const QString& userId, bool isUser, bool definite);
    void setInterrupted();
    void clearMessages();
    void setAIReady(bool ready);
    void setUserName(const QString& name);
    void setAIName(const QString& name);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    void addMessageWidget(const ChatMessage& msg);
    void scrollToBottom();
    QWidget* createMessageWidget(const ChatMessage& msg);
    void trimMessages();
    void rebuildMessageWidgets();

private:
    QScrollArea* m_scrollArea;
    QWidget* m_contentWidget;
    QVBoxLayout* m_layout;
    QList<ChatMessage> m_messages;
    QLabel* m_statusLabel;
    bool m_aiReady = false;
    QString m_userName = "我";
    QString m_aiName = "AI";
};
