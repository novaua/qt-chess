#pragma once
#include <QString>

class AppConfig {
public:
	int       lastLevel = 3;
	bool      useFen = true;
	bool      playerPlaysWhite = true;

	static AppConfig load();
	void             save() const;

	static QString appDataDir();
	static void    setAppDataDir(const QString& dir); // --data-dir: use this instead of AppLocalDataLocation
	static QString configFilePath();
	static QString autoSaveFilePath(const QString& userId = QString());
	static QString savedGameFilePath(const QString& userId = QString());
};
