#pragma once

#include <QObject>
#include <QTemporaryDir>
#include <memory>

class AppServices;
class QQmlEngine;

// Qt Quick Test's setup object (BL-05): the app's services on a fresh data folder, made by the
// app's own AppServices, so a test runs what main.cpp runs. One test file per process: the
// services and their profile are the file's own (scripts\Run-UiTests.ps1 runs them one by one).
class TestSetup : public QObject
{
	Q_OBJECT

public:
	TestSetup();
	~TestSetup() override;

public slots:
	void applicationAvailable();
	void qmlEngineAvailable(QQmlEngine* engine);

private:
	QTemporaryDir _dataDir;
	std::unique_ptr<AppServices> _services;
	int _engines = 0;
};
