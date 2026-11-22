#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QList>
#include <QMap>
#include <QObject>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <algorithm>
#include <memory>
#include <optional>
#include "LichessClient.h"
#include "StartPosition.h"

class GameAnalysis;
class UserManager;
namespace Chess { class Game; }

// The player's rating vs the robot (BL-19 step 5), standard Elo: it starts at 1200, a rated game moves it
// by K·(S − E) with E = 1/(1 + 10^((robot − rating)/400)), K = 40 for the first 30 rated games, 20 after;
// provisional under 10. Stored as it goes (the owner's choice, 2026-11-16): each rated game's change is
// worked out when it ends and kept on its record, the rating itself in rating.json (signed, BL-20), so
// removing games (the cap, Delete) never changes it. The robot's rating is its level's EngineLevel::DisplayElo().
class LocalRating
{
public:
	static constexpr int Start = 1200;
	static constexpr int ProvisionalGames = 10; // provisional under this many rated games
	static constexpr int SettledGames = 30;     // K 40 before, 20 from then on

	LocalRating() = default;
	LocalRating(int value, int games) : _value(value), _games(std::max(0, games)) {}

	int  value() const { return _value; }
	int  games() const { return _games; }
	bool provisional() const { return _games < ProvisionalGames; }
	int  kFactor() const { return _games < SettledGames ? 40 : 20; }
	// The change a game against `opponentElo` brings: score 1 a win, 0.5 a draw, 0 a loss (rounded).
	int  changeFor(int opponentElo, double score) const;
	void apply(int change)  { _value += change; ++_games; }
	void revert(int change) { _value -= change; _games = std::max(0, _games - 1); }

	QJsonObject toJson() const;
	static LocalRating fromJson(const QJsonObject& o); // the start rating when there is none

	bool operator==(const LocalRating&) const = default;

private:
	int _value = Start;
	int _games = 0;
};

// LI-14: what a kept Lichess game knows beyond a local record: its id on lichess.org, whether it was rated
// there, its speed and days per move, and both players as its gameFull gave them, with the ratings from
// before the game (the change isn't in the Board API's stream: the profile's Lichess ratings catch up at the
// next account sync). A record of kind "lichess" has one, as its JSON's "lichess" object.
class LichessGameInfo
{
public:
	LichessGameInfo() = default;
	LichessGameInfo(const QString& gameId, bool rated, const QString& speed, int daysPerTurn,
	                const LichessPlayer& white, const LichessPlayer& black);

	QString gameId() const { return _gameId; }
	bool    isEmpty() const { return _gameId.isEmpty(); }
	bool    rated() const { return _rated; }
	QString speed() const { return _speed; }             // "rapid", "classical", "correspondence", …; "" unknown
	int     daysPerTurn() const { return _daysPerTurn; } // a correspondence game's; 0 otherwise
	const LichessPlayer& player(bool white) const { return white ? _white : _black; }
	QString url() const;                                 // https://lichess.org/<id>

	QJsonObject toJson() const;
	static LichessGameInfo fromJson(const QJsonObject& o); // empty when there is none

private:
	QString _gameId;
	bool    _rated = false;
	QString _speed;
	int     _daysPerTurn = 0;
	LichessPlayer _white;
	LichessPlayer _black;
};

// One game of the active profile, as the games library keeps it (BL-19): who, when, the moves in UCI, the
// result, the clock as it was left. Every game vs the robot or between two players is kept from its first
// move, finished or left, so Continue, the Games page, the rating and the analysis all read from here; a
// Lichess game only once it has finished here (LI-14). The player's own name isn't stored: it is the
// profile's (renamed with it).
struct GameRecord
{
	QString   id;                 // a uuid; a Lichess game's: lichessRecordId(its Lichess id)
	// "computer" (vs the robot), "local" (two players on this computer) or "lichess" (LI-14: a finished
	// Lichess game, never rated, continued or Played from here).
	QString   kind;
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
	QString   startFen;           // a Chess960 game's start position (LI-01A), as games.json has it; "" = the standard game
	// startFen read once (setStartPosition, fromJson). nullopt: a Chess960 record whose startFen can't be
	// read: it can't be replayed (continue / open refuse it), and is written back as it was read.
	std::optional<Chess::StartPosition> start = Chess::StartPosition();
	QStringList moves;            // UCI, e.g. "e2e4", "e7e8q"; a Chess960 castling is the king onto its rook
	QString   fen;                // the position after the last move: the card's board preview
	QString   result;             // "1-0", "0-1", "1/2-1/2"; "" while unfinished
	QString   resultText;         // e.g. "Alice wins by checkmate"
	bool      resultRecorded = false; // the statistics have this game (a mate taken back: not again)
	int       undoCount = 0;      // takebacks (a game with any isn't rated)
	QString   forkOfId;           // Play from here: the game this one continues from, and after how many plies
	int       forkPly = 0;
	QJsonObject analysis;         // the engine's analysis (GameAnalysis::toJson, step 6); empty until made, dropped when the moves change
	bool      rated = false;      // its result moved the player's rating (step 5): by ratingChange, from ratingBefore
	int       ratingBefore = 0;
	int       ratingChange = 0;
	// BL-20: the player's rated games after this one, so the newest rated game gives the whole rating
	// (ratingBefore + ratingChange, ratedGames) when rating.json fails its check; 0 in records rated before.
	int       ratedGames = 0;
	LichessGameInfo lichess;      // a Lichess game's own facts (kind "lichess"); empty otherwise

