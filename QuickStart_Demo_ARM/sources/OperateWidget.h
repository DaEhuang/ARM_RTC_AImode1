#pragma once

#include <QtWidgets/QMainWindow>
#include <QSlider>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include "ui_OperateWidget.h"
#include "ExternalVideoSource.h"

class OperateWidget : public QWidget {
    Q_OBJECT

public:
    OperateWidget(QWidget *parent = Q_NULLPTR);
    void reset();
    
    // AIGC 相关
    void setAIStarted(bool started);
    void setAILoading(bool loading);
    void setAIEnabled(bool enabled);
    
protected:
    bool eventFilter(QObject *watched, QEvent *event) override;
    void paintEvent(QPaintEvent *event) override;

private slots:
    void on_hangupBtn_clicked();
    void on_muteAudioBtn_clicked();
    void on_muteVideoBtn_clicked();
    void on_muteSpeakerBtn_clicked();
    void onMicVolumeChanged(int value);
    void onSpeakerVolumeChanged(int value);

signals:
    void sigHangup();
    void sigMuteVideo(bool);
    void sigMuteAudio(bool);
    void sigMuteSpeaker(bool);
    void sigMicVolumeChanged(int volume);
    void sigSpeakerVolumeChanged(int volume);
    
    // AIGC 信号
    void sigStartAI();
    void sigStopAI();
    
    // 摄像头切换信号
    void sigCameraChanged(const CameraInfo& camera);

public:
    // 摄像头管理
    void refreshCameras();
    void setCurrentCamera(const CameraInfo& camera);
    
private:
    void setupVolumeControls();
    void setupAIButton();
    void setupCameraSelector();
    void onAIButtonClicked();
    void onCameraChanged(int index);
    
    Ui::OperateForm ui;
    QSlider* m_micVolumeSlider = nullptr;
    QSlider* m_speakerVolumeSlider = nullptr;
    QLabel* m_micLabel = nullptr;
    QLabel* m_speakerLabel = nullptr;
    
    // AIGC 控制按钮
    QPushButton* m_aiButton = nullptr;
    bool m_aiStarted = false;
    bool m_aiLoading = false;
    
    // 摄像头选择
    QComboBox* m_cameraCombo = nullptr;
    QList<CameraInfo> m_cameras;
};
