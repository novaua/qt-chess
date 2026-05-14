#pragma once
#include <QDate>
#include <QString>

struct GameStats {
	int gamesPlayed = 0;
	int humanWins = 0;
	int computerWins = 0;
};

class AppConfig {
public:
	int       lastLevel = 3;
	bool      useFen = true;
	GameStats stats;
	bool      autoSaveIsSinglePlayer    = false;
	bool      autoSavePlayerPlaysWhite  = true;
	QString   autoSavePlayerAvatarName   = "wizard";
	QString   autoSaveOpponentAvatarName = "unicorn";
	bool      savedGameIsSinglePlayer    = false;
	bool      savedGamePlayerPlaysWhite  = true;
	QString   savedGamePlayerAvatarName  = "wizard";
	QString   savedGameOpponentAvatarName = "unicorn";
	bool      playerPlaysWhite           = true;
	QDate     createdDate;

	static AppConfig load();
	void             save() const;

	static QString appDataDir();
	static QString configFilePath();
	static QString autoSaveFilePath();
	static QString savedGameFilePath();
};
