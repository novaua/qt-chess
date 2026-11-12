#pragma once
#include <QQuickItem>

// Opens when its window prepares the first frame after it was made (BL-11): in the polish pass, before
// the frame is drawn, once whatever built the page has finished and its layout has settled. QML makes
// costly items behind it (`Loader { active: gate.open }`), so they are made once, at their real size,
// and are still in that first frame: items made during the polish pass are polished in the same pass.
// While a page is built, its bindings pass through stand-in sizes (a binding can read another before
// that one is updated); items made then are laid out again for each of them.
class LayoutGate : public QQuickItem
{
	Q_OBJECT
	Q_PROPERTY(bool open READ isOpen NOTIFY openChanged)

public:
	explicit LayoutGate(QQuickItem* parent = nullptr);

	bool isOpen() const { return _open; }

signals:
	void openChanged();

protected:
	void componentComplete() override;
	void updatePolish() override;

private:
	bool _open = false;
};
