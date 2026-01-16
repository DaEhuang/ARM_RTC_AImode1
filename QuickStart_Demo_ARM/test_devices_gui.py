#!/usr/bin/env python3
"""
树莓派麦克风和摄像头测试 GUI Demo
带图形界面的设备测试工具
"""

import sys
import os
import subprocess
import threading
import time

try:
    from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                                  QHBoxLayout, QPushButton, QLabel, QTextEdit,
                                  QGroupBox, QProgressBar, QFrame, QComboBox)
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread
    from PyQt5.QtGui import QImage, QPixmap, QFont
except ImportError:
    print("正在安装 PyQt5...")
    subprocess.check_call([sys.executable, "-m", "pip", "install", "PyQt5", "--break-system-packages", "-q"])
    from PyQt5.QtWidgets import (QApplication, QMainWindow, QWidget, QVBoxLayout, 
                                  QHBoxLayout, QPushButton, QLabel, QTextEdit,
                                  QGroupBox, QProgressBar, QFrame, QComboBox)
    from PyQt5.QtCore import Qt, QTimer, pyqtSignal, QThread
    from PyQt5.QtGui import QImage, QPixmap, QFont

try:
    import pyaudio
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "pyaudio", "--break-system-packages", "-q"])
    import pyaudio

try:
    import numpy as np
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "numpy", "--break-system-packages", "-q"])
    import numpy as np

try:
    import cv2
except ImportError:
    subprocess.check_call([sys.executable, "-m", "pip", "install", "opencv-python", "--break-system-packages", "-q"])
    import cv2


