#include "stdafx.h"
#include "AppConfig.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
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

QString AppConfig::gamesFilePath(const QString& userId)
{
	QString dir = appDataDir() + "/users/" + userId;
	QDir().mkpath(dir);
	return dir + "/games.json";
}

QString AppConfig::gamesDirPath(const QString& userId)
{
	return appDataDir() + "/users/" + userId + "/games";
}

QString AppConfig::ratingFilePath(const QString& userId)
{
	return appDataDir() + "/users/" + userId + "/rating.json";
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

	writeFileAtomically(configFilePath(), QJsonDocument(root).toJson());
}

bool AppConfig::writeFileAtomically(const QString& path, const QByteArray& data, QString* error)
{
	QDir().mkpath(QFileInfo(path).absolutePath());
	QSaveFile file(path);
	if (file.open(QIODevice::WriteOnly) && file.write(data) == data.size() && file.commit())
		return true;
	qWarning() << "can't write" << path << file.errorString();
	if (error)
		*error = file.errorString();
	return false;
}
