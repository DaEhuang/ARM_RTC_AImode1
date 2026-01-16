#include "RoomMainWidget.h"
#include "ConfigManager.h"
#include "Logger.h"
#include "StyleManager.h"
#include <QtWidgets/QApplication>
#include <QScreen>
#include <QDir>

#define LOG_MODULE "Main"

int main(int argc, char *argv[]) {
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);
    a.setWindowIcon(QIcon(":/QuickStart/app.ico"));
    
    // 初始化日志系统
    Logger::init();
    LOG_INFO("Application starting...");
    
    // 加载配置文件
    QString configPath = QDir(QCoreApplication::applicationDirPath()).filePath("../config/config.json");
    ConfigManager::instance()->loadFromFile(configPath);
    LOG_INFO(QString("Config loaded from: %1").arg(configPath));
    
    // 加载样式主题
    StyleManager::instance()->loadTheme(":/QuickStart/../src/ui/styles/dark.qss");
    LOG_INFO("Theme loaded");
    
    RoomMainWidget w;
    // 全屏无边框，适配4.3寸横屏 (800x480)
    w.setWindowFlags(Qt::FramelessWindowHint);
    
    // 获取屏幕尺寸并全屏显示
    QScreen *screen = QGuiApplication::primaryScreen();
    QRect screenGeometry = screen->geometry();
    w.setGeometry(screenGeometry);
    w.showFullScreen();

    return a.exec();
}