class AudioThread(QThread):
    """音频录制线程"""
    level_signal = pyqtSignal(int)
    status_signal = pyqtSignal(str)
    
    def __init__(self):
        super().__init__()
        self.running = False
        self.p = None
        self.stream = None
        
    def run(self):
        self.running = True
        try:
            self.p = pyaudio.PyAudio()
            
            # 查找USB麦克风
            device_index = None
            for i in range(self.p.get_device_count()):
                info = self.p.get_device_info_by_index(i)
                if info['maxInputChannels'] > 0:
                    device_index = i
                    self.status_signal.emit(f"使用设备: {info['name']}")
                    break
            
            if device_index is None:
                self.status_signal.emit("❌ 未找到麦克风")
                return
            
            CHUNK = 1024
            FORMAT = pyaudio.paInt16
            CHANNELS = 1
            RATE = 16000
            
            self.stream = self.p.open(format=FORMAT,
                                      channels=CHANNELS,
                                      rate=RATE,
                                      input=True,
                                      input_device_index=device_index,
                                      frames_per_buffer=CHUNK)
            
            self.status_signal.emit("✅ 麦克风已启动")
            
            while self.running:
                try:
                    data = self.stream.read(CHUNK, exception_on_overflow=False)
                    amplitude = max(abs(int.from_bytes(data[i:i+2], 'little', signed=True)) 
                                  for i in range(0, min(len(data), 200), 2))
                    level = min(100, int(amplitude / 327))
                    self.level_signal.emit(level)
                except Exception as e:
                    pass
                    
        except Exception as e:
            self.status_signal.emit(f"❌ 麦克风错误: {str(e)[:50]}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        if self.stream:
            try:
                self.stream.stop_stream()
                self.stream.close()
            except:
                pass
        if self.p:
            try:
                self.p.terminate()
            except:
                pass
    
    def stop(self):
        self.running = False
        self.wait(1000)


class SpeakerThread(QThread):
    """喇叭测试线程"""
    status_signal = pyqtSignal(str)
    finished_signal = pyqtSignal(bool)
    
    def __init__(self):
        super().__init__()
        self.p = None
        self.stream = None
        
    def run(self):
        try:
            self.p = pyaudio.PyAudio()
            
            # 检查输出设备
            output_devices = []
            for i in range(self.p.get_device_count()):
                info = self.p.get_device_info_by_index(i)
                if info['maxOutputChannels'] > 0:
                    output_devices.append((i, info['name']))
            
            if not output_devices:
                self.status_signal.emit("❌ 未找到输出设备")
                self.finished_signal.emit(False)
                return
            
            self.status_signal.emit("🔊 正在播放...")
            
            # 获取默认输出设备支持的采样率
            default_output = self.p.get_default_output_device_info()
            RATE = int(default_output.get('defaultSampleRate', 48000))
            
            DURATION = 1.5
            FREQUENCY = 440
            
            t = np.linspace(0, DURATION, int(RATE * DURATION), False)
            envelope = np.ones_like(t)
            fade_samples = int(RATE * 0.05)
            envelope[:fade_samples] = np.linspace(0, 1, fade_samples)
            envelope[-fade_samples:] = np.linspace(1, 0, fade_samples)
            
            tone = np.sin(2 * np.pi * FREQUENCY * t) * envelope * 0.5
            audio_data = (tone * 32767).astype(np.int16).tobytes()
            
            self.stream = self.p.open(format=pyaudio.paInt16,
                                      channels=1,
                                      rate=RATE,
                                      output=True)
            
            self.stream.write(audio_data)
            
            self.status_signal.emit("✅ 播放完成")
            self.finished_signal.emit(True)
            
        except Exception as e:
            self.status_signal.emit(f"❌ 错误: {str(e)[:30]}")
            self.finished_signal.emit(False)
        finally:
            self.cleanup()
    
    def cleanup(self):
        if self.stream:
            try:
                self.stream.stop_stream()
                self.stream.close()
            except:
                pass
        if self.p:
            try:
                self.p.terminate()
            except:
                pass


class CameraThread(QThread):
    """摄像头线程 - 支持CSI和USB摄像头"""
    frame_signal = pyqtSignal(QImage)
    status_signal = pyqtSignal(str)
    
    def __init__(self, camera_type="CSI", usb_device_index=0):
        super().__init__()
        self.running = False
        self.process = None
        self.cap = None
        self.camera_type = camera_type
        self.usb_device_index = usb_device_index
        
    def run(self):
        self.running = True
        if self.camera_type == "CSI":
            self.run_csi_camera()
        else:
            self.run_usb_camera()
    
    def run_csi_camera(self):
        """运行CSI摄像头 (rpicam)"""
        try:
            result = subprocess.run(['rpicam-hello', '--list-cameras'],
                                   capture_output=True, text=True, timeout=3)
            
            if 'Available cameras' not in result.stdout:
                self.status_signal.emit("❌ 未检测到CSI摄像头")
                return
            
            self.status_signal.emit("✅ CSI摄像头已启动")
            
            cmd = [
                'rpicam-vid', '-t', '0', '--inline', '--nopreview',
                '--width', '640', '--height', '480', '--framerate', '15',
                '--codec', 'mjpeg', '-o', '-'
            ]
            
            self.process = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
            
            buffer = b''
            while self.running and self.process.poll() is None:
                chunk = self.process.stdout.read(4096)
                if not chunk:
                    break
                buffer += chunk
                
                start = buffer.find(b'\xff\xd8')
                end = buffer.find(b'\xff\xd9')
                
                if start != -1 and end != -1 and end > start:
                    jpg = buffer[start:end+2]
                    buffer = buffer[end+2:]
                    
                    nparr = np.frombuffer(jpg, np.uint8)
                    frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
                    
                    if frame is not None:
                        rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                        h, w, ch = rgb.shape
                        qimg = QImage(rgb.data, w, h, ch * w, QImage.Format_RGB888)
                        self.frame_signal.emit(qimg.copy())
                        
        except subprocess.TimeoutExpired:
            self.status_signal.emit("❌ CSI摄像头检测超时")
        except FileNotFoundError:
            self.status_signal.emit("❌ rpicam未安装")
        except Exception as e:
            self.status_signal.emit(f"❌ CSI摄像头错误: {str(e)[:50]}")
        finally:
            self.cleanup()
    
    def run_usb_camera(self):
        """运行USB摄像头 (OpenCV)"""
        try:
            self.cap = cv2.VideoCapture(self.usb_device_index)
            
            if not self.cap.isOpened():
                self.status_signal.emit(f"❌ 无法打开USB摄像头 /dev/video{self.usb_device_index}")
                return
            
            self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
            self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)
            self.cap.set(cv2.CAP_PROP_FPS, 15)
            
            self.status_signal.emit(f"✅ USB摄像头已启动 (video{self.usb_device_index})")
            
            while self.running:
                ret, frame = self.cap.read()
                if not ret:
                    continue
                
                rgb = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
                h, w, ch = rgb.shape
                qimg = QImage(rgb.data, w, h, ch * w, QImage.Format_RGB888)
                self.frame_signal.emit(qimg.copy())
                
                time.sleep(0.033)  # ~30fps
                
        except Exception as e:
            self.status_signal.emit(f"❌ USB摄像头错误: {str(e)[:50]}")
        finally:
            self.cleanup()
    
    def cleanup(self):
        if self.process:
            try:
                self.process.terminate()
                self.process.wait(timeout=1)
            except:
                try:
                    self.process.kill()
                except:
                    pass
        if self.cap:
            try:
                self.cap.release()
            except:
                pass
    
    def stop(self):
        self.running = False
        self.cleanup()
        self.wait(2000)


