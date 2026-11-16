#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <memory>
#include <optional>

class UserManager;
namespace Chess { class Game; }

// One local game of the active profile, as the games library keeps it (BL-19): who, when, the moves
// in UCI, the result, the clock as it was left. Every game vs the robot or between two players is kept
// from its first move, finished or left, so Continue, the Games page, the rating and the analysis all
// read from here. The player's own name isn't stored: it is the profile's (renamed with it).
struct GameRecord
{
	QString   id;                 // a uuid
	QString   kind;               // "computer" (vs the robot) or "local" (two players on this computer)
	bool      finished = false;   // false: Continue offers it
	QDateTime startedAt;
	QDateTime finishedAt;         // when the result came (finished only)
	bool      playerPlaysWhite = true;
	QString   opponentName;       // "Robot", a profile's name or "Guest"
	QString   opponentUserId;     // the opponent's profile in a two-player game; "" = Guest
	int       level = 0;          // the robot's level (computer only)
	QString   playerAvatar;       // AvatarProvider raw names, as the save info kept them
	QString   opponentAvatar;
	int       clockLimitSec = 0;  // the time control; 0 and 0: no clock
	int       clockIncSec = 0;
	qint64    whiteMs = 0;        // the times left when the game was left (unfinished with a clock)
	qint64    blackMs = 0;
	QStringList moves;            // UCI, e.g. "e2e4", "e7e8q"
	QString   fen;                // the position after the last move: the card's board preview
	QString   result;             // "1-0", "0-1", "1/2-1/2"; "" while unfinished
	QString   resultText;         // e.g. "Alice wins by checkmate"
	bool      resultRecorded = false; // the statistics have this game (a mate taken back: not again)
	int       undoCount = 0;      // takebacks (a game with any isn't rated)
	QString   forkOfId;           // Play from here: the game this one continues from, and after how many plies
	int       forkPly = 0;
	QJsonObject analysis;         // the engine's analysis, once made (step 6); empty until then

	int  plyCount() const { return (int)moves.size(); }
	bool whiteToMove() const { return moves.size() % 2 == 0; }
	bool playerToMove() const { return whiteToMove() == playerPlaysWhite; }
	QString lastMove() const { return moves.isEmpty() ? QString() : moves.last(); }
	// The moves and the position after them (fen), played from the start position; false (and nothing
	// changed) when one of them can't be played. For records made outside a game: the screenshot
	// scenes, the UI tests.
	bool setMoves(const QStringList& uciMoves);
	// The moves played from the start position on a game of their own (a position to look at: its SAN,
	// its FEN, whether it is over); null when one of them can't be played.
	static std::shared_ptr<Chess::Game> play(const QStringList& uciMoves);
	// The moves played on `game` as it stands; false at the first one that can't be played (a move
	// from an empty square too, which the game itself only asserts), the ones before it stay played.
	static bool playOn(Chess::Game& game, const QStringList& uciMoves);

	QJsonObject toJson() const;
	static GameRecord fromJson(const QJsonObject& o);
	QVariantMap toVariant() const; // for QML: the fields above plus plyCount, playerToMove, lastMove
};

// The games library of the active profile (BL-19): `users/<id>/games.json`, written whole with
// QSaveFile on every change (a record is small; a crash loses nothing). It follows the active profile
// (UserManager::activeUserChanged) and keeps the last `keepCount` finished games: the oldest finished
// one goes when a new one comes; unfinished games are never removed on their own. On its first load
// for a profile it migrates the two save slots of earlier versions (`chess.autosave`, `chess.save`).
//
// QML singleton `GameLibrary` (import ChessPlusPlus): the list for the Games page and the Continue
// list, and Delete; anything that puts a game on the board goes through ChessConnector (openGame,
// continueGame), as the Key Conventions say.
class GameLibrary : public QObject
{
	Q_OBJECT
	// Every kept game, newest first (GameRecord::toVariant).
	Q_PROPERTY(QVariantList games READ games NOTIFY gamesChanged)
	// The unfinished ones, newest first: the Continue list's local cards (with the Lichess ones).
	Q_PROPERTY(QVariantList unfinishedGames READ unfinishedGames NOTIFY gamesChanged)
	Q_PROPERTY(int count READ count NOTIFY gamesChanged)
	Q_PROPERTY(int unfinishedCount READ unfinishedCount NOTIFY gamesChanged)
	// The finished ones: those beyond keepCount go (Settings asks before a smaller keepCount removes some).
	Q_PROPERTY(int finishedCount READ finishedCount NOTIFY gamesChanged)
	// How many finished games are kept per profile (Settings → Data; AppSettings.gamesKept binds it).
	Q_PROPERTY(int keepCount READ keepCount WRITE setKeepCount NOTIFY keepCountChanged)

public:
	static constexpr int DefaultKeepCount = 100;
	static constexpr int FormatVersion = 1;

	explicit GameLibrary(QObject* parent = nullptr);
	~GameLibrary() override; // a change still waiting is written

	void setUserManager(UserManager* um); // follows the active profile from now on (and loads its games)

	QVariantList games() const;
	QVariantList unfinishedGames() const;
	int count() const { return (int)_records.size(); }
	int unfinishedCount() const;
	int finishedCount() const { return count() - unfinishedCount(); }
	int keepCount() const { return _keepCount; }
	void setKeepCount(int count); // every profile's games: the finished ones beyond it go, now
	// How many finished games a keepCount of `keep` would delete, of every profile (Settings asks first).
	Q_INVOKABLE int finishedBeyond(int keep) const;

	// The records, newest first.
	const QList<GameRecord>& records() const { return _records; }
	QList<GameRecord> unfinished() const;
	std::optional<GameRecord> find(const QString& id) const;

	// Adds the record (an empty id gets one) or replaces the one with its id; trims the finished games
	// to keepCount. Returns the id. Nothing is kept without an active profile. The file is written once
	// the current event is done (a move and the result it brings: one write); flush() writes it now.
	QString put(GameRecord record);
	void flush();
	Q_INVOKABLE bool remove(const QString& id);
	Q_INVOKABLE QVariantMap game(const QString& id) const; // {} when there is none

	// The profile's file. Public for the tests.
	QString filePath() const;
	static QString filePathOf(const QString& userId);

signals:
	void gamesChanged();
	void keepCountChanged();

private:
	enum class FileState { Missing, Read, Damaged, Unreadable };
	// A games.json: its records, newest first. Unreadable: it is there but couldn't be opened (a lock).
	static FileState readFile(const QString& path, QList<GameRecord>& records);
	static bool writeFile(const QString& path, const QList<GameRecord>& records);
	static bool trimRecords(QList<GameRecord>& records, int keep); // true: some went
	QStringList profileIds() const;

	void reload();
	bool save();
	void scheduleSave();
	void trim() { trimRecords(_records, _keepCount); }
	void trimOtherProfiles(); // their files, as setKeepCount trims the active one's
	void migrateOldSaves(); // chess.autosave / chess.save + their GameSaveInfo → records (once)

	UserManager* _userManager = nullptr;
	QString _userId;           // the profile whose games these are; "" = none (nothing is kept)
	QList<GameRecord> _records; // newest first
	int _keepCount = DefaultKeepCount;
	bool _unreadable = false;  // its games.json couldn't be read: never written over (save() reads it again)
	bool _dirty = false;       // a change not written yet
	bool _saveQueued = false;  // scheduleSave(): the write is on its way
};
