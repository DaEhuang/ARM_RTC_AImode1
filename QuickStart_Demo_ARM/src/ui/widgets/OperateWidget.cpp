#include "OperateWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDebug>

OperateWidget::OperateWidget(QWidget *parent)
        : QWidget(parent) {
    ui.setupUi(this);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | windowFlags());
    setAttribute(Qt::WA_TranslucentBackground);
    parent->installEventFilter(this);

    connect(this, SIGNAL(sigHangup()), parent, SLOT(slotOnHangup()));
    
    // 手动连接按钮点击信号（确保信号槽连接生效）
    connect(ui.muteAudioBtn, &QPushButton::clicked, this, &OperateWidget::on_muteAudioBtn_clicked);
    connect(ui.muteVideoBtn, &QPushButton::clicked, this, &OperateWidget::on_muteVideoBtn_clicked);
    connect(ui.muteSpeakerBtn, &QPushButton::clicked, this, &OperateWidget::on_muteSpeakerBtn_clicked);
    connect(ui.hangupBtn, &QPushButton::clicked, this, &OperateWidget::on_hangupBtn_clicked);
    
    setupVolumeControls();
    setupCameraSelector();
}

void OperateWidget::setupVolumeControls() {
    // 在操作栏左侧添加紧凑的音量滑块
    // 麦克风音量 (绿色，放在音频按钮旁边)
    m_micVolumeSlider = new QSlider(Qt::Horizontal, this);
    m_micVolumeSlider->setRange(0, 100);
    m_micVolumeSlider->setValue(100);
    m_micVolumeSlider->setFixedSize(60, 20);
    m_micVolumeSlider->setToolTip("麦克风音量");
    m_micVolumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #4CAF50; width: 10px; margin: -3px 0; border-radius: 5px; }"
        "QSlider::sub-page:horizontal { background: #4CAF50; border-radius: 2px; }"
    );
    
    // 喇叭音量 (蓝色)
    m_speakerVolumeSlider = new QSlider(Qt::Horizontal, this);
    m_speakerVolumeSlider->setRange(0, 100);
    m_speakerVolumeSlider->setValue(100);
    m_speakerVolumeSlider->setFixedSize(60, 20);
    m_speakerVolumeSlider->setToolTip("喇叭音量");
    m_speakerVolumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 4px; border-radius: 2px; }"
        "QSlider::handle:horizontal { background: #2196F3; width: 10px; margin: -3px 0; border-radius: 5px; }"
        "QSlider::sub-page:horizontal { background: #2196F3; border-radius: 2px; }"
    );
    
    // 将滑块插入到现有布局中
    QHBoxLayout* mainLayout = qobject_cast<QHBoxLayout*>(ui.muteAudioBtn->parentWidget()->layout());
    if (mainLayout) {
        // 在音频按钮后面插入麦克风音量
        int audioIndex = mainLayout->indexOf(ui.muteAudioBtn);
        if (audioIndex >= 0) {
            mainLayout->insertWidget(audioIndex + 1, m_micVolumeSlider);
        }
        // 在喇叭按钮后面插入喇叭音量
        int speakerIndex = mainLayout->indexOf(ui.muteSpeakerBtn);
        if (speakerIndex >= 0) {
            mainLayout->insertWidget(speakerIndex + 1, m_speakerVolumeSlider);
        }
    }
    
    // 连接信号
    connect(m_micVolumeSlider, &QSlider::valueChanged, this, &OperateWidget::onMicVolumeChanged);
    connect(m_speakerVolumeSlider, &QSlider::valueChanged, this, &OperateWidget::onSpeakerVolumeChanged);
}

void OperateWidget::onMicVolumeChanged(int value) {
    emit sigMicVolumeChanged(value);
}

void OperateWidget::onSpeakerVolumeChanged(int value) {
    emit sigSpeakerVolumeChanged(value);
}

bool OperateWidget::eventFilter(QObject *watched, QEvent *event) {
    if (watched == parent()) {
        auto parentWindow = dynamic_cast<QWidget *>(parent());
        if (parentWindow == nullptr) {
            return false;
        }
        if (event->type() == QEvent::Resize || event->type() == QEvent::Move) {
            // 使用父窗口的 rect() 而非 geometry()，获取相对坐标
            auto selfRect = this->rect();
            auto parentRect = parentWindow->rect();

            int nleft = (parentRect.width() - selfRect.width()) / 2;
            int nTop = parentRect.height() - 60 - selfRect.height();

            setGeometry(nleft, nTop, selfRect.width(), selfRect.height());
        }
    }
    return false;
}

void OperateWidget::paintEvent(QPaintEvent *event) {
    QPainter p(this);
    QBrush b("#36393F");
    p.setBrush(b);
    QPen pen(Qt::transparent);
    p.setPen(pen);

    p.drawRoundedRect(this->rect(), 30, 30);
}

void OperateWidget::on_hangupBtn_clicked() {
    emit sigHangup();
}

