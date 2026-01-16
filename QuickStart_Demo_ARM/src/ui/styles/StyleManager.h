#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QWidget>

/**
 * 样式管理器
 * 统一管理 QSS 样式，支持主题切换
 */
class StyleManager : public QObject {
    Q_OBJECT

public:
    static StyleManager* instance();
    
    // 加载主题
    bool loadTheme(const QString& themePath);
    
    // 获取样式
    QString getStyle(const QString& styleName) const;
    QString getFullStyleSheet() const { return m_fullStyleSheet; }
    
    // 应用样式到 Widget
    void applyToWidget(QWidget* widget);
    void applyToWidget(QWidget* widget, const QString& additionalStyle);
    
    // 获取颜色常量
    static QString primaryColor() { return "#4CAF50"; }
    static QString secondaryColor() { return "#2196F3"; }
    static QString accentColor() { return "#635BFF"; }
    static QString backgroundColor() { return "rgba(0, 0, 0, 0.7)"; }
    static QString textColor() { return "#FFFFFF"; }
    static QString textSecondaryColor() { return "rgba(255, 255, 255, 0.7)"; }

signals:
    void themeChanged();

private:
    explicit StyleManager(QObject* parent = nullptr);
    ~StyleManager() = default;
    
    StyleManager(const StyleManager&) = delete;
    StyleManager& operator=(const StyleManager&) = delete;
    
    static StyleManager* s_instance;
    QString m_fullStyleSheet;
    QString m_themePath;
};
