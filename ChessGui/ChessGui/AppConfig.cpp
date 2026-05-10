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
		cfg.lastLevel = s.value("lastLevel").toInt(cfg.lastLevel);
	}

	if (root.contains("statistics") && root["statistics"].isObject()) {
		QJsonObject st = root["statistics"].toObject();
		cfg.stats.gamesPlayed = st.value("gamesPlayed").toInt(0);
		cfg.stats.humanWins = st.value("humanWins").toInt(0);
		cfg.stats.computerWins = st.value("computerWins").toInt(0);
	}

	if (root.contains("autoSave") && root["autoSave"].isObject()) {
		QJsonObject a = root["autoSave"].toObject();
		cfg.autoSaveIsSinglePlayer = a.value("isSinglePlayer").toBool(false);
	}

	return cfg;
}

void AppConfig::save() const
{
	QJsonObject settings;
	settings["lastLevel"] = lastLevel;

	QJsonObject statistics;
	statistics["gamesPlayed"] = stats.gamesPlayed;
	statistics["humanWins"] = stats.humanWins;
	statistics["computerWins"] = stats.computerWins;

	QJsonObject autoSave;
	autoSave["isSinglePlayer"] = autoSaveIsSinglePlayer;

	QJsonObject root;
	root["version"] = 1;
	root["settings"] = settings;
	root["statistics"] = statistics;
	root["autoSave"] = autoSave;

	QFile file(configFilePath());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		file.write(QJsonDocument(root).toJson());
}
