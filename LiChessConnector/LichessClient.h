#pragma once
#include "lichessconnector_global.h"
#include <QNetworkAccessManager>
#include <QTimer>

class QNetworkReply;
class QJsonObject;

// Settings of the last game, kept so a rematch can recreate it with colors swapped.
struct RematchInfo
{
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

public:
	explicit LichessClient(QObject* parent = nullptr);

	void setToken(const QString& decryptedToken);
	void setUsername(const QString& username);

	Q_INVOKABLE void validateToken(const QString& token);
	Q_INVOKABLE void createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant);
	Q_INVOKABLE void acceptChallenge(const QString& challengeId);
	Q_INVOKABLE void streamGame(const QString& gameId);
	// registerAsPlayer: POST /accept on our own open challenge first (needed for the Board API
	// stream of an open challenge; not possible for a challenge we sent to a user).
	Q_INVOKABLE void waitForGameStart(const QString& gameId, bool registerAsPlayer = true);

	// Rematch the last game with colors swapped: a direct challenge to a registered opponent
	// (emits directChallengeSent), or a new open challenge for an anonymous one (emits
	// challengeCreated). Either way gameStarted follows once the opponent joins.
	Q_INVOKABLE void createRematch();
	bool canRematch() const { return _lastGame.valid && !_token.isEmpty(); }
	const RematchInfo& lastGame() const { return _lastGame; }

	// Settings of the game described by a Board API "gameFull" stream line, from our side.
	static RematchInfo parseRematchInfo(const QJsonObject& gameFull, const QString& username);
	Q_INVOKABLE void postMove(const QString& gameId, const QString& uciMove);
	Q_INVOKABLE void resign(const QString& gameId);
	Q_INVOKABLE void offerDraw(const QString& gameId, bool accept);
	Q_INVOKABLE void requestTakeback(const QString& gameId, bool accept);
	Q_INVOKABLE void abortGame(const QString& gameId);
	Q_INVOKABLE void stopStream();
	Q_INVOKABLE void cancelChallenge(const QString& challengeId);

	Q_INVOKABLE QString gameIdFromUrl(const QString& urlOrId) const;

	static QString encryptToken(const QString& plaintext);
	static QString decryptToken(const QString& base64Ciphertext);
	static bool    resolveIsWhite(const QString& myColor, const QString& whiteId, const QString& username);

	QString currentGameId() const { return _currentGameId; }

signals:
	void tokenValidated(bool ok, QString username);
	void challengeCreated(QString gameId, QString joinUrl);
	void gameStarted(QString gameId, bool playingAsWhite, QString opponentName, QString opponentAvatarUrl);
	void opponentMoveReceived(QString uciMove);
	void gameEnded(QString status, QString winner);
	void networkError(QString message);
	void challengeCanceled();
	void directChallengeSent(QString challengeId, QString opponentName);
	void challengeDeclined(QString opponentName);
	void canRematchChanged();
	void drawOfferReceived();
	void takebackRequested();

private:
	QNetworkRequest makeRequest(const QString& path) const;
	void postGameAction(const QString& path);
	void createOpenChallengeSec(int limitSec, int incrementSec, int days,
	                            const QString& color, const QString& variant);
	void challengeUser(const RematchInfo& game, const QString& color);
	void handleDirectChallengeLine(const QByteArray& line);
	void stopDirectChallenge();
	void handleStreamData(QNetworkReply* reply);

	void handleEventStreamData(QNetworkReply* reply);
	void stopEventStream();
	void checkGameReady();
	void onGameStartDetected(const QString& gameId);

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
	QNetworkReply* _directChallengeReply = nullptr; // keepAliveStream: open while the challenge waits
	QString        _directChallengeId;
	QTimer         _pollTimer;
	RematchInfo    _lastGame;
};
