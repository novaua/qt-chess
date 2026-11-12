#include "sceneinput.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QPointer>
#include <QPointingDevice>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTimer>

SceneInput::SceneInput(QObject* parent)
	: QObject(parent)
{
	_clock.start();
}

void SceneInput::click(QQuickItem* item, QPointF point)
{
	if (!item || !item->window())
		return;
	send(item->window(), QEvent::MouseButtonPress, point, true);
	send(item->window(), QEvent::MouseButtonRelease, point, false);
}

void SceneInput::drag(QQuickItem* item, QPointF from, QPointF to, int ms)
{
	if (!item || !item->window())
		return;
	QPointer<QQuickWindow> window = item->window();
	send(window, QEvent::MouseButtonPress, from, true);
	const int steps = qMax(1, ms / 16);
	for (int i = 1; i <= steps; ++i) {
		const QPointF at = from + (to - from) * (qreal(i) / steps);
		QTimer::singleShot(i * 16, this, [=, this]() {
			if (window)
				send(window, QEvent::MouseMove, at, true);
		});
	}
	QTimer::singleShot(steps * 16 + 16, this, [=, this]() {
		if (window)
			send(window, QEvent::MouseButtonRelease, to, false);
	});
}

void SceneInput::send(QQuickWindow* window, int type, QPointF point, bool buttonDown)
{
	const auto button = type == QEvent::MouseMove ? Qt::NoButton : Qt::LeftButton;
	const auto buttons = buttonDown ? Qt::MouseButtons(Qt::LeftButton) : Qt::MouseButtons(Qt::NoButton);
	QMouseEvent event(QEvent::Type(type), point, point, window->mapToGlobal(point), button, buttons,
	                  Qt::NoModifier, QPointingDevice::primaryPointingDevice());
	event.setTimestamp(quint64(_clock.elapsed()));
	QCoreApplication::sendEvent(window, &event);
}
