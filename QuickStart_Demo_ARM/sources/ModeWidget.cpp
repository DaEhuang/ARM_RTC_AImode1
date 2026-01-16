#include "ModeWidget.h"
#include <QDebug>
#include <QPainter>

ModeWidget::ModeWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
    setAttribute(Qt::WA_TranslucentBackground);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | windowFlags());
    parent->installEventFilter(this);
    
    setupButtonGroup();
    
    // 默认选中待机模式
    setMode(AIMode::Standby);
}

bool ModeWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (watched == parent()) {
        QWidget* parentWindow = qobject_cast<QWidget*>(parent());
        if (parentWindow == nullptr) {
            return false;
        }
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
            auto selfRect = this->rect();
            auto parentRect = parentWindow->rect();

            int nleft = (parentRect.width() - selfRect.width()) / 2;
            // 定位在 OperateWidget 上方 (OperateWidget 在底部 60px 处，高度约 59px)
            int nTop = parentRect.height() - 60 - 59 - 10 - selfRect.height();

            setGeometry(nleft, nTop, selfRect.width(), selfRect.height());
        }
    }
    return false;
}

void ModeWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    QBrush b(QColor(54, 57, 63, 200));
    p.setBrush(b);
    QPen pen(Qt::transparent);
    p.setPen(pen);
    p.drawRoundedRect(this->rect(), 22, 22);
}

void ModeWidget::setupButtonGroup()
{
    m_buttonGroup = new QButtonGroup(this);
    m_buttonGroup->setExclusive(true);
    
    m_buttonGroup->addButton(ui.chatBtn, static_cast<int>(AIMode::Chat));
    m_buttonGroup->addButton(ui.superviseBtn, static_cast<int>(AIMode::Supervise));
    m_buttonGroup->addButton(ui.teachBtn, static_cast<int>(AIMode::Teach));
    m_buttonGroup->addButton(ui.standbyBtn, static_cast<int>(AIMode::Standby));
    
    connect(m_buttonGroup, QOverload<int>::of(&QButtonGroup::buttonClicked),
            this, &ModeWidget::onModeButtonClicked);
}

void ModeWidget::setMode(AIMode mode)
{
    m_currentMode = mode;
    
    QAbstractButton* btn = m_buttonGroup->button(static_cast<int>(mode));
    if (btn) {
        btn->setChecked(true);
    }
}

void ModeWidget::onModeButtonClicked(int id)
{
    AIMode newMode = static_cast<AIMode>(id);
    if (newMode != m_currentMode) {
        m_currentMode = newMode;
        
        QString modeName;
        switch (newMode) {
            case AIMode::Chat: modeName = "闲聊"; break;
            case AIMode::Supervise: modeName = "监督"; break;
            case AIMode::Teach: modeName = "教学"; break;
            case AIMode::Standby: modeName = "待机"; break;
        }
        qDebug() << "Mode changed to:" << modeName;
        
        emit sigModeChanged(newMode);
    }
}
