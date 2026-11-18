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
	// BL-19: the profile's games library of versions before BL-20, users/<id>/games.json (migrated once).
	static QString gamesFilePath(const QString& userId);
	// BL-20: the profile's games, one signed file each (users/<id>/games/<gameId>.json), and its rating
	// vs the robot (users/<id>/rating.json). The folder isn't made here.
	static QString gamesDirPath(const QString& userId);
	static QString ratingFilePath(const QString& userId);
};
