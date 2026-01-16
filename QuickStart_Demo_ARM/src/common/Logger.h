#pragma once

#include <QString>
#include <QFile>
#include <QMutex>
#include <QDateTime>

/**
 * 日志系统
 * 统一日志格式和级别控制
 */
class Logger {
public:
    enum Level {
        Debug = 0,
        Info = 1,
        Warning = 2,
        Error = 3
    };
    
    // 初始化
    static void init();
    static void shutdown();
    
    // 配置
    static void setLevel(Level level);
    static void setLogFile(const QString& path);
    static void setConsoleOutput(bool enabled);
    
    // 日志输出
    static void debug(const QString& module, const QString& message);
    static void info(const QString& module, const QString& message);
    static void warning(const QString& module, const QString& message);
    static void error(const QString& module, const QString& message);
    
    // 通用日志
    static void log(Level level, const QString& module, const QString& message);

private:
    static QString levelToString(Level level);
    static QString formatMessage(Level level, const QString& module, const QString& message);
    static void writeToFile(const QString& message);
    static void writeToConsole(Level level, const QString& message);
    
    static Level s_level;
    static bool s_consoleOutput;
    static QFile* s_logFile;
    static QMutex s_mutex;
    static bool s_initialized;
};

// 便捷宏 - 在源文件开头定义 LOG_MODULE
#define LOG_DEBUG(msg) Logger::debug(LOG_MODULE, msg)
#define LOG_INFO(msg)  Logger::info(LOG_MODULE, msg)
#define LOG_WARN(msg)  Logger::warning(LOG_MODULE, msg)
#define LOG_ERROR(msg) Logger::error(LOG_MODULE, msg)