	// A Lichess game's record id: "lichess-<gameId>", a file name (isFileId), so a game is never kept twice.
	static QString lichessRecordId(const QString& lichessGameId) { return QStringLiteral("lichess-") + lichessGameId; }
	bool isLichess() const { return kind == QLatin1String("lichess"); }
	bool chess960() const { return !startFen.isEmpty() || !start; }
	// The standard one unless startFen is a Chess960 start (also for one that can't be read: replay() fails then).
	Chess::StartPosition startPosition() const { return start.value_or(Chess::StartPosition()); }
	void setStartPosition(const Chess::StartPosition& start);
	int  plyCount() const { return (int)moves.size(); }
	bool whiteToMove() const { return moves.size() % 2 == 0; }
	bool playerToMove() const { return whiteToMove() == playerPlaysWhite; }
	QString lastMove() const { return moves.isEmpty() ? QString() : moves.last(); }
	// Its result can be rated (step 5): vs the robot, finished, no takeback, not begun from another game,
	// and both sides moved (as lichess.org: a game resigned or left before that is aborted, not rated).
	// Never a Chess960 game (the owner's choice, LI-01A).
	bool rateable() const;
	double playerScore() const; // 1 the player won, 0.5 a draw, 0 lost
	// The moves and the position after them (fen), played from its start position; false (and nothing
	// changed) when one of them can't be played. For records made outside a game: the screenshot
	// scenes, the UI tests.
	bool setMoves(const QStringList& uciMoves);
	// Its first `plies` moves (all: -1) played from its start position: play(moves, startPosition()).
	std::shared_ptr<Chess::Game> replay(int plies = -1) const;
	// The moves played from `start` on a game of their own (a position to look at: its SAN, its FEN,
	// whether it is over); null when one of them can't be played.
	static std::shared_ptr<Chess::Game> play(const QStringList& uciMoves,
	                                         const Chess::StartPosition& start = Chess::StartPosition());
	// The moves played on `game` as it stands; false at the first one that can't be played (a move
	// from an empty square too, which the game itself only asserts), the ones before it stay played.
	static bool playOn(Chess::Game& game, const QStringList& uciMoves);

	QJsonObject toJson() const;
	static GameRecord fromJson(const QJsonObject& o);
	// For QML: the fields above plus plyCount, playerToMove, lastMove; a Lichess game's lichessUrl, lichessRated,
	// lichessSpeed, lichessDays, opponentTitle and opponentRating ("1523?").
	QVariantMap toVariant() const;
};

// The games library of the active profile (BL-19). BL-20: one signed file per game,
// `users/<id>/games/<gameId>.json` ({"record": …, "sig": GameSignature}), written with QSaveFile when it
// changes (only that game's file), and the rating vs the robot in `users/<id>/rating.json` (signed too;
// one that fails its check is set aside and the rating restored from the newest rated game). A game file
// that fails its check (edited by hand, damaged, another game's id) is set aside in `games/rejected/`,
// never loaded or counted (gamesSetAside). It follows the active profile (UserManager::activeUserChanged)
// and keeps the last `keepCount` finished games: the oldest finished one goes when a new one comes;
// unfinished games are never removed on their own. It migrates once what earlier versions kept: the
// profile's `games.json` (BL-19: trusted and signed, then renamed `games.json.migrated`) and the two save
// slots before it (`chess.autosave`, `chess.save`).
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
	// The player's rating vs the robot (LocalRating): Home's header, the Profile page, PGN's Elo.
	Q_PROPERTY(int rating READ rating NOTIFY ratingChanged)
	Q_PROPERTY(int ratedGames READ ratedGames NOTIFY ratingChanged)
	Q_PROPERTY(bool ratingProvisional READ ratingProvisional NOTIFY ratingChanged)
	// The rating is provisional under this many rated games (LocalRating::ProvisionalGames).
	Q_PROPERTY(int provisionalGames READ provisionalGames CONSTANT)
	// BL-20: the games of this profile set aside when it was loaded (they failed their check).
	Q_PROPERTY(int setAsideCount READ setAsideCount NOTIFY gamesChanged)
	// BL-20: this profile's rating.json failed its check (or was missing with rated games kept) when it was
	// loaded: the rating was restored from the newest rated game, or started again (ratedGames 0).
	Q_PROPERTY(bool ratingRestored READ ratingRestored NOTIFY ratingChanged)

