#pragma once
#include "onlinecore_global.h"
#include <QObject>
#include <QString>

class OnlineAuth;

// LI-18: what one online server does its own way. OnlineClient speaks the Board API (Lichess's, which other
// servers answer too) and asks its server for the rest: its addresses, its links, what may be played there,
// its name in messages, and how to sign in. Lichess's is LichessServer (LiChessConnector).
class ONLINECORE_EXPORT OnlineServer : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString id READ id CONSTANT)
	Q_PROPERTY(QString name READ name CONSTANT)

public:
	using QObject::QObject;

	virtual QString id() const = 0;     // "lichess": the key of its accounts and kept games
	virtual QString name() const = 0;   // "Lichess": in messages ("Lichess didn't answer")

	virtual QString apiUrl() const = 0;   // the Board API's base, "https://lichess.org"
	virtual QString assetUrl() const = 0; // the flairs' images: <assetUrl>/assets/flair/img/<flair>.webp
	virtual QString webUrl() const = 0;   // the game pages: gameUrl()

	// A game's page; color "white" / "black": that player's link (the side it joins as), "" the game's.
	virtual QString gameUrl(const QString& gameId, const QString& color = {}) const = 0;
	// "Join a game": a link of this server or a bare game id → the id; "" when it is neither.
	virtual QString gameIdFromUrl(const QString& urlOrId) const = 0;
	// The side a player link is for: "white" / "black", else "".
	virtual QString colorFromUrl(const QString& url) const = 0;

	// The variants Chess++ plays here (of those it can play at all: standard, chess960).
	virtual bool playsVariant(const QString& variant) const = 0;
	// A game of this speed ("blitz", "rapid", …), source ("friend", "lobby", "pool", "api", "ai", …) and
	// variant can be played here through the Board API: a challenge's or ongoing game's canPlayHere.
	virtual bool canPlay(const QString& speed, const QString& source, const QString& variant) const = 0;

	// A seek refused for asking too often (HTTP 429).
	virtual QString seekRateLimitMessage() const = 0;

	// A new sign-in, a child of `parent`.
	virtual OnlineAuth* createAuth(QObject* parent) const = 0;
};
