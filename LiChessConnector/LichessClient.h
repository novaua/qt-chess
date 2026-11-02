#pragma once
#include "lichessconnector_global.h"
#include <QNetworkAccessManager>
#include <QTimer>
#include <QVariantMap>

class QNetworkReply;
class QJsonObject;

// Settings of the last game, kept so a rematch can recreate it with colors swapped.
struct RematchInfo
{
	QString gameId;              // the game itself: the opponent's rematch offer names it in rematchOf
	QString opponentId;          // Lichess user id; empty = anonymous opponent
	QString opponentName;
	bool    playedAsWhite     = true;
	int     clockLimitSec     = 0; // 0 with days == 0: no clock (unlimited)
	int     clockIncrementSec = 0;
	int     days              = 0; // correspondence: days per move
	QString variant           = QStringLiteral("standard");
	bool    valid             = false;
};

class LICHESSCONNECTOR_EXPORT LichessClient: public QObject
{
	Q_OBJECT
	// LI-04: the player's unfinished games on Lichess (refreshOngoingGames), one map per game, most
	// urgent first: gameId, color, opponentName, opponentRating, opponentIsAi, isMyTurn, secondsLeft,
	// timeLeftText, speed, variant, canPlayHere, lastMove, fen, rated.
	Q_PROPERTY(QVariantList ongoingGames READ ongoingGames NOTIFY ongoingGamesChanged)
	Q_PROPERTY(bool ongoingGamesLoading READ ongoingGamesLoading NOTIFY ongoingGamesChanged)
	Q_PROPERTY(QString ongoingGamesError READ ongoingGamesError NOTIFY ongoingGamesChanged) // the last refresh failed

public:
	explicit LichessClient(QObject* parent = nullptr);

	void setToken(const QString& decryptedToken);
	void setUsername(const QString& username);

	Q_INVOKABLE void validateToken(const QString& token);
	// GET /api/account with our token: emits accountFetched, or authExpired on a 401.
	void refreshAccount();
	// "perfs" of /api/account: { "blitz": { rating, games, prov }, ... } for the pools that have a rating.
	static QVariantMap parseRatings(const QJsonObject& account);
	// color: our side, "white", "black" or "random". Lichess's open challenge has no colour of its
	// own: we take our seat when we accept it (waitForGameStart), and the friend gets the other link.
	// days > 0 (with minutes 0): a correspondence game with that many days per move.
	Q_INVOKABLE void createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant,
	                                     int days = 0);
	// color: accept an open challenge as that side ("" = whatever Lichess gives).
	Q_INVOKABLE void acceptChallenge(const QString& challengeId, const QString& color = {});
	// "Join a game": a Lichess link or a game id. A player link (…?color=black, …/black) joins as
	// that side. False if it is neither a link nor an id.
	Q_INVOKABLE bool joinFromUrl(const QString& urlOrId);
	Q_INVOKABLE void streamGame(const QString& gameId);
	// registerAsPlayer: POST /accept on our own open challenge first (needed for the Board API
	// stream of an open challenge; not possible for a challenge we sent to a user), as myColor.
	Q_INVOKABLE void waitForGameStart(const QString& gameId, bool registerAsPlayer = true, const QString& myColor = {});
	// After a restart: wait again for our own open challenge. We play the side the friend's link doesn't.
	Q_INVOKABLE void resumeOpenChallenge(const QString& gameId, const QString& friendUrl);

	// LI-04: GET /api/account/playing (real-time and correspondence games); no-op without a token.
	Q_INVOKABLE void refreshOngoingGames();
	// Back into one of them: its stream starts with gameFull (all moves), then gameStarted as for any game.
	Q_INVOKABLE void rejoinGame(const QString& gameId);
	QVariantList ongoingGames() const { return _ongoingGames; }
	bool ongoingGamesLoading() const { return _ongoingGamesLoading; }
	QString ongoingGamesError() const { return _ongoingGamesError; }
	void setOngoingGames(const QVariantList& games); // tests and screenshot scenes (no network), from C++
	// Screenshot mode: the refreshes the app starts on its own (refreshAccount, refreshOngoingGames)
	// do nothing; the scenes' token is a placeholder, and no scene creates, joins or plays a game.
	void setOffline(bool offline) { _offline = offline; }
	// The rows of ongoingGames from the answer of /api/account/playing ({ "nowPlaying": [...] }).
	static QVariantList parseOngoingGames(const QJsonObject& playing);
	// "45 s", "3 min", "47 h", "2 days"; "" for no clock (negative).
	static QString timeLeftText(int seconds);

	// Rematch the last game with colors swapped: a direct challenge to a registered opponent
	// (emits directChallengeSent), or a new open challenge for an anonymous one (emits
	// challengeCreated). Either way gameStarted follows once the opponent joins.
	void createRematch(); // via ChessConnector::rematch()
	bool canRematch() const { return _lastGame.valid && !_token.isEmpty(); }
	// After the game: listen for the opponent's rematch offer until stopStream(). Lichess sends it
	// on the event stream as a challenge to us (emits rematchOffered, then maybe rematchOfferCanceled).
	void watchForRematch();
	void acceptRematch();  // the offer's id is the new game's id: gameStarted follows
	void declineRematch();
	// A "challenge" event's challenge that is the opponent's rematch offer after lastGameId.
	static bool isRematchOffer(const QJsonObject& challenge, const QString& lastGameId, const QString& username);
	const RematchInfo& lastGame() const { return _lastGame; }

	// Settings of the game described by a Board API "gameFull" stream line, from our side.
	static RematchInfo parseRematchInfo(const QJsonObject& gameFull, const QString& username);
	Q_INVOKABLE void postMove(const QString& gameId, const QString& uciMove);
	Q_INVOKABLE void resign(const QString& gameId);
	Q_INVOKABLE void offerDraw(const QString& gameId, bool accept);
	Q_INVOKABLE void requestTakeback(const QString& gameId, bool accept);
	Q_INVOKABLE void abortGame(const QString& gameId);
	void claimVictory(const QString& gameId); // allowed once opponentGone's countdown ends
	Q_INVOKABLE void stopStream();
	Q_INVOKABLE void cancelChallenge(const QString& challengeId);

	Q_INVOKABLE QString gameIdFromUrl(const QString& urlOrId) const;
	// The side a player link is for: "white" / "black" from …?color=… or …/<id>/white, else "".
	static QString colorFromUrl(const QString& url);

	static QString encryptToken(const QString& plaintext);
	static QString decryptToken(const QString& base64Ciphertext);
	static bool    resolveIsWhite(const QString& myColor, const QString& whiteId, const QString& username);

	QString currentGameId() const { return _currentGameId; }

	// One line of the Board API game stream (handleStreamData; tests feed lines here).
	// Returns false once the game has ended: the rest of the stream is not read.
	bool handleGameStreamLine(const QByteArray& line);

