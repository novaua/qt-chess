#include "stdafx.h"
#include "AppConfig.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QString>

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

QString AppConfig::autoSaveFilePath(const QString& userId)
{
	if (userId.isEmpty()) return appDataDir() + "/chess.autosave";
	QString dir = appDataDir() + "/users/" + userId;
	QDir().mkpath(dir);
	return dir + "/chess.autosave";
}

QString AppConfig::savedGameFilePath(const QString& userId)
{
	if (userId.isEmpty()) return appDataDir() + "/chess.save";
	QString dir = appDataDir() + "/users/" + userId;
	QDir().mkpath(dir);
	return dir + "/chess.save";
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

	return cfg;
}

void AppConfig::save() const
{
	QJsonObject settings;
	settings["lastLevel"]        = lastLevel;
	settings["useFen"]           = useFen;
	settings["playerPlaysWhite"] = playerPlaysWhite;

	QJsonObject root;
	root["version"]  = 1;
	root["settings"] = settings;

	QFile file(configFilePath());
	if (file.open(QIODevice::WriteOnly | QIODevice::Truncate))
		file.write(QJsonDocument(root).toJson());
}
