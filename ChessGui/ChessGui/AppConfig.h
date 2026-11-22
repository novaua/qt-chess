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

	// BL-39: the compiled QML of this executable only, in a cache folder of its own:
	// <CacheLocation>/qml/<exe name>-<hash of its path>, set as QML_DISK_CACHE_PATH (unless that is set already).
	// Every build (Debug, Release, the installed app, ChessGuiTests) shares the organization and application
	// names, so without it they share one cache: one with other QML rewrites a .qmlc that a running one has
	// mapped, and that one then reads zeros (an assert in Debug, a crash in Release). Called once the
	// application object exists and before the QML engine is made. Returns the folder.
	static QString useOwnQmlCache();

	// The app's own files are written whole or not at all (QSaveFile: a crash mid-write leaves the previous
	// file, not a broken one); the folder is made. false: not written (logged; *error says why).
	static bool writeFileAtomically(const QString& path, const QByteArray& data, QString* error = nullptr);
	// The same without the flush to disk (QSaveFile's commit, ~3-6 ms a file): for many files that are
	// made safe another way, e.g. a profile import's staging folder swapped in once all are written (BL-29).
	static bool writeFileUnflushed(const QString& path, const QByteArray& data, QString* error = nullptr);
	// Tests only: while set, a write whose path contains pathPart fails ("" ends it; BL-29).
	static void failWritesForTest(const QString& pathPart);
};