def detect_usb_cameras():
    """检测可用的USB摄像头设备"""
    cameras = []
    try:
        # 使用v4l2-ctl获取设备列表
        result = subprocess.run(['v4l2-ctl', '--list-devices'],
                               capture_output=True, text=True, timeout=5)
        output = result.stdout
        
        # 解析输出，查找USB摄像头
        lines = output.split('\n')
        current_device = None
        first_video_found = False
        
        for line in lines:
            # 设备名称行（不以空白开头）
            if line and not line.startswith('\t') and not line.startswith(' '):
                current_device = line.strip()
                first_video_found = False
            # video设备路径行
            elif line.strip().startswith('/dev/video') and not first_video_found:
                video_path = line.strip()
                video_num = int(video_path.replace('/dev/video', ''))
                
                # 排除非USB摄像头设备
                if current_device:
                    # 排除: CSI摄像头(rp1-cfe)、PISP后端(pispbe)、解码器(hevc)
                    skip_keywords = ['rp1-cfe', 'pispbe', 'hevc-dec']
                    if any(kw in current_device.lower() for kw in skip_keywords):
                        continue
                    
                    # 这是USB摄像头
                    device_name = current_device.split('(')[0].strip()
                    if ':' in device_name:
                        device_name = device_name.split(':')[0].strip()
                    cameras.append((video_num, f"{device_name} (video{video_num})"))
                    first_video_found = True  # 每个设备只取第一个video节点
                    
    except Exception as e:
        pass
    
    return cameras


