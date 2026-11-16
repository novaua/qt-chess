#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>
#include <QStringList>

// The engine's analysis of a kept game (BL-19 step 6): for every position 0…n of the game (the start,
// then after each move) its evaluation from White's view and the engine's best move there. What a move
// was worth follows from these as on lichess.org: the win chance of each position, the mover's drop by
// the move (an inaccuracy, a mistake or a blunder), the accuracy of each side. Stored on the record
// (GameRecord::analysis) as it came from the engine; the rest is worked out when read.
class GameAnalysis
{
public:
	enum class Judgement { None, Inaccuracy, Mistake, Blunder };

	static constexpr int FormatVersion = 1;
	static constexpr int MateCp = 100000; // a mate in N: ±(MateCp − N), as UciConnector's MateScoreCp
	static constexpr int CeilingCp = 1000; // win chances read a score as at most this (Lichess's "ceiled")
	// The mover's drop in win chance (percentage points, 0–100) from which a move is judged. Lichess's
	// 0.1 / 0.2 / 0.3 of its winning chances, which run from −1 to 1.
	static constexpr double InaccuracyDrop = 5, MistakeDrop = 10, BlunderDrop = 15;

	GameAnalysis() = default;
	// evals: one per position, the start position first (so one more than the moves); best: as many
	// ("" where the game is over); moves: the game's, which it is of (kept as a key: fits()).
	GameAnalysis(int depth, QList<int> evals, QStringList best, const QStringList& moves = {});

	bool isEmpty() const { return _evals.isEmpty(); }
	int  depth() const { return _depth; }
	int  plyCount() const { return isEmpty() ? 0 : (int)_evals.size() - 1; }
	// Made for these moves: the same moves (their key), not only as many. GameLibrary::put drops one that
	// doesn't fit (a takeback, the game went on, moves typed in over it).
	bool fits(const QStringList& moves) const;
	static QString movesKey(const QStringList& moves); // a short hash of the moves; "" for none

	int     eval(int position) const { return _evals.value(position); }     // White's view, centipawns or a mate
	QString bestMove(int position) const { return _best.value(position); }  // UCI; "" when the game is over there
	static bool isMate(int cp) { return cp > MateCp / 2 || cp < -MateCp / 2; }
	// Moves to the mate (> 0: White mates, < 0: Black mates); 0 when it isn't one.
	static int mateIn(int cp) { return !isMate(cp) ? 0 : cp > 0 ? MateCp - cp : -(MateCp + cp); }

	// White's chance to win, 0–100, Lichess's curve: 50 + 50·(2/(1 + e^(−0.00368208·cp)) − 1).
	static double winPercent(int cpWhite);
	// The accuracy of one move, 0–100, from the mover's win chance before and after (Lichess's formula).
	static double moveAccuracy(double winBefore, double winAfter);

	// The move of ply `ply` (0: White's first) goes from position ply to ply + 1.
	bool   whiteMoved(int ply) const { return ply % 2 == 0; }
	double drop(int ply) const; // the mover's loss of win chance by it (≤ 0: none)
	Judgement judgement(int ply) const;
	// The side's accuracy, 0–100, as lichess.org's game accuracy: the mean of a volatility-weighted mean
	// and the harmonic mean of its moves' accuracies; −1 when the side made no move.
	double accuracy(bool white) const;
	int count(bool white, Judgement judgement) const;

	static QString judgementName(Judgement j); // "inaccuracy", "mistake", "blunder"; "" for none

	QJsonObject toJson() const;
	static GameAnalysis fromJson(const QJsonObject& o); // empty when it isn't one (or of a later format)

	bool operator==(const GameAnalysis&) const = default;

private:
	int _depth = 0;
	QList<int> _evals;
	QStringList _best;
	QString _movesKey;
};