signals:
	void tokenValidated(bool ok, QString username);
	void accountFetched(QString username, QVariantMap ratings);
	void authExpired(); // Lichess answered 401 to a call made with our token
	void challengeCreated(QString gameId, QString joinUrl);
	void gameStarted(QString gameId, bool playingAsWhite, QString opponentName, QString opponentAvatarUrl);
	void opponentMoveReceived(QString uciMove);
	void gameEnded(QString status, QString winner);
	void networkError(QString message);
	void challengeCanceled();
	void directChallengeSent(QString challengeId, QString opponentName);
	void challengeDeclined(QString opponentName);
	void canRematchChanged();
	void rematchOffered();
	void rematchOfferCanceled(); // withdrawn by the opponent
	// The opponent left (gone) or came back. claimWinInSeconds: when claimVictory() becomes
	// possible; -1 if Lichess didn't say.
	void opponentGone(bool gone, int claimWinInSeconds);
	void drawOfferReceived();
	void takebackRequested();
	void ongoingGamesChanged();

private:
	QNetworkRequest makeRequest(const QString& path) const;
	void checkAuthExpired(QNetworkReply* reply, bool unauthorized);
	void postGameAction(const QString& path);
	void createOpenChallengeSec(int limitSec, int incrementSec, int days,
	                            const QString& color, const QString& variant);
	void challengeUser(const RematchInfo& game, const QString& color);
	void handleDirectChallengeLine(const QByteArray& line);
	void stopDirectChallenge();
	void stopWaitingForDirectChallenge(); // stop probing for the game; keeps the reply
	void handleStreamData(QNetworkReply* reply);

	void handleEventStreamData(QNetworkReply* reply);
	void handleRematchOffer(const QString& type, const QJsonObject& challenge);
	void openEventStream();
	void stopEventStream();
	void checkGameReady();
	void stopProbe(); // abort the /api/board/game/stream probe of checkGameReady()
	void onGameStartDetected(const QString& gameId);
	void cancelOngoingGamesRefresh(); // drop the /api/account/playing request in flight

	QNetworkAccessManager _nam;
	QString               _token;
	QString               _username;
	QString               _currentGameId;
	bool                  _playingAsWhite  = true;
	bool                  _lastOppDraw     = false;
	bool                  _lastOppTakeback = false;
	QString               _waitForGameId;
	QStringList           _lastMovesList;   // tracks moves seen so far in stream
	QNetworkReply* _streamReply      = nullptr;
	QNetworkReply* _eventStreamReply = nullptr;
	QNetworkReply* _probeReply       = nullptr;
	QNetworkReply* _openChallengeReply   = nullptr; // POST /api/challenge/open in flight
	QNetworkReply* _acceptReply          = nullptr; // acceptChallenge's POST in flight (then the stream)
	bool           _gameFullSeen         = false;   // the game stream has sent gameFull: the game is on
	QNetworkReply* _directChallengeReply = nullptr; // keepAliveStream: open while the challenge waits
	QString        _directChallengeId;
	bool           _directChallengeAccepted = false;
	bool           _directChallengeDone     = false; // a {"done":...} line arrived
	QTimer         _pollTimer;
	RematchInfo    _lastGame;
	bool           _watchingForRematch = false; // keep the event stream open for the opponent's offer
	QString        _rematchOfferId;             // the opponent's offer we got; empty = none
	QVariantList   _ongoingGames;
	bool           _ongoingGamesLoading = false;
	QString        _ongoingGamesError;
	QNetworkReply* _ongoingGamesReply = nullptr; // the refresh in flight: a newer one replaces it
	bool           _offline = false;
};
