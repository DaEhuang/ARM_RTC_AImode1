#include "RoomMainWidget.h"
#include <QtWidgets/QApplication>
#include <QScreen>

int main(int argc, char *argv[]) {
    qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");

    QApplication a(argc, argv);
    a.setQuitOnLastWindowClosed(true);
    a.setWindowIcon(QIcon(":/QuickStart/app.ico"));
    
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
