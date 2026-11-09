#pragma once
#include "lichessconnector_global.h"
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QObject>
#include <QTimer>

class QNetworkAccessManager;

// LI-06 / BL-10: one Lichess ndjson stream (a GET that stays open): the game stream, the event stream.
// Once it is established (setReconnect(true)), a lost connection comes back on its own: the reply
// ends (closed, an error) or brings nothing for silenceMs, not even Lichess's keep-alive line (every
// 7 s). It is opened again after 1, 2, 4, 8, then every 15 s. Lichess's own "no" (4xx) stops that:
// asking again won't change it. No answer or a server error (5xx, Lichess restarting): tried again.
class LICHESSCONNECTOR_EXPORT LichessStream : public QObject
{
	Q_OBJECT

public:
	// name: for the log ("stream", "event stream").
	LichessStream(QNetworkAccessManager& nam, const QString& name, QObject* parent = nullptr);

	void open(const QNetworkRequest& request); // replaces an open one; not established yet
	void reopen();                             // now, with the same request (a resync), unless lost, closed or refused
	void close();                              // no more lines, no reconnect
	void setReconnect(bool reconnect);         // established: a loss reconnects, the silence watchdog runs
	void connected();                          // the owner has seen it's back: connectionChanged(true)
	bool isOpen() const { return _reply || _retry.isActive(); }
	bool lost() const { return _lost; }
	void setTiming(int silenceMs, int firstRetryMs); // tests: shorter waits

signals:
	void line(const QByteArray& line);      // every line, trimmed; an empty one is a keep-alive
	void connectionChanged(bool connected); // false when lost, true from connected()
	void stopped(bool authExpired, int httpStatus); // Lichess refused it (4xx): closed, no more attempts
	// Not established (setReconnect(false)): the reply ended. NoError: Lichess closed it.
	void ended(QNetworkReply::NetworkError error, const QString& errorString, bool authExpired);

private:
	void start(); // GET _request
	void drop();  // abort the reply without a signal
	void lose();  // reconnect after a while

	QNetworkAccessManager& _nam;
	QString         _name;
	QNetworkRequest _request;
	QNetworkReply*  _reply = nullptr;
	QTimer          _retry;
	QTimer          _watchdog;
	int             _firstRetryMs = 1000;
	int             _attempt = 0;
	bool            _reconnect = false;
	bool            _lost = false;
};