public:
	static constexpr int DefaultKeepCount = 100;
	static constexpr int FormatVersion = 1;       // games.json (BL-19): read only to migrate it
	static constexpr int RatingFormatVersion = 1; // rating.json

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

	const LocalRating& localRating() const { return _rating; }
	int  rating() const { return _rating.value(); }
	int  ratedGames() const { return _rating.games(); }
	bool ratingProvisional() const { return _rating.provisional(); }
	static int provisionalGames() { return LocalRating::ProvisionalGames; }

	// The records, newest first.
	const QList<GameRecord>& records() const { return _records; }
	QList<GameRecord> unfinished() const;
	std::optional<GameRecord> find(const QString& id) const;

	// Adds the record (an empty id gets one) or replaces the one with its id; trims the finished games
	// to keepCount. A rateable record that just finished is rated (its rated fields set, the rating
	// moved); a rated one that isn't finished any more (a takeback resumed it) gives its change back.
	// An analysis made for other moves (GameAnalysis::fits) is dropped.
	// Returns the id. Nothing is kept without an active profile. The file is written once
	// the current event is done (a move and the result it brings: one write); flush() writes it now.
	QString put(GameRecord record);
	void flush();
	Q_INVOKABLE bool remove(const QString& id);
	// The engine's analysis of a kept game (GameAnalyzer), made for these moves: false (nothing kept)
	// when there is no such game or its moves aren't these any more.
	bool setAnalysis(const QString& id, const QStringList& moves, const GameAnalysis& analysis);
	QString userId() const { return _userId; } // the profile whose games these are; "" = none
	Q_INVOKABLE QVariantMap game(const QString& id) const; // {} when there is none
	int setAsideCount() const { return _setAside; }
	bool ratingRestored() const { return _ratingRestored; }

	// The profile's files. Public for the tests.
	QString gamesDir() const { return gamesDirOf(_userId); }
	static QString gamesDirOf(const QString& userId);
	static QString gameFilePathOf(const QString& userId, const QString& gameId);
	static QString rejectedDirOf(const QString& userId);
	static QString ratingFilePathOf(const QString& userId);
	static QString legacyFilePathOf(const QString& userId); // games.json of BL-19
	// The profile has files of BL-20 (rating.json, a game file, games.json.migrated, games/rejected/): a
	// games.json beside them isn't migrated (it is set aside), and a users.json older than version 3 isn't
	// trusted (UserManager). Either would be a way around the check.
	static bool hasSignedFiles(const QString& userId);
	// A game file's content: {"record": …, "sig": …}, signed with the current key.
	static QByteArray gameFileContent(const GameRecord& record);
	// Why a game file failed its check (readGameFile).
	enum class ReadFailure { None, CantOpen, NotJson, BadSignature, OtherId };
	static QString describe(ReadFailure why); // for the log
	// A game file read and checked: its record, or nullopt (why: the reason). resign: it verified with an
	// older key (to be signed again).
	static std::optional<GameRecord> readGameFile(const QString& path, ReadFailure* why = nullptr, bool* resign = nullptr);
	// The same for a game file's content, named `fileId` (its file name without ".json").
	static std::optional<GameRecord> readGameContent(const QByteArray& content, const QString& fileId,
	                                                 ReadFailure* why = nullptr, bool* resign = nullptr);

	// BL-20 step 3: what checkImport found in an imported profile's files.
	struct ImportCheck
	{
		int  setAside = 0;          // game files that failed their check: put in games/rejected/
		bool unrated = false;       // an export from before BL-20: its games.json came in unrated
		bool ratingRestored = false; // its rating.json failed its check: restored from its games (or none)
	};
	// An imported profile's files (the name under users/<id>/ → content, as the export has them), checked
	// before UserManager writes them: a game file that fails its check goes to games/rejected/; games.json
	// (an export from before BL-20) becomes a signed file per game without its rating fields, and no
	// rating comes with it (the owner's choice: an old export imports unrated); a rating.json that fails
	// its check goes to games/rejected/ and the rating is restored from the newest verified rated game.
	static ImportCheck checkImport(QMap<QString, QByteArray>& files);
	// A game id that can name a file: letters, digits, '-' and '_' (a UUID).
	static bool isFileId(const QString& id);
	// The file id of a games.json record: its id when it can name a file, else a UUID made from it (always
	// the same one: a migration tried again finds the files it already wrote).
	static QString fileIdOf(const QString& id);
	// rating.json's content: {"version", "rating", "sig"}, the rating signed with the current key.
	static QByteArray ratingFileContent(const LocalRating& rating);
	// rating.json read and checked: nullopt when it isn't there, isn't JSON or its signature doesn't fit.
	static std::optional<LocalRating> readRatingFile(const QString& path);
	// The rating after the newest rated game of these (BL-20 §3.4): its ratingBefore + ratingChange, and
	// its ratedGames (a record from before BL-20 without it: the rated games kept up to it); nullopt when
	// none is rated.
	static std::optional<LocalRating> ratingFromGames(const QList<GameRecord>& records);
	// A profile's kept games that verify, newest first, read from its files as they are (nothing set
	// aside): UserManager rebuilds statistics that fail their check from them.
	static QList<GameRecord> keptRecordsOf(const QString& userId);

