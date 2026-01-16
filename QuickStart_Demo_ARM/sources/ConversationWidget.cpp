#include "ConversationWidget.h"
#include <QPainter>
#include <QScrollBar>
#include <QTimer>

ConversationWidget::ConversationWidget(QWidget* parent)
    : QWidget(parent) {
    setAttribute(Qt::WA_TranslucentBackground);
    
    // 创建滚动区域
    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { background: transparent; border: none; }");
    m_scrollArea->viewport()->setStyleSheet("background: transparent;");
    
    // 内容容器
    m_contentWidget = new QWidget();
    m_contentWidget->setStyleSheet("background: transparent;");
    m_layout = new QVBoxLayout(m_contentWidget);
    m_layout->setContentsMargins(8, 8, 8, 8);
    m_layout->setSpacing(8);
    m_layout->addStretch();  // 初始空白时弹性空间在底部
    
    m_scrollArea->setWidget(m_contentWidget);
    
    // 状态标签 (AI准备中) - 初始隐藏
    m_statusLabel = new QLabel("AI 准备中，请稍候...", this);
    m_statusLabel->setStyleSheet(
        "QLabel {"
        "  color: rgba(255, 255, 255, 0.7);"
        "  font-size: 16px;"
        "  font-weight: 500;"
        "  background: transparent;"
        "}"
    );
    m_statusLabel->setAlignment(Qt::AlignCenter);
    m_statusLabel->hide();  // 初始隐藏
    
    // 主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(m_statusLabel);
    mainLayout->addWidget(m_scrollArea);
}

ConversationWidget::~ConversationWidget() {
}

void ConversationWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    // 透明背景，不绘制任何内容
}

void ConversationWidget::handleMessage(const QString& text, const QString& userId, bool isUser, bool definite) {
    // 收到消息后隐藏"准备中"状态
    if (m_statusLabel->isVisible()) {
        m_statusLabel->hide();
    }
    
    // 参考 React 实现: room.ts setHistoryMsg
    // 如果没有消息，直接添加
    if (m_messages.isEmpty()) {
        ChatMessage msg;
        msg.user = userId;
        msg.content = text;
        msg.isUser = isUser;
        msg.definite = definite;
        msg.isInterrupted = false;
        m_messages.append(msg);
        trimMessages();
        rebuildMessageWidgets();
        scrollToBottom();
        return;
    }
    
    // 获取最后一条消息
    ChatMessage& lastMsg = m_messages.last();
    
    // 如果最后一条消息已完成 (definite=true)，添加新消息
    if (lastMsg.definite) {
        ChatMessage msg;
        msg.user = userId;
        msg.content = text;
        msg.isUser = isUser;
        msg.definite = definite;
        msg.isInterrupted = false;
        m_messages.append(msg);
        trimMessages();
    } else {
        // 最后一条消息未完成，更新它
        lastMsg.content = text;
        lastMsg.definite = definite;
        lastMsg.user = userId;
    }
    
    rebuildMessageWidgets();
    scrollToBottom();
}

void ConversationWidget::setInterrupted() {
    // 找到最后一条未完成的AI消息，标记为打断
    for (int i = m_messages.size() - 1; i >= 0; --i) {
        if (!m_messages[i].isUser && !m_messages[i].definite) {
            m_messages[i].isInterrupted = true;
            rebuildMessageWidgets();
            break;
        }
    }
}

void ConversationWidget::trimMessages() {
    // 只保留最后2条对话 (用户+AI 为一组对话)
    const int maxMessages = 4;  // 2条对话 = 最多4条消息
    while (m_messages.size() > maxMessages) {
        m_messages.removeFirst();
    }
}

void ConversationWidget::rebuildMessageWidgets() {
    // 清空现有widgets
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    
    // 按顺序添加消息 (第一条在最上面，新消息追加到下面)
    for (const auto& msg : m_messages) {
        QWidget* widget = createMessageWidget(msg);
        m_layout->addWidget(widget);
    }
    
    // 底部弹性空间填充剩余区域
    m_layout->addStretch();
}

void ConversationWidget::clearMessages() {
    m_messages.clear();
    
    QLayoutItem* item;
    while ((item = m_layout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }
    m_layout->addStretch();
}

void ConversationWidget::setAIReady(bool ready) {
    m_aiReady = ready;
    m_statusLabel->setVisible(!ready);
}

void ConversationWidget::setUserName(const QString& name) {
    m_userName = name;
}

void ConversationWidget::setAIName(const QString& name) {
    m_aiName = name;
}

QWidget* ConversationWidget::createMessageWidget(const ChatMessage& msg) {
    QWidget* container = new QWidget();
    container->setStyleSheet("background: transparent;");
    
    QVBoxLayout* layout = new QVBoxLayout(container);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(4);
    
    // 用户名标签
    QLabel* nameLabel = new QLabel(msg.isUser ? m_userName : m_aiName);
    nameLabel->setStyleSheet(
        "QLabel {"
        "  color: rgba(255, 255, 255, 0.8);"
        "  font-size: 12px;"
        "  background: transparent;"
        "  padding-left: 4px;"
        "}"
    );
    
    // 消息内容
    QLabel* contentLabel = new QLabel(msg.content);
    contentLabel->setWordWrap(true);
    contentLabel->setMaximumWidth(700);  // 适配横屏
    
    if (msg.isUser) {
        // 用户消息样式 - 半透明灰色背景
        contentLabel->setStyleSheet(
            "QLabel {"
            "  color: #FFFFFF;"
            "  font-size: 14px;"
            "  font-weight: 500;"
            "  background: rgba(0, 0, 0, 0.25);"
            "  border-radius: 12px;"
            "  padding: 8px 12px;"
            "}"
        );
    } else {
        // AI消息样式 - 半透明蓝色背景
        QString style = 
            "QLabel {"
            "  color: #FFFFFF;"
            "  font-size: 14px;"
            "  font-weight: 500;"
            "  background: rgba(0, 12, 71, 0.5);"
            "  border-radius: 12px;"
            "  padding: 8px 12px;"
            "}";
        contentLabel->setStyleSheet(style);
    }
    
    layout->addWidget(nameLabel);
    layout->addWidget(contentLabel);
    
    // 打断标签
    if (!msg.isUser && msg.isInterrupted) {
        QLabel* interruptLabel = new QLabel("已打断");
        interruptLabel->setStyleSheet(
            "QLabel {"
            "  color: #635BFF;"
            "  font-size: 11px;"
            "  background: rgba(99, 91, 255, 0.2);"
            "  border-radius: 4px;"
            "  padding: 2px 6px;"
            "}"
        );
        interruptLabel->setFixedWidth(50);
        layout->addWidget(interruptLabel);
    }
    
    return container;
}

void ConversationWidget::addMessageWidget(const ChatMessage& msg) {
    QWidget* widget = createMessageWidget(msg);
    
    // 在 stretch 之前插入
    int insertIndex = m_layout->count() - 1;
    if (insertIndex < 0) insertIndex = 0;
    m_layout->insertWidget(insertIndex, widget);
}

void ConversationWidget::scrollToBottom() {
    QTimer::singleShot(50, [this]() {
        QScrollBar* vbar = m_scrollArea->verticalScrollBar();
        vbar->setValue(vbar->maximum());
    });
}
