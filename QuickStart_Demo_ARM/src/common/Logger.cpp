#include "Logger.h"
#include <QDebug>
#include <QDir>
#include <QTextStream>

Logger::Level Logger::s_level = Logger::Debug;
bool Logger::s_consoleOutput = true;
QFile* Logger::s_logFile = nullptr;
QMutex Logger::s_mutex;
bool Logger::s_initialized = false;

void Logger::init()
{
    QMutexLocker locker(&s_mutex);
    if (s_initialized) return;
    
    s_level = Debug;
    s_consoleOutput = true;
    s_initialized = true;
}

void Logger::shutdown()
{
    QMutexLocker locker(&s_mutex);
    if (s_logFile) {
        s_logFile->close();
        delete s_logFile;
        s_logFile = nullptr;
    }
    s_initialized = false;
}

void Logger::setLevel(Level level)
{
    QMutexLocker locker(&s_mutex);
    s_level = level;
}

void Logger::setLogFile(const QString& path)
{
    QMutexLocker locker(&s_mutex);
    
    if (s_logFile) {
        s_logFile->close();
        delete s_logFile;
        s_logFile = nullptr;
    }
    
    if (path.isEmpty()) return;
    
    // 确保目录存在
    QFileInfo fileInfo(path);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    s_logFile = new QFile(path);
    if (!s_logFile->open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        qWarning() << "Logger: Failed to open log file:" << path;
        delete s_logFile;
        s_logFile = nullptr;
    }
}

void Logger::setConsoleOutput(bool enabled)
{
    QMutexLocker locker(&s_mutex);
    s_consoleOutput = enabled;
}

void Logger::debug(const QString& module, const QString& message)
{
    log(Debug, module, message);
}

void Logger::info(const QString& module, const QString& message)
{
    log(Info, module, message);
}

void Logger::warning(const QString& module, const QString& message)
{
    log(Warning, module, message);
}

void Logger::error(const QString& module, const QString& message)
{
    log(Error, module, message);
}

void Logger::log(Level level, const QString& module, const QString& message)
{
    QMutexLocker locker(&s_mutex);
    
    if (level < s_level) return;
    
    QString formatted = formatMessage(level, module, message);
    
    if (s_consoleOutput) {
        writeToConsole(level, formatted);
    }
    
    if (s_logFile) {
        writeToFile(formatted);
    }
}

QString Logger::levelToString(Level level)
{
    switch (level) {
        case Debug:   return "DEBUG";
        case Info:    return "INFO ";
        case Warning: return "WARN ";
        case Error:   return "ERROR";
        default:      return "?????";
    }
}

QString Logger::formatMessage(Level level, const QString& module, const QString& message)
{
    QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
    return QString("[%1] [%2] [%3] %4")
        .arg(timestamp)
        .arg(levelToString(level))
        .arg(module, -15)  // 左对齐，宽度15
        .arg(message);
}

void Logger::writeToFile(const QString& message)
{
    if (!s_logFile) return;
    
    QTextStream stream(s_logFile);
    stream << message << "\n";
    stream.flush();
}

void Logger::writeToConsole(Level level, const QString& message)
{
    switch (level) {
        case Debug:
        case Info:
            qDebug().noquote() << message;
            break;
        case Warning:
            qWarning().noquote() << message;
            break;
        case Error:
            qCritical().noquote() << message;
            break;
    }
}
