#pragma once
#include "lichessconnector_global.h"
#include "OnlineServer.h"

// LI-18: lichess.org as an OnlineServer: its addresses and links, what its Board API lets an app play (the
// Board API's restrictions), its name, and its sign-in (LichessAuth: OAuth in the browser, or a token).
class LICHESSCONNECTOR_EXPORT LichessServer : public OnlineServer
{
	Q_OBJECT

public:
	using OnlineServer::OnlineServer;

	QString id() const override { return QStringLiteral("lichess"); }
	QString name() const override { return QStringLiteral("Lichess"); }
	QString apiUrl() const override { return QStringLiteral("https://lichess.org"); }
	QString assetUrl() const override { return QStringLiteral("https://lichess1.org"); } // Lichess's asset host
	QString webUrl() const override { return QStringLiteral("https://lichess.org"); }

	QString gameUrl(const QString& gameId, const QString& color = {}) const override;
	// https://lichess.org/XXXXXXXX, …/XXXXXXXX/white, and the challenge links Lichess gives out
	// (https://lichess.org/game/XXXXXXXX?color=black); a bare 8-character id.
	QString gameIdFromUrl(const QString& urlOrId) const override;
	// From …?color=… or …/<id>/white.
	QString colorFromUrl(const QString& url) const override;

	// Standard and chess960 (LI-01A); the Board API itself takes any.
	bool playsVariant(const QString& variant) const override;
	// The Board API plays Rapid, Classical and Correspondence; Blitz only in a direct challenge (from the
	// website "friend", from the API "api"), bulk pairing ("api") or vs Lichess AI (Lichess API spec,
	// "Board → Restrictions").
	bool canPlay(const QString& speed, const QString& source, const QString& variant) const override;

	QString seekRateLimitMessage() const override
	{
		return QStringLiteral("Lichess asks to wait a minute before seeking again");
	}

	OnlineAuth* createAuth(QObject* parent) const override;
};
