#include "stdafx.h"
#include "AppConfig.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QString>

static QString g_appDataDir;

QString AppConfig::appDataDir()
{
	QString dir = g_appDataDir.isEmpty()
		? QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation)
		: g_appDataDir;
	QDir().mkpath(dir);
	return dir;
}

void AppConfig::setAppDataDir(const QString& dir)
{
	g_appDataDir = dir;
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

	// QSaveFile: a crash mid-write leaves the previous file, not a broken one.
	QSaveFile file(configFilePath());
	if (!file.open(QIODevice::WriteOnly) || file.write(QJsonDocument(root).toJson()) < 0 || !file.commit())
		qWarning() << "AppConfig: can't write" << configFilePath() << file.errorString();
}
