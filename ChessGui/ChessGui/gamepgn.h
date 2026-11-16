#pragma once

#include <QDate>
#include <QString>
#include <optional>
#include <string>
#include <vector>

struct GameRecord;

// A game as PGN (BL-19): the tags that say what game it was, then its moves in SAN. The game on the
// board (ChessConnector::pgn) and a kept game (a Games card's Export PGN) are both written by it, so
// the two say the same about the same game.
//
// Tags: the seven of the PGN standard (Event, Site, Date, Round, White, Black, Result), then the
// robot's Elo (WhiteElo or BlackElo; the player's comes with the local rating, BL-19 step 5) and
// TimeControl ("300+2", "-" for none; left out for a Lichess game: Lichess's own PGN has it).
class GamePgn
{
public:
	GamePgn(const QString& white, const QString& black, std::vector<std::string> sanMoves);

	// A kept game of the active profile, whose name is playerName. Empty when its moves can't be
	// played (a damaged record).
	static std::optional<GamePgn> ofRecord(const GameRecord& record, const QString& playerName);

	void setDate(const QDate& date) { _date = date; }
	void setResult(const QString& result) { _result = result; } // "1-0", "0-1", "1/2-1/2"; "" while unfinished ("*")
	void setRobot(int level, bool robotPlaysWhite);  // vs the robot: the Event names the level, its Elo is tagged
	void setTwoPlayers();                            // two players at this computer
	void setLichessGame(const QString& gameId);      // Site: the game on lichess.org
	void setTimeControl(int limitSec, int incSec);   // a local game's clock; 0 and 0: none

	QString event() const { return _event; }
	QString timeControl() const { return _timeControl; }
	int whiteElo() const { return _whiteElo; }
	int blackElo() const { return _blackElo; }

	QString text() const;     // the whole game, ending with a newline
	QString fileName() const; // e.g. "Alice vs Bob 2026-10-29.pgn", without characters Windows refuses

private:
	QString _white;
	QString _black;
	std::vector<std::string> _san;
	QString _event = QStringLiteral("Casual game");
	QString _site = QStringLiteral("Chess++");
	QDate   _date;
	QString _result;
	QString _timeControl; // "" = no tag
	int _whiteElo = 0;    // 0 = no tag
	int _blackElo = 0;
};
