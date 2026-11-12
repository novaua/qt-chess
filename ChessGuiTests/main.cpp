#include "testsetup.h"

#include <QApplication>
#include <QByteArray>
#include <QFileInfo>
#include <QFont>
#include <QQuickStyle>
#include <QtQuickTest/quicktest.h>

// BL-05 Layer 1: the app's pages played by the tests in qml\tst_*.qml (docs/UiTestSuitePlan.md).
//   ChessGuiTests.exe -platform offscreen -input qml\tst_twoPlayers.qml [testfunction...]
// One test file per run (TestSetup); scripts\Run-UiTests.ps1 builds and runs them all.
int main(int argc, char** argv)
{
	// As the app (ChessGui's main.cpp): the style, the names its settings are kept under, fonts offscreen.
	QQuickStyle::setStyle(QStringLiteral("FluentWinUI3"));
	QCoreApplication::setOrganizationName("ChessPlusPlus_Inc");
	QCoreApplication::setOrganizationDomain("chessplusplus.app");
	QCoreApplication::setApplicationName("ChessPlusPlus");
	if (!qEnvironmentVariableIsSet("QT_QPA_FONTDIR"))
		qputenv("QT_QPA_FONTDIR", qgetenv("WINDIR") + "\\Fonts");
	// 1 test pixel = 1 QML pixel, whatever the monitor's scaling (the window sizes in the tests).
	if (!qEnvironmentVariableIsSet("QT_ENABLE_HIGHDPI_SCALING"))
		qputenv("QT_ENABLE_HIGHDPI_SCALING", "0");

	QApplication app(argc, argv); // takes Qt's own options (-platform); quick_test_main uses this one
	if (QGuiApplication::platformName() == "offscreen")
		QGuiApplication::setFont(QFont("Segoe UI", 9)); // the Windows default; offscreen picks any font

	QTEST_SET_MAIN_SOURCE_PATH
	// The tests next to this file, when no -input is given.
	const QByteArray testsDir = QFileInfo(QString::fromUtf8(__FILE__)).absolutePath().toUtf8() + "/qml";
	TestSetup setup;
	return quick_test_main_with_setup(argc, argv, "ChessGuiTests", testsDir.constData(), &setup);
}
