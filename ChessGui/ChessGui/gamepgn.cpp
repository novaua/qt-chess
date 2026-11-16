#include "stdafx.h"
#include "gamepgn.h"

#include "gamelibrary.h"
#include "EngineLevel.h"
#include "Game.h"
#include "San.h"

#include <QRegularExpression>

GamePgn::GamePgn(const QString& white, const QString& black, std::vector<std::string> sanMoves)
	: _white(white), _black(black), _san(std::move(sanMoves))
{
}

std::optional<GamePgn> GamePgn::ofRecord(const GameRecord& record, const QString& playerName)
{
	std::vector<std::string> uci;
	for (const auto& m : record.moves)
		uci.push_back(m.toStdString());
	auto game = std::make_shared<Chess::Game>();
	try {
		game->LoadMoves(uci);
		game->ResumeFromLoad();
	}
	catch (const std::exception&) {
		return std::nullopt;
	}
	const QString opponent = record.opponentName.isEmpty() ? QStringLiteral("Guest") : record.opponentName;
	GamePgn pgn(record.playerPlaysWhite ? playerName : opponent,
	            record.playerPlaysWhite ? opponent : playerName,
	            Chess::Game::FormatHistorySan(game->GetGameRecord()));
	pgn.setDate(record.startedAt.date());
	pgn.setResult(record.result);
	if (record.kind == QLatin1String("computer"))
		pgn.setRobot(record.level, !record.playerPlaysWhite);
	else
		pgn.setTwoPlayers();
	pgn.setTimeControl(record.clockLimitSec, record.clockIncSec);
	return pgn;
}

void GamePgn::setRobot(int level, bool robotPlaysWhite)
{
	const Chess::EngineLevel robot(level);
	_event = QStringLiteral("Casual game vs the robot, level %1 (%2)").arg(robot.Level()).arg(QString::fromUtf8(robot.Name()));
	_whiteElo = robotPlaysWhite ? robot.DisplayElo() : 0;
	_blackElo = robotPlaysWhite ? 0 : robot.DisplayElo();
}

void GamePgn::setTwoPlayers()
{
	_event = QStringLiteral("Casual two-player game");
	_whiteElo = _blackElo = 0;
}

void GamePgn::setLichessGame(const QString& gameId)
{
	_site = QStringLiteral("https://lichess.org/") + gameId;
	_timeControl.clear();
}

void GamePgn::setTimeControl(int limitSec, int incSec)
{
	_timeControl = limitSec > 0 || incSec > 0 ? QStringLiteral("%1+%2").arg(limitSec).arg(incSec)
	                                          : QStringLiteral("-");
}

QString GamePgn::text() const
{
	const QString result = _result.isEmpty() ? QStringLiteral("*") : _result;
	auto tag = [](const char* name, const QString& value) {
		return Chess::PgnTag{ name, value.toStdString() };
	};
	std::vector<Chess::PgnTag> tags = {
		tag("Event",  _event),
		tag("Site",   _site),
		tag("Date",   _date.isValid() ? _date.toString(QStringLiteral("yyyy.MM.dd")) : QStringLiteral("????.??.??")),
		tag("Round",  QStringLiteral("-")),
		tag("White",  _white),
		tag("Black",  _black),
		tag("Result", result),
	};
	if (_whiteElo > 0)
		tags.push_back(tag("WhiteElo", QString::number(_whiteElo)));
	if (_blackElo > 0)
		tags.push_back(tag("BlackElo", QString::number(_blackElo)));
	if (!_timeControl.isEmpty())
		tags.push_back(tag("TimeControl", _timeControl));
	return QString::fromStdString(Chess::FormatPgn(tags, _san, result.toStdString()));
}

QString GamePgn::fileName() const
{
	QString name = _white + QStringLiteral(" vs ") + _black
		+ (_date.isValid() ? QLatin1Char(' ') + _date.toString(Qt::ISODate) : QString())
		+ QStringLiteral(".pgn");
	static const QRegularExpression notInFileNames(QStringLiteral("[<>:\"/\\\\|?*]"));
	return name.replace(notInFileNames, QStringLiteral("_"));
}
