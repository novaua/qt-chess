#include "stdafx.h"
#include "LichessServer.h"
#include "LichessAuth.h"

#include <QRegularExpression>

QString LichessServer::gameUrl(const QString& gameId, const QString& color) const
{
	const QString game = webUrl() + QLatin1Char('/') + gameId;
	return color.isEmpty() ? game : game + QLatin1Char('/') + color;
}

QString LichessServer::gameIdFromUrl(const QString& urlOrId) const
{
	static const QRegularExpression link(QStringLiteral("(?:lichess\\.org/)(?:game/)?([A-Za-z0-9]{8})"));
	const auto match = link.match(urlOrId);
	if (match.hasMatch())
		return match.captured(1);
	static const QRegularExpression bareId(QStringLiteral("^[A-Za-z0-9]{8}$"));
	if (bareId.match(urlOrId.trimmed()).hasMatch())
		return urlOrId.trimmed();
	return {};
}

QString LichessServer::colorFromUrl(const QString& url) const
{
	static const QRegularExpression re(
		QStringLiteral("(?:[?&]color=|lichess\\.org/(?:game/)?[A-Za-z0-9]{8}/)(white|black)\\b"));
	const auto match = re.match(url);
	return match.hasMatch() ? match.captured(1) : QString();
}

bool LichessServer::playsVariant(const QString& variant) const
{
	return variant == QLatin1String("standard") || variant == QLatin1String("chess960");
}

bool LichessServer::canPlay(const QString& speed, const QString& source, const QString& variant) const
{
	if (!playsVariant(variant))
		return false;
	if (speed == QLatin1String("rapid") || speed == QLatin1String("classical") || speed == QLatin1String("correspondence"))
		return true;
	return speed == QLatin1String("blitz")
		&& (source == QLatin1String("friend") || source == QLatin1String("api") || source == QLatin1String("ai"));
}

OnlineAuth* LichessServer::createAuth(QObject* parent) const
{
	return new LichessAuth(parent);
}
