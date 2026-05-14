#include "stdafx.h"
#include "AppConfig.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>

QString AppConfig::appDataDir()
{
	QString dir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
	QDir().mkpath(dir);
	return dir;
}

QString AppConfig::configFilePath()
{
	return appDataDir() + "/config.json";
}

QString AppConfig::autoSaveFilePath()
{
	return appDataDir() + "/chess.autosave";
}

QString AppConfig::savedGameFilePath()
{
	return appDataDir() + "/chess.save";
}

AppConfig AppConfig::load()
{
	AppConfig cfg;
	QFile file(configFilePath());
	if (!file.open(QIODevice::ReadOnly))
		return cfg;

	QJsonParseError err;
	QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
	if (err.error != QJsonParseError::NoError || !doc.isObject())
		return cfg;

	QJsonObject root = doc.object();

	if (root.contains("settings") && root["settings"].isObject()) {
		QJsonObject s = root["settings"].toObject();
		cfg.lastLevel        = s.value("lastLevel").toInt(cfg.lastLevel);
		cfg.useFen           = s.value("useFen").toBool(true);
		cfg.playerPlaysWhite = s.value("playerPlaysWhite").toBool(true);
	}

	if (root.contains("statistics") && root["statistics"].isObject()) {
		QJsonObject st = root["statistics"].toObject();
		cfg.stats.gamesPlayed = st.value("gamesPlayed").toInt(0);
		cfg.stats.humanWins = st.value("humanWins").toInt(0);
		cfg.stats.computerWins = st.value("computerWins").toInt(0);
		cfg.createdDate = QDate::fromString(st.value("createdDate").toString(), Qt::ISODate);
	}
	if (!cfg.createdDate.isValid())
		cfg.createdDate = QDate::currentDate();

	if (root.contains("autoSave") && root["autoSave"].isObject()) {
		QJsonObject a = root["autoSave"].toObject();
		cfg.autoSaveIsSinglePlayer    = a.value("isSinglePlayer").toBool(false);
		cfg.autoSavePlayerPlaysWhite  = a.value("playerPlaysWhite").toBool(true);
		cfg.autoSavePlayerAvatarName   = a.value("playerAvatarName").toString("wizard");
		cfg.autoSaveOpponentAvatarName = a.value("opponentAvatarName").toString("unicorn");
	}

	if (root.contains("savedGame") && root["savedGame"].isObject()) {
		QJsonObject sg = root["savedGame"].toObject();
		cfg.savedGameIsSinglePlayer    = sg.value("isSinglePlayer").toBool(false);
		cfg.savedGamePlayerPlaysWhite  = sg.value("playerPlaysWhite").toBool(true);
		cfg.savedGamePlayerAvatarName  = sg.value("playerAvatarName").toString("wizard");
		cfg.savedGameOpponentAvatarName = sg.value("opponentAvatarName").toString("unicorn");
	}

	return cfg;
}

void AppConfig::save() const
{
	QJsonObject settings;
	settings["lastLevel"]        = lastLevel;
	settings["useFen"]           = useFen;
	settings["playerPlaysWhite"] = playerPlaysWhite;

	QJsonObject statistics;
	statistics["gamesPlayed"] = stats.gamesPlayed;
	statistics["humanWins"]   = stats.humanWins;
	statistics["computerWins"] = stats.computerWins;
	statistics["createdDate"] = createdDate.toString(Qt::ISODate);

	QJsonObject autoSave;
	autoSave["isSinglePlayer"]   = autoSaveIsSinglePlayer;
	autoSave["playerPlaysWhite"] = autoSavePlayerPlaysWhite;
	autoSave["playerAvatarName"]   = autoSavePlayerAvatarName;
	autoSave["opponentAvatarName"] = autoSaveOpponentAvatarName;

	QJsonObject savedGame;
	savedGame["isSinglePlayer"]    = savedGameIsSinglePlayer;
	savedGame["playerPlaysWhite"]  = savedGamePlayerPlaysWhite;
	savedGame["playerAvatarName"]  = savedGamePlayerAvatarName;
	savedGame["opponentAvatarName"] = savedGameOpponentAvatarName;

	QJsonObject root;
	root["version"]   = 1;
	root["settings"]  = settings;
	root["statistics"] = statistics;
	root["autoSave"]  = autoSave;
	root["savedGame"] = savedGame;

	QFile file(configFilePath());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		file.write(QJsonDocument(root).toJson());
}
