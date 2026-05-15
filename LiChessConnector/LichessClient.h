#pragma once
#include "lichessconnector_global.h"
#include <QNetworkAccessManager>

class QNetworkReply;

class LICHESSCONNECTOR_EXPORT LichessClient: public QObject
{
	Q_OBJECT

public:
	explicit LichessClient(QObject* parent = nullptr);

	void setToken(const QString& decryptedToken);

	Q_INVOKABLE void validateToken(const QString& token);
	Q_INVOKABLE void createOpenChallenge(int minutes, int increment);
	Q_INVOKABLE void acceptChallenge(const QString& challengeId);
	Q_INVOKABLE void streamGame(const QString& gameId);
	Q_INVOKABLE void postMove(const QString& gameId, const QString& uciMove);
	Q_INVOKABLE void resign(const QString& gameId);
	Q_INVOKABLE void stopStream();

	static QString encryptToken(const QString& plaintext);
	static QString decryptToken(const QString& base64Ciphertext);
	static QString gameIdFromUrl(const QString& urlOrId);

	QString currentGameId() const { return _currentGameId; }

signals:
	void tokenValidated(bool ok, QString username);
	void challengeCreated(QString gameId, QString joinUrl);
	void gameStarted(bool playingAsWhite, QString opponentName, QString opponentAvatarUrl);
	void opponentMoveReceived(QString uciMove);
	void gameEnded(QString status, QString winner);
	void networkError(QString message);

private:
	QNetworkRequest makeRequest(const QString& path) const;
	void handleStreamData(QNetworkReply* reply);

	QNetworkAccessManager _nam;
	QString               _token;
	QString               _currentGameId;
	QString               _lastMovesList;   // tracks moves seen so far in stream
	QNetworkReply* _streamReply = nullptr;
};