signals:
	void gamesChanged();
	void keepCountChanged();
	void ratingChanged();
	void gamesSetAside(int count); // the profile just loaded had `count` games that failed their check
	void ratingRestoredOnLoad();   // the profile just loaded had its rating restored (ratingRestored)

private:
	enum class FileState { Missing, Read, Damaged, Unreadable };
	enum class RatingState { Missing, Verified, Failed };
	// A profile's games: its records, newest first, and its rating.
	struct ProfileFile
	{
		QList<GameRecord> records;
		LocalRating rating;  // the start rating unless rating.json verified
		RatingState ratingState = RatingState::Missing;
		int setAside = 0;   // game files that failed their check
		QStringList resign; // records that verified with an older key: signed again when written
	};
	// Its game files and its rating file. setAside: a failing game file is moved to rejected/ (else
	// only left out).
	static void readProfile(const QString& userId, ProfileFile& file, bool setAside);
	// games.json of BL-19. Unreadable: it is there but couldn't be opened (a lock).
	static FileState readLegacyFile(const QString& path, ProfileFile& file);
	static FileState parseLegacy(const QByteArray& content, ProfileFile& file); // Read or Damaged
	static bool writeGameFile(const QString& userId, const GameRecord& record);
	static bool writeRatingFile(const QString& userId, const LocalRating& rating);
	void rate(GameRecord& record); // put(): rates a game that just finished, or gives a resumed one's change back
	static QStringList trimRecords(QList<GameRecord>& records, int keep); // the ids that went
	QStringList profileIds() const;

	void reload();
	bool save();
	void scheduleSave();
	void trim();
	void trimOtherProfiles(); // their files, as setKeepCount trims the active one's
	FileState migrateLegacyFile(); // games.json → a file per game (once; trusted and signed)
	// After loading (BL-20 §3.4): a rating.json that failed its check is set aside and the rating restored
	// from the games; one behind the rated games that continue it (a crash between the two writes) catches up.
	void recoverRating(RatingState state);
	// The rating after the rated games that continue `rating` exactly (ratingBefore the rating so far, one
	// more rated game each): a game of another profile copied here doesn't.
	static LocalRating continued(LocalRating rating, const QList<GameRecord>& records);
	void migrateOldSaves(); // chess.autosave / chess.save + their GameSaveInfo → records (once)

	UserManager* _userManager = nullptr;
	QString _userId;           // the profile whose games these are; "" = none (nothing is kept)
	QList<GameRecord> _records; // newest first
	LocalRating _rating;        // the profile's rating vs the robot
	int _keepCount = DefaultKeepCount;
	int _setAside = 0;          // game files of this profile set aside when it was loaded
	// Its games.json (BL-19) is there but couldn't be read now (a lock): its games aren't in yet, so the
	// rating isn't known. Every write tries the migration again; games.json is never written over.
	bool _legacyPending = false;
	QStringList _rateLater;    // games that finished while the rating wasn't known: rated once it is
	QSet<QString> _changed;    // records to write: put, analysis, migrated, signed with an older key
	QSet<QString> _removed;    // records whose files go: removed, trimmed
	bool _ratingChanged = false; // rating.json to write
	bool _ratingRestored = false; // ratingRestored: this profile's, when it was loaded
	bool _dirty = false;       // a change not written yet
	bool _saveQueued = false;  // scheduleSave(): the write is on its way
};
