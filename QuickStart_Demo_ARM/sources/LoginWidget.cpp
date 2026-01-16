#include "LoginWidget.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QMessageBox>

/**
 * VolcEngineRTC 音视频通话入口页面
 *
 * 包含如下简单功能：
 * - 该页面用来跳转至音视频通话主页面
 * - 校验房间名和用户名
 * - 展示当前 SDK 使用的版本号 {@link RTCEngine#getSdkVersion()}
 *
 * 有以下常见的注意事项：
 * 1.SDK 对房间名、用户名的限制是：非空且最大长度不超过128位的数字、大小写字母、@ . _ \ -
 */

LoginWidget::LoginWidget(QWidget *parent)
    : QWidget(parent)
{
    ui.setupUi(this);
	setAttribute(Qt::WA_StyledBackground);
	parent->installEventFilter(this);

	connect(this, SIGNAL(sigEnterRoom(const QString &, const QString &)), parent, SLOT(slotOnEnterRoom(const QString &, const QString &)));
	
	// 默认显示加载中状态
	ui.enterRoomBtn->setText(QStringLiteral(u"正在连接 Server..."));
	ui.enterRoomBtn->setEnabled(false);
	ui.roomIDLineEdit->setEnabled(false);
	ui.userIDLineEdit->setEnabled(false);
}

bool LoginWidget::eventFilter(QObject *watched, QEvent *event) 
{
	if (watched == parent())
	{
		auto parentWindow = dynamic_cast<QWidget*>(parent());
		if (parentWindow == nullptr) 
		{
			return false;
		}
		if (event->type() == QEvent::Resize) 
		{	
			//update login geometry
			auto selfRect = this->rect();
			auto parentGem = parentWindow->rect();
			selfRect.moveCenter(parentGem.center());
			setGeometry(selfRect);
		}
	}
	return false;
}

void LoginWidget::on_enterRoomBtn_clicked() 
{
	// 使用服务器配置时，直接使用配置的值
	if (m_useServerConfig) {
		emit sigEnterRoom(m_serverRoomId, m_serverUserId);
		return;
	}
	
	auto checkStr = [=](const QString &typeName, const QString &str)->bool
	{
		if (str.isEmpty()) 
		{
			QMessageBox::warning(this, QStringLiteral(u"提示"), typeName+ QStringLiteral(u"不能为空！"),QStringLiteral(u"确定"));
			return false;
		}

		if (str.size() > 128) 
		{
			QMessageBox::warning(this, QStringLiteral(u"提示"), typeName + QStringLiteral(u"不能超过128个字符！"), QStringLiteral(u"确定"));
			return false;
		}
	
		for (int i = 0; i < str.size(); i++) 
		{
			if (isalpha(str[i].cell())
				|| isdigit(str[i].cell())
				|| str[i] == '@'
				|| str[i] == '.'
				|| str[i] == '_'
				|| str[i] == '-'
				|| str[i] == '\\'
				) 
			{
				continue;
			}
			else 
			{
				QMessageBox::warning(this, QStringLiteral(u"输入不合法"), 
					QStringLiteral(u"只支持数字、大小写字母、@._-"),
					QStringLiteral(u"确定"));
				return false;
			}
		}
		return true;
	};

	if (!checkStr(QStringLiteral(u"房间号"),ui.roomIDLineEdit->text()))
	{
		return;
	}

	if (!checkStr(QStringLiteral(u"用户ID"), ui.userIDLineEdit->text()))
	{
		return;
	}

	emit sigEnterRoom(ui.roomIDLineEdit->text(),ui.userIDLineEdit->text());
}

void LoginWidget::setServerConfig(const QString& roomId, const QString& userId, const QString& sceneName)
{
	m_useServerConfig = true;
	m_serverRoomId = roomId;
	m_serverUserId = userId;
	
	// 更新 UI 显示
	ui.roomIDLineEdit->setText(roomId);
	ui.userIDLineEdit->setText(userId);
	ui.roomIDLineEdit->setEnabled(false);  // 禁用编辑，使用服务器配置
	ui.userIDLineEdit->setEnabled(false);
	
	ui.enterRoomBtn->setText(QStringLiteral(u"🤖 进入房间 - ") + sceneName);
	ui.enterRoomBtn->setEnabled(true);
}

void LoginWidget::setConfigLoading(bool loading)
{
	if (loading) {
		ui.enterRoomBtn->setText(QStringLiteral(u"正在连接 Server..."));
		ui.enterRoomBtn->setEnabled(false);
	}
}

void LoginWidget::setConfigError(const QString& error)
{
	m_useServerConfig = false;
	ui.roomIDLineEdit->setEnabled(true);
	ui.userIDLineEdit->setEnabled(true);
	ui.enterRoomBtn->setText(QStringLiteral(u"进入房间 (手动模式)"));
	ui.enterRoomBtn->setEnabled(true);
}