class DeviceTestWindow(QMainWindow):
    """设备测试主窗口"""
    
    def __init__(self):
        super().__init__()
        self.audio_thread = None
        self.camera_thread = None
        self.speaker_thread = None
        self.init_ui()
        
    def init_ui(self):
        self.setWindowTitle("设备测试")
        self.setFixedSize(580, 320)  # 适配屏幕，增加宽度以容纳喇叭测试
        
        central = QWidget()
        self.setCentralWidget(central)
        layout = QHBoxLayout(central)
        
        # 左侧 - 摄像头
        camera_group = QGroupBox("📷 摄像头")
        camera_layout = QVBoxLayout(camera_group)
        
        # 摄像头选择下拉框
        camera_select_layout = QHBoxLayout()
        self.camera_combo = QComboBox()
        self.camera_combo.setFixedHeight(24)
        self.refresh_cameras()
        camera_select_layout.addWidget(self.camera_combo, stretch=1)
        
        self.refresh_btn = QPushButton("🔄")
        self.refresh_btn.setFixedSize(28, 24)
        self.refresh_btn.clicked.connect(self.refresh_cameras)
        camera_select_layout.addWidget(self.refresh_btn)
        camera_layout.addLayout(camera_select_layout)
        
        self.camera_label = QLabel("点击启动")
        self.camera_label.setFixedSize(240, 160)
        self.camera_label.setAlignment(Qt.AlignCenter)
        self.camera_label.setStyleSheet("background-color: #2d2d2d; color: white; border-radius: 4px; font-size: 11px;")
        camera_layout.addWidget(self.camera_label)
        
        self.camera_status = QLabel("未启动")
        self.camera_status.setStyleSheet("font-size: 10px;")
        camera_layout.addWidget(self.camera_status)
        
        camera_btn_layout = QHBoxLayout()
        camera_btn_layout.setSpacing(2)
        self.camera_start_btn = QPushButton("▶")
        self.camera_start_btn.setFixedSize(40, 28)
        self.camera_start_btn.clicked.connect(self.start_camera)
        self.camera_stop_btn = QPushButton("⏹")
        self.camera_stop_btn.setFixedSize(40, 28)
        self.camera_stop_btn.clicked.connect(self.stop_camera)
        self.camera_stop_btn.setEnabled(False)
        self.camera_capture_btn = QPushButton("📸")
        self.camera_capture_btn.setFixedSize(40, 28)
        self.camera_capture_btn.clicked.connect(self.capture_photo)
        self.camera_capture_btn.setEnabled(False)
        camera_btn_layout.addWidget(self.camera_start_btn)
        camera_btn_layout.addWidget(self.camera_stop_btn)
        camera_btn_layout.addWidget(self.camera_capture_btn)
        camera_btn_layout.addStretch()
        camera_layout.addLayout(camera_btn_layout)
        
        layout.addWidget(camera_group, stretch=2)
        
        # 中间 - 麦克风
        audio_group = QGroupBox("🎙 麦克风")
        audio_layout = QVBoxLayout(audio_group)
        
        self.audio_level = QProgressBar()
        self.audio_level.setOrientation(Qt.Vertical)
        self.audio_level.setMinimum(0)
        self.audio_level.setMaximum(100)
        self.audio_level.setValue(0)
        self.audio_level.setFixedSize(40, 150)
        self.audio_level.setStyleSheet("""
            QProgressBar {
                border: 2px solid #555;
                border-radius: 5px;
                background-color: #2d2d2d;
            }
            QProgressBar::chunk {
                background-color: #4CAF50;
            }
        """)
        
        level_layout = QHBoxLayout()
        level_layout.addStretch()
        level_layout.addWidget(self.audio_level)
        level_layout.addStretch()
        audio_layout.addLayout(level_layout)
        
        self.audio_value_label = QLabel("0%")
        self.audio_value_label.setAlignment(Qt.AlignCenter)
        self.audio_value_label.setStyleSheet("font-size: 11px;")
        audio_layout.addWidget(self.audio_value_label)
        
        self.audio_status = QLabel("未启动")
        self.audio_status.setStyleSheet("font-size: 10px;")
        audio_layout.addWidget(self.audio_status)
        
        audio_btn_layout = QHBoxLayout()
        audio_btn_layout.setSpacing(2)
        self.audio_start_btn = QPushButton("▶")
        self.audio_start_btn.setFixedSize(40, 28)
        self.audio_start_btn.clicked.connect(self.start_audio)
        self.audio_stop_btn = QPushButton("⏹")
        self.audio_stop_btn.setFixedSize(40, 28)
        self.audio_stop_btn.clicked.connect(self.stop_audio)
        self.audio_stop_btn.setEnabled(False)
        audio_btn_layout.addWidget(self.audio_start_btn)
        audio_btn_layout.addWidget(self.audio_stop_btn)
        audio_layout.addLayout(audio_btn_layout)
        
        layout.addWidget(audio_group, stretch=1)
        
        # 右侧 - 喇叭
        speaker_group = QGroupBox("🔊 喇叭")
        speaker_layout = QVBoxLayout(speaker_group)
        
        self.speaker_icon = QLabel("🔈")
        self.speaker_icon.setAlignment(Qt.AlignCenter)
        self.speaker_icon.setStyleSheet("font-size: 48px;")
        speaker_layout.addWidget(self.speaker_icon)
        
        self.speaker_status = QLabel("未测试")
        self.speaker_status.setAlignment(Qt.AlignCenter)
        self.speaker_status.setStyleSheet("font-size: 10px;")
        speaker_layout.addWidget(self.speaker_status)
        
        self.speaker_test_btn = QPushButton("🔊 测试")
        self.speaker_test_btn.setFixedHeight(28)
        self.speaker_test_btn.clicked.connect(self.test_speaker)
        speaker_layout.addWidget(self.speaker_test_btn)
        
        speaker_layout.addStretch()
        
        layout.addWidget(speaker_group, stretch=1)
        
        # 样式
        self.setStyleSheet("""
            QMainWindow {
                background-color: #1e1e1e;
            }
            QGroupBox {
                font-size: 12px;
                font-weight: bold;
                color: white;
                border: 1px solid #555;
                border-radius: 4px;
                margin-top: 8px;
                padding-top: 6px;
            }
            QGroupBox::title {
                subcontrol-origin: margin;
                left: 10px;
                padding: 0 5px;
            }
            QPushButton {
                background-color: #0078d4;
                color: white;
                border: none;
                padding: 4px 8px;
                border-radius: 3px;
                font-size: 12px;
            }
            QPushButton:hover {
                background-color: #1084d8;
            }
            QPushButton:disabled {
                background-color: #555;
            }
            QLabel {
                color: white;
                font-size: 10px;
            }
            QComboBox {
                background-color: #3d3d3d;
                color: white;
                border: 1px solid #555;
                border-radius: 3px;
                padding: 2px 5px;
                font-size: 10px;
            }
            QComboBox::drop-down {
                border: none;
            }
            QComboBox QAbstractItemView {
                background-color: #3d3d3d;
                color: white;
                selection-background-color: #0078d4;
            }
        """)
    
    def refresh_cameras(self):
        """刷新摄像头列表"""
        self.camera_combo.clear()
        self.camera_combo.addItem("CSI 摄像头", ("CSI", 0))
        
        usb_cameras = detect_usb_cameras()
        for idx, name in usb_cameras:
            self.camera_combo.addItem(name, ("USB", idx))
    
    def start_camera(self):
        if self.camera_thread and self.camera_thread.isRunning():
            return
        
        self.camera_status.setText("状态: 启动中...")
        self.camera_start_btn.setEnabled(False)
        self.camera_combo.setEnabled(False)
        self.refresh_btn.setEnabled(False)
        
        # 获取选中的摄像头类型
        camera_data = self.camera_combo.currentData()
        if camera_data:
            camera_type, device_index = camera_data
        else:
            camera_type, device_index = "CSI", 0
        
        self.camera_thread = CameraThread(camera_type, device_index)
        self.camera_thread.frame_signal.connect(self.update_camera_frame)
        self.camera_thread.status_signal.connect(self.update_camera_status)
        self.camera_thread.start()
        
        self.camera_stop_btn.setEnabled(True)
        self.camera_capture_btn.setEnabled(True)
    
    def stop_camera(self):
        if self.camera_thread:
            self.camera_thread.stop()
            self.camera_thread = None
        
        self.camera_label.setText("摄像头已停止")
        self.camera_status.setText("状态: 已停止")
        self.camera_start_btn.setEnabled(True)
        self.camera_stop_btn.setEnabled(False)
        self.camera_capture_btn.setEnabled(False)
        self.camera_combo.setEnabled(True)
        self.refresh_btn.setEnabled(True)
    
    def update_camera_frame(self, qimg):
        pixmap = QPixmap.fromImage(qimg)
        scaled = pixmap.scaled(self.camera_label.size(), Qt.KeepAspectRatio, Qt.SmoothTransformation)
        self.camera_label.setPixmap(scaled)
    
    def update_camera_status(self, status):
        self.camera_status.setText(f"状态: {status}")
        if "❌" in status:
            self.camera_start_btn.setEnabled(True)
            self.camera_stop_btn.setEnabled(False)
            self.camera_capture_btn.setEnabled(False)
            self.camera_combo.setEnabled(True)
            self.refresh_btn.setEnabled(True)
    
    def capture_photo(self):
        """拍照保存"""
        try:
            filename = f"/home/gary/Desktop/photo_{int(time.time())}.jpg"
            camera_data = self.camera_combo.currentData()
            camera_type = camera_data[0] if camera_data else "CSI"
            
            if camera_type == "CSI":
                subprocess.run(['rpicam-still', '-o', filename, '-t', '500', '--nopreview'],
                              capture_output=True, timeout=5)
            else:
                # USB摄像头拍照 - 从当前显示的图像保存
                pixmap = self.camera_label.pixmap()
                if pixmap:
                    pixmap.save(filename, "JPEG")
            self.camera_status.setText(f"状态: 照片已保存到桌面")
        except Exception as e:
            self.camera_status.setText(f"状态: 拍照失败")
    
    def start_audio(self):
        if self.audio_thread and self.audio_thread.isRunning():
            return
        
        self.audio_status.setText("状态: 启动中...")
        self.audio_start_btn.setEnabled(False)
        
        self.audio_thread = AudioThread()
        self.audio_thread.level_signal.connect(self.update_audio_level)
        self.audio_thread.status_signal.connect(self.update_audio_status)
        self.audio_thread.start()
        
        self.audio_stop_btn.setEnabled(True)
    
    def stop_audio(self):
        if self.audio_thread:
            self.audio_thread.stop()
            self.audio_thread = None
        
        self.audio_level.setValue(0)
        self.audio_value_label.setText("音量: 0")
        self.audio_status.setText("状态: 已停止")
        self.audio_start_btn.setEnabled(True)
        self.audio_stop_btn.setEnabled(False)
    
    def update_audio_level(self, level):
        self.audio_level.setValue(level)
        self.audio_value_label.setText(f"音量: {level}%")
    
    def update_audio_status(self, status):
        self.audio_status.setText(f"状态: {status}")
        if "❌" in status:
            self.audio_start_btn.setEnabled(True)
            self.audio_stop_btn.setEnabled(False)
    
    def test_speaker(self):
        """测试喇叭"""
        if self.speaker_thread and self.speaker_thread.isRunning():
            return
        
        self.speaker_status.setText("状态: 准备中...")
        self.speaker_test_btn.setEnabled(False)
        self.speaker_icon.setText("🔊")
        
        self.speaker_thread = SpeakerThread()
        self.speaker_thread.status_signal.connect(self.update_speaker_status)
        self.speaker_thread.finished_signal.connect(self.on_speaker_finished)
        self.speaker_thread.start()
    
    def update_speaker_status(self, status):
        self.speaker_status.setText(f"状态: {status}")
    
    def on_speaker_finished(self, success):
        self.speaker_test_btn.setEnabled(True)
        if success:
            self.speaker_icon.setText("🔊")
        else:
            self.speaker_icon.setText("🔇")
    
    def closeEvent(self, event):
        self.stop_camera()
        self.stop_audio()
        event.accept()


def main():
    # 抑制ALSA警告和Qt插件冲突
    os.environ['PYTHONWARNINGS'] = 'ignore'
    os.environ['QT_QPA_PLATFORM_PLUGIN_PATH'] = ''  # 使用系统Qt插件
    
    app = QApplication(sys.argv)
    app.setStyle('Fusion')
    
    window = DeviceTestWindow()
    window.show()
    
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
