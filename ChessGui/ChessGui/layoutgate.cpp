#include "layoutgate.h"

LayoutGate::LayoutGate(QQuickItem* parent)
	: QQuickItem(parent)
{
}

void LayoutGate::componentComplete()
{
	QQuickItem::componentComplete();
	polish(); // the next frame's polish pass (once the item is in a window)
}

void LayoutGate::updatePolish()
{
	if (_open)
		return;
	_open = true;
	emit openChanged();
}
