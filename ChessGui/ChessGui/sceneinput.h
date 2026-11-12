#pragma once
#include <QElapsedTimer>
#include <QObject>
#include <QPointF>

class QQuickItem;
class QQuickWindow;

// Mouse input for the screenshot scenes (BL-11's "play" scene): clicks and drags sent to the window as
// mouse events, so they go the way a user's do, through the board's TapHandler and DragHandler.
// Points are in the window's (scene) coordinates; `item` is any item in that window. Only in the
// screenshot mode (main.cpp registers it as the QML singleton SceneInput).
class SceneInput : public QObject
{
	Q_OBJECT

public:
	explicit SceneInput(QObject* parent = nullptr);

	// A press, and the release 50 ms later, at the point.
	Q_INVOKABLE void click(QQuickItem* item, QPointF point);
	// A press at `from`, a move every frame (16 ms) to `to` over `ms`, then the release there.
	Q_INVOKABLE void drag(QQuickItem* item, QPointF from, QPointF to, int ms);

private:
	void send(QQuickWindow* window, int type, QPointF point, bool buttonDown);

	QElapsedTimer _clock; // the events' timestamps
};
