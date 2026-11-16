#include "stdafx.h"
#include "gameanalysis.h"

#include <QCryptographicHash>
#include <QJsonArray>
#include <algorithm>
#include <cmath>

GameAnalysis::GameAnalysis(int depth, QList<int> evals, QStringList best, const QStringList& moves)
	: _depth(depth), _evals(std::move(evals)), _best(std::move(best)), _movesKey(movesKey(moves))
{
	while (_best.size() < _evals.size())
		_best.append(QString());
}

QString GameAnalysis::movesKey(const QStringList& moves)
{
	if (moves.isEmpty())
		return {};
	return QString::fromLatin1(QCryptographicHash::hash(moves.join(' ').toLatin1(), QCryptographicHash::Sha1)
	                           .toHex().left(16));
}

bool GameAnalysis::fits(const QStringList& moves) const
{
	return !isEmpty() && plyCount() == moves.size() && _movesKey == movesKey(moves);
}

double GameAnalysis::winPercent(int cpWhite)
{
	const int cp = std::clamp(cpWhite, -CeilingCp, CeilingCp); // a mate reads as the ceiling
	return 50 + 50 * (2 / (1 + std::exp(-0.00368208 * cp)) - 1);
}

double GameAnalysis::moveAccuracy(double winBefore, double winAfter)
{
	if (winAfter >= winBefore)
		return 100;
	const double raw = 103.1668100711649 * std::exp(-0.04354415386753951 * (winBefore - winAfter)) - 3.166924740191411;
	return std::clamp(raw + 1, 0.0, 100.0); // + 1: Lichess's allowance for an analysis that isn't perfect
}

double GameAnalysis::drop(int ply) const
{
	if (ply < 0 || ply >= plyCount())
		return 0;
	const double before = winPercent(_evals[ply]), after = winPercent(_evals[ply + 1]);
	return whiteMoved(ply) ? before - after : after - before;
}

GameAnalysis::Judgement GameAnalysis::judgement(int ply) const
{
	const double d = drop(ply);
	return d >= BlunderDrop ? Judgement::Blunder
	     : d >= MistakeDrop ? Judgement::Mistake
	     : d >= InaccuracyDrop ? Judgement::Inaccuracy
	     : Judgement::None;
}

int GameAnalysis::count(bool white, Judgement j) const
{
	int n = 0;
	for (int ply = white ? 0 : 1; ply < plyCount(); ply += 2)
		n += judgement(ply) == j ? 1 : 0;
	return n;
}

namespace
{
	double standardDeviation(const QList<double>& xs)
	{
		if (xs.isEmpty())
			return 0;
		double mean = 0;
		for (double x : xs)
			mean += x;
		mean /= xs.size();
		double variance = 0;
		for (double x : xs)
			variance += (x - mean) * (x - mean);
		return std::sqrt(variance / xs.size());
	}
}

// lichess.org's game accuracy (lila, AccuracyPercent.gameAccuracy): each move's accuracy weighted by how
// much the win chances swing around it (the standard deviation of a window of positions, 0.5–12), so
// quiet stretches count less; the mean of that weighted mean and the harmonic mean (which a few bad
// moves pull down).
double GameAnalysis::accuracy(bool white) const
{
	const int moves = plyCount();
	if (moves < (white ? 1 : 2))
		return -1;
	QList<double> wins;
	for (int cp : _evals)
		wins.append(winPercent(cp));

	const int windowSize = std::clamp(moves / 10, 2, 8);
	QList<double> weights;
	for (int i = 0; i < windowSize - 2; ++i) // the first moves share the first window
		weights.append(std::clamp(standardDeviation(wins.mid(0, windowSize)), 0.5, 12.0));
	for (int start = 0; start + windowSize <= wins.size(); ++start)
		weights.append(std::clamp(standardDeviation(wins.mid(start, windowSize)), 0.5, 12.0));

	double weighted = 0, weightSum = 0, inverseSum = 0;
	int n = 0;
	for (int ply = white ? 0 : 1; ply < moves && ply < weights.size(); ply += 2) {
		const double a = white ? moveAccuracy(wins[ply], wins[ply + 1])
		                       : moveAccuracy(100 - wins[ply], 100 - wins[ply + 1]);
		weighted += a * weights[ply];
		weightSum += weights[ply];
		inverseSum += 1 / std::max(1.0, a);
		++n;
	}
	if (n == 0)
		return -1;
	return (weighted / weightSum + n / inverseSum) / 2;
}

QString GameAnalysis::judgementName(Judgement j)
{
	switch (j) {
	case Judgement::Inaccuracy: return QStringLiteral("inaccuracy");
	case Judgement::Mistake:    return QStringLiteral("mistake");
	case Judgement::Blunder:    return QStringLiteral("blunder");
	default:                    return QString();
	}
}

QJsonObject GameAnalysis::toJson() const
{
	if (isEmpty())
		return {};
	QJsonArray evals;
	for (int cp : _evals)
		evals.append(cp);
	return QJsonObject{
		{ QStringLiteral("version"), FormatVersion },
		{ QStringLiteral("depth"),   _depth },
		{ QStringLiteral("evals"),   evals },
		{ QStringLiteral("best"),    QJsonArray::fromStringList(_best) },
		{ QStringLiteral("moves"),   _movesKey },
	};
}

GameAnalysis GameAnalysis::fromJson(const QJsonObject& o)
{
	if (o.value(QStringLiteral("version")).toInt() != FormatVersion)
		return {};
	QList<int> evals;
	for (const auto& v : o.value(QStringLiteral("evals")).toArray())
		evals.append(v.toInt());
	QStringList best;
	for (const auto& v : o.value(QStringLiteral("best")).toArray())
		best.append(v.toString());
	if (evals.size() < 2 || best.size() != evals.size())
		return {};
	GameAnalysis analysis(o.value(QStringLiteral("depth")).toInt(), evals, best);
	analysis._movesKey = o.value(QStringLiteral("moves")).toString(); // none: it fits no moves
	return analysis;
}
