#include "StyleManager.h"
#include <QFile>
#include <QDebug>
#include <QDir>

StyleManager* StyleManager::s_instance = nullptr;

StyleManager* StyleManager::instance()
{
    if (!s_instance) {
        s_instance = new StyleManager();
    }
    return s_instance;
}

StyleManager::StyleManager(QObject* parent)
    : QObject(parent)
{
}

bool StyleManager::loadTheme(const QString& themePath)
{
    QFile file(themePath);
    if (!file.exists()) {
        qDebug() << "StyleManager: Theme file not found:" << themePath;
        return false;
    }
    
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "StyleManager: Failed to open theme file:" << themePath;
        return false;
    }
    
    m_fullStyleSheet = QString::fromUtf8(file.readAll());
    file.close();
    
    m_themePath = themePath;
    qDebug() << "StyleManager: Theme loaded from" << themePath;
    
    emit themeChanged();
    return true;
}

QString StyleManager::getStyle(const QString& styleName) const
{
    // 从完整样式表中提取特定样式（简化实现）
    // 实际可以解析 QSS 文件中的注释标记
    return m_fullStyleSheet;
}

void StyleManager::applyToWidget(QWidget* widget)
{
    if (widget && !m_fullStyleSheet.isEmpty()) {
        widget->setStyleSheet(m_fullStyleSheet);
    }
}

void StyleManager::applyToWidget(QWidget* widget, const QString& additionalStyle)
{
    if (widget) {
        QString style = m_fullStyleSheet + "\n" + additionalStyle;
        widget->setStyleSheet(style);
    }
}
