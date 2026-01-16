#pragma once

#include <QString>

/**
 * 错误码枚举
 * 按模块分段，便于扩展和管理
 */
enum class ErrorCode {
    // 通用错误 (0-99)
    Success = 0,
    Unknown = 1,
    InvalidParameter = 2,
    NotInitialized = 3,
    
    // RTC 错误 (100-199)
    RtcEngineCreateFailed = 100,
    RtcEngineNotCreated = 101,
    RtcEngineAlreadyExists = 102,
    RtcRoomCreateFailed = 110,
    RtcRoomJoinFailed = 111,
    RtcRoomAlreadyJoined = 112,
    RtcRoomNotJoined = 113,
    
    // AI 错误 (200-299)
    AiConfigNotLoaded = 200,
    AiStartFailed = 201,
    AiStopFailed = 202,
    AiServerError = 203,
    AiAlreadyStarted = 204,
    AiNotStarted = 205,
    
    // 媒体错误 (300-399)
    MediaEngineNull = 300,
    MediaCameraNotFound = 301,
    MediaAudioDeviceNotFound = 302,
    MediaCaptureStartFailed = 303,
    MediaCaptureStopFailed = 304,
    MediaRenderStartFailed = 305,
    MediaRenderStopFailed = 306,
    
    // 配置错误 (400-499)
    ConfigFileNotFound = 400,
    ConfigParseError = 401,
    ConfigInvalid = 402,
};

/**
 * 应用错误类
 * 封装错误码、用户消息和技术细节
 */
class AppError {
public:
    AppError() : m_code(ErrorCode::Success) {}
    
    AppError(ErrorCode code) 
        : m_code(code), m_message(getDefaultMessage(code)) {}
    
    AppError(ErrorCode code, const QString& message)
        : m_code(code), m_message(message) {}
    
    AppError(ErrorCode code, const QString& message, const QString& detail)
        : m_code(code), m_message(message), m_detail(detail) {}
    
    // 访问器
    ErrorCode code() const { return m_code; }
    QString message() const { return m_message; }
    QString detail() const { return m_detail; }
    int codeValue() const { return static_cast<int>(m_code); }
    
    // 状态检查
    bool isSuccess() const { return m_code == ErrorCode::Success; }
    bool isError() const { return m_code != ErrorCode::Success; }
    operator bool() const { return isSuccess(); }
    
    // 格式化输出
    QString toString() const {
        if (m_detail.isEmpty()) {
            return QString("[%1] %2").arg(codeValue()).arg(m_message);
        }
        return QString("[%1] %2 (%3)").arg(codeValue()).arg(m_message).arg(m_detail);
    }
    
    // 获取默认错误消息
    static QString getDefaultMessage(ErrorCode code) {
        switch (code) {
            // 通用错误
            case ErrorCode::Success:
                return QStringLiteral("成功");
            case ErrorCode::Unknown:
                return QStringLiteral("未知错误");
            case ErrorCode::InvalidParameter:
                return QStringLiteral("无效参数");
            case ErrorCode::NotInitialized:
                return QStringLiteral("未初始化");
            
            // RTC 错误
            case ErrorCode::RtcEngineCreateFailed:
                return QStringLiteral("RTC 引擎创建失败");
            case ErrorCode::RtcEngineNotCreated:
                return QStringLiteral("RTC 引擎未创建");
            case ErrorCode::RtcEngineAlreadyExists:
                return QStringLiteral("RTC 引擎已存在");
            case ErrorCode::RtcRoomCreateFailed:
                return QStringLiteral("房间创建失败");
            case ErrorCode::RtcRoomJoinFailed:
                return QStringLiteral("加入房间失败");
            case ErrorCode::RtcRoomAlreadyJoined:
                return QStringLiteral("已在房间中");
            case ErrorCode::RtcRoomNotJoined:
                return QStringLiteral("未加入房间");
            
            // AI 错误
            case ErrorCode::AiConfigNotLoaded:
                return QStringLiteral("AI 配置未加载");
            case ErrorCode::AiStartFailed:
                return QStringLiteral("AI 启动失败");
            case ErrorCode::AiStopFailed:
                return QStringLiteral("AI 停止失败");
            case ErrorCode::AiServerError:
                return QStringLiteral("AI 服务器错误");
            case ErrorCode::AiAlreadyStarted:
                return QStringLiteral("AI 已启动");
            case ErrorCode::AiNotStarted:
                return QStringLiteral("AI 未启动");
            
            // 媒体错误
            case ErrorCode::MediaEngineNull:
                return QStringLiteral("媒体引擎为空");
            case ErrorCode::MediaCameraNotFound:
                return QStringLiteral("未找到摄像头");
            case ErrorCode::MediaAudioDeviceNotFound:
                return QStringLiteral("未找到音频设备");
            case ErrorCode::MediaCaptureStartFailed:
                return QStringLiteral("采集启动失败");
            case ErrorCode::MediaCaptureStopFailed:
                return QStringLiteral("采集停止失败");
            case ErrorCode::MediaRenderStartFailed:
                return QStringLiteral("渲染启动失败");
            case ErrorCode::MediaRenderStopFailed:
                return QStringLiteral("渲染停止失败");
            
            // 配置错误
            case ErrorCode::ConfigFileNotFound:
                return QStringLiteral("配置文件未找到");
            case ErrorCode::ConfigParseError:
                return QStringLiteral("配置解析错误");
            case ErrorCode::ConfigInvalid:
                return QStringLiteral("配置无效");
            
            default:
                return QStringLiteral("未知错误");
        }
    }

private:
    ErrorCode m_code;
    QString m_message;
    QString m_detail;
};

// 便捷函数
inline AppError makeSuccess() {
    return AppError(ErrorCode::Success);
}

inline AppError makeError(ErrorCode code) {
    return AppError(code);
}

inline AppError makeError(ErrorCode code, const QString& detail) {
    return AppError(code, AppError::getDefaultMessage(code), detail);
}