void OperateWidget::on_muteAudioBtn_clicked() {
    qDebug() << "on_muteAudioBtn_clicked called, isChecked:" << ui.muteAudioBtn->isChecked();
    emit sigMuteAudio(ui.muteAudioBtn->isChecked());
}

void OperateWidget::on_muteVideoBtn_clicked() {
    emit sigMuteVideo(ui.muteVideoBtn->isChecked());
}

void OperateWidget::on_muteSpeakerBtn_clicked() {
    qDebug() << "on_muteSpeakerBtn_clicked called, isChecked:" << ui.muteSpeakerBtn->isChecked();
    emit sigMuteSpeaker(ui.muteSpeakerBtn->isChecked());
}

void OperateWidget::reset()
{
	ui.muteAudioBtn->blockSignals(true);
	ui.muteAudioBtn->clicked(false);
	ui.muteAudioBtn->setChecked(false);
	ui.muteAudioBtn->blockSignals(false);

	ui.muteVideoBtn->blockSignals(true);
	ui.muteVideoBtn->clicked(false);
	ui.muteVideoBtn->setChecked(false);
	ui.muteVideoBtn->blockSignals(false);
	
}

void OperateWidget::setupCameraSelector()
{
    // 创建摄像头选择下拉框 - 紧凑圆形按钮样式
    m_cameraCombo = new QComboBox(this);
    m_cameraCombo->setFixedSize(50, 50);
    m_cameraCombo->setToolTip("选择摄像头");
    m_cameraCombo->setStyleSheet(
        "QComboBox {"
        "  background: #3d3d3d;"
        "  color: white;"
        "  border: none;"
        "  border-radius: 25px;"
        "  font-size: 16px;"
        "  padding-left: 15px;"
        "}"
        "QComboBox:hover {"
        "  background: #4d4d4d;"
        "}"
        "QComboBox::drop-down {"
        "  border: none;"
        "  width: 0px;"
        "}"
        "QComboBox::down-arrow {"
        "  image: none;"
        "}"
        "QComboBox QAbstractItemView {"
        "  background: #3d3d3d;"
        "  color: white;"
        "  selection-background-color: #0078d4;"
        "  min-width: 150px;"
        "}"
    );
    
    // 将摄像头选择器插入到布局最左边 (在摄像头开关按钮后面)
    QHBoxLayout* mainLayout = qobject_cast<QHBoxLayout*>(ui.muteAudioBtn->parentWidget()->layout());
    if (mainLayout) {
        // 找到 muteVideoBtn 的位置，在其后面插入摄像头选择器
        int videoIndex = mainLayout->indexOf(ui.muteVideoBtn);
        if (videoIndex >= 0) {
            mainLayout->insertWidget(videoIndex + 1, m_cameraCombo);
        }
    }
    
    // 连接信号
    connect(m_cameraCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OperateWidget::onCameraChanged);
    
    // 初始化摄像头列表
    refreshCameras();
}

void OperateWidget::refreshCameras()
{
    if (!m_cameraCombo) return;
    
    m_cameraCombo->blockSignals(true);
    m_cameraCombo->clear();
    
    m_cameras = ExternalVideoSource::detectCamerasStatic();
    
    // 下拉框显示简短序号，完整名称在下拉列表中显示
    for (int i = 0; i < m_cameras.size(); i++) {
        const CameraInfo& cam = m_cameras[i];
        // 显示文本用序号，但下拉列表显示完整名称
        QString displayText = QString("%1").arg(i + 1);
        m_cameraCombo->addItem(displayText, cam.id);
        // 设置下拉列表中的完整名称
        m_cameraCombo->setItemData(i, cam.name, Qt::ToolTipRole);
    }
    
    if (m_cameras.isEmpty()) {
        m_cameraCombo->addItem("×", "");
        m_cameraCombo->setEnabled(false);
        m_cameraCombo->setToolTip("无摄像头");
    } else {
        m_cameraCombo->setEnabled(true);
        updateCameraTooltip();
    }
    
    m_cameraCombo->blockSignals(false);
}

void OperateWidget::updateCameraTooltip()
{
    if (!m_cameraCombo || m_cameras.isEmpty()) return;
    int idx = m_cameraCombo->currentIndex();
    if (idx >= 0 && idx < m_cameras.size()) {
        m_cameraCombo->setToolTip(m_cameras[idx].name);
    }
}

void OperateWidget::setCurrentCamera(const CameraInfo& camera)
{
    if (!m_cameraCombo) return;
    
    for (int i = 0; i < m_cameraCombo->count(); i++) {
        if (m_cameraCombo->itemData(i).toString() == camera.id) {
            m_cameraCombo->blockSignals(true);
            m_cameraCombo->setCurrentIndex(i);
            m_cameraCombo->blockSignals(false);
            break;
        }
    }
}

void OperateWidget::onCameraChanged(int index)
{
    if (index < 0 || index >= m_cameras.size()) return;
    
    const CameraInfo& camera = m_cameras[index];
    qDebug() << "OperateWidget: camera changed to" << camera.name;
    updateCameraTooltip();
    emit sigCameraChanged(camera);
}