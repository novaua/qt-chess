#pragma once
#include "lichessconnector_global.h"
#include "LichessStream.h"
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QSet>
#include <QTimer>
#include <QVariantMap>

class QNetworkReply;

// LI-07: a player of a Lichess game, as gameFull gives them (the Board API's GameEventPlayer): name,
// title ("GM", "BOT", …), rating and whether it is provisional. Lichess's AI has no name, only its level.
// gameFull has no flair: LichessClient asks /api/users for it once the game has started (LI-11).
class LICHESSCONNECTOR_EXPORT LichessPlayer
{
public:
	LichessPlayer() = default;
	static LichessPlayer fromJson(const QJsonObject& player);
	// The player as gameFull has them, plus the flair once it came ("flairUrl"): fromJson reads it back
	// (LI-14: a kept Lichess game's players).
	QJsonObject toJson() const;

	QString name() const;  // "Stockfish level 3" for the AI; "" for an anonymous player
	QString title() const { return _title; }
	int     rating() const { return _rating; }  // 0: none (anonymous, the AI)
	bool    provisional() const { return _provisional; }
	QString ratingText() const;  // "1523", "1523?" (provisional, as lichess.org writes it), "" without one
	bool    isAi() const { return _aiLevel > 0; }
	QString id() const { return _id; }          // Lichess's user id (lowercase); "" for the AI, an anonymous player
	QString flairUrl() const { return _flairUrl; } // LI-11: the flair's image, "" without one (or not known yet)
	void    setFlairUrl(const QString& url) { _flairUrl = url; }

private:
	QString _id;
	QString _flairUrl;
	QString _name;
	QString _title;
	int     _rating = 0;
	bool    _provisional = false;
	int     _aiLevel = 0;
};

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
	QString startFen;            // a Chess960 game's start (gameFull's initialFen): its rematch keeps it (LI-01A); "" = none
	bool    rated             = false;
	bool    fromSeek          = false; // LI-16: paired by our real-time seek (quick pairing): seekAgain() repeats it
	bool    valid             = false;
};

class LICHESSCONNECTOR_EXPORT LichessClient: public QObject
{
	Q_OBJECT
	// LI-04: the player's unfinished games on Lichess (refreshOngoingGames), one map per game, most
	// urgent first: gameId, color, opponentName, opponentRating, opponentIsAi, isMyTurn, secondsLeft,
	// timeLeftText, speed, variant, variantName ("Chess960"), canPlayHere, lastMove, fen, rated.
	Q_PROPERTY(QVariantList ongoingGames READ ongoingGames NOTIFY ongoingGamesChanged)
	Q_PROPERTY(bool ongoingGamesLoading READ ongoingGamesLoading NOTIFY ongoingGamesChanged)
	Q_PROPERTY(QString ongoingGamesError READ ongoingGamesError NOTIFY ongoingGamesChanged) // the last refresh failed
	// BL-10: the challenges to the player waiting on Lichess, oldest first: id, url, challengerId,
	// challengerName, challengerRating (0: none), challengerProvisional, challengerTitle,
	// challengerText ("FM Speedy (2105?)"), timeControlText ("10+5", "3 days per move", "Unlimited"),
	// speed, rated, variant, variantName, ourSide ("white", "black", "random"), sideText ("You play Black",
	// "Random side"), canPlayHere (the Board API can play it), rematchOf.
	Q_PROPERTY(QVariantList incomingChallenges READ incomingChallenges NOTIFY incomingChallengesChanged)
	// The opponent's rematch offer the finished game's own bar shows (watchForRematch); "" for none.
	// The challenge bar leaves that one out.
	Q_PROPERTY(QString rematchOfferId READ rematchOfferId NOTIFY rematchOfferIdChanged)
	// LI-09: the real-time seek waiting for an opponent (startSeek): {} when none, else { limitSec,
	// incrementSec, speed ("rapid", "classical"), rated, variant, since (ms since the epoch) }.
	Q_PROPERTY(QVariantMap seek READ seekState NOTIFY seekChanged)
	Q_PROPERTY(bool seeking READ seeking NOTIFY seekChanged)
	// The seconds the seek has waited, counted here once for every view (the grid's tile, each page's
	// SeekIndicator): it changes once a second, only while seeking.
	Q_PROPERTY(int seekWaitedSec READ seekWaitedSec NOTIFY seekWaitedChanged)

public:
	explicit LichessClient(QObject* parent = nullptr);
	~LichessClient() override;

	void setToken(const QString& decryptedToken);
	void setUsername(const QString& username);

	Q_INVOKABLE void validateToken(const QString& token);
	// GET /api/account with our token: emits accountFetched, or authExpired on a 401.
	void refreshAccount();
	// "perfs" of /api/account: { "blitz": { rating, games, prov }, ... } for the pools that have a rating
	// (bullet, blitz, rapid, classical, correspondence, chess960).
	static QVariantMap parseRatings(const QJsonObject& account);
	// The variants Chess++ plays: standard and chess960 (LI-01A). The Board API itself takes any.
	static bool playableVariant(const QString& variant);
	// Lichess's name of a variant key: "chess960" → "Chess960", "kingOfTheHill" → "King of the Hill".
	Q_INVOKABLE static QString variantName(const QString& variant);
	// A gameFull's variant ("standard" when none) and its start position: initialFen, "" for "startpos".
	static QString variantOf(const QJsonObject& gameFull);
	static QString startFenOf(const QJsonObject& gameFull);
	// color: our side, "white", "black" or "random". Lichess's open challenge has no colour of its
	// own: we take our seat when we accept it (waitForGameStart), and the friend gets the other link.
	// days > 0 (with minutes 0): a correspondence game with that many days per move.
	Q_INVOKABLE void createOpenChallenge(int minutes, int increment, const QString& color, const QString& variant,
	                                     int days = 0);
	// A challenge to a named player: the only way to play Blitz from a Board API app (LI-02). Sent:
	// directChallengeSent; refused by Lichess (no such player): networkError. limitSec 0 with days 0:
	// unlimited. color: "white", "black" or "random". variant: "standard" or "chess960" (Lichess picks the position).
	Q_INVOKABLE void challengePlayer(const QString& username, int limitSec, int incrementSec, int days, const QString& color,
	                                 const QString& variant = QStringLiteral("standard"));
	Q_INVOKABLE static bool isUsername(const QString& text); // could be one: 2 to 30 letters, digits, - or _
	// Lichess drops a challenge with a clock after 20 s unless a request keeps it alive. The others
	// stay there for two weeks: no need to wait for them.
	static bool needsKeepAlive(const RematchInfo& game) { return game.clockLimitSec > 0 || game.clockIncrementSec > 0; }
	// color: accept an open challenge as that side ("" = whatever Lichess gives). A challenge to the
	// player (incomingChallenges) stays listed until its game starts; if Lichess refuses it,
	// challengeAcceptFailed says why (otherwise networkError).
	Q_INVOKABLE void acceptChallenge(const QString& challengeId, const QString& color = {});
	// BL-16: the challenge acceptChallenge() is accepting, until its game starts (gameStarted), Lichess
	// refuses it (challengeAcceptFailed), its stream fails (networkError) or stopStream() drops it. Its
	// row stays in incomingChallenges meanwhile (also after Lichess's gameStart), so its Accept says so.
	QString acceptingChallengeId() const { return _acceptingId; }
	// "Join a game": a Lichess link or a game id. A player link (…?color=black, …/black) joins as
	// that side. False if it is neither a link nor an id.
	Q_INVOKABLE bool joinFromUrl(const QString& urlOrId);
	Q_INVOKABLE void streamGame(const QString& gameId);
	// LI-06: open the running game's stream again now; its gameFull tells what Lichess has (a move of
	// ours that got no answer). While the stream is lost it reconnects on its own.
	void resyncGame();
	// Tests: a fake server instead of lichess.org, and shorter waits: a stream is lost after
	// silenceMs without even a keep-alive line (Lichess sends one every 7 s), the first reconnect
	// comes after firstRetryMs (then twice as long each time, up to 15 s), a move without an answer
	// gives up after moveTimeoutMs, challenges not resent within settleMs of a reconnect are gone, and
	// an event stream Lichess refused (a 4xx but 401, e.g. 429) is tried again after refusedRetryMs.
	void setBaseUrl(const QString& url) { _baseUrl = url; }
	// LI-11: where the flair images are (lichess1.org, Lichess's asset host); the tests' fake server instead.
	void setAssetBaseUrl(const QString& url) { _assetBaseUrl = url; }
	// A flair's image: <asset base>/assets/flair/img/<flair>.webp; "" for a name that isn't one
	// ("people.man-raising-hand": lowercase letters, digits, '.' and '-' only).
	QString flairUrl(const QString& flair) const;
	void setStreamTiming(int silenceMs, int firstRetryMs, int moveTimeoutMs, int settleMs = 3000,
	                     int refusedRetryMs = 60000);

	// BL-10: the account's event stream (/api/stream/event), open and reconnecting until the token
	// changes: incoming challenges, the start of a game we wait for, rematch offers, games started or
	// ended elsewhere. Lichess allows one per token: everything here shares it. No-op without a token
	// or offline (screenshot mode).
	void startEventStream();
	bool eventStreamOpen() const { return _eventStream->isOpen(); }
	QVariantList incomingChallenges() const { return _incomingChallenges; }
	QString rematchOfferId() const { return _rematchOfferId; }
	// Lichess shows the challenger the reason: generic, later, tooFast, tooSlow, timeControl, rated,
	// casual, standard, variant, noBot, onlyBot.
	Q_INVOKABLE void declineChallenge(const QString& challengeId, const QString& reason = QStringLiteral("generic"));

	// LI-09 quick pairing: a public seek (POST /api/board/seek) for a random opponent. Real time
	// (limitSec / incrementSec; the Board API allows Rapid and Classical only, Lichess refuses the rest):
	// the seek lives while its request stays open, seekState() says so, and its game starts as any
	// (gameStarted), from the first gameStart of a lobby or pool game that isn't a known one and is of
	// the seek's speed, variant and rating (seekMatches). days > 0 (limitSec 0): a correspondence seek,
	// left on Lichess (seekSent; the API can't cancel it). One seek at a time: a new one replaces the last.
	// Not while a real-time challenge of ours waits (one game at a time): refused. Refused or ended
	// unpaired: seekFailed. Offline (screenshot mode): the state only, nothing sent.
	Q_INVOKABLE void startSeek(int limitSec, int incrementSec, int days, bool rated, const QString& variant);
	// Closes the real-time seek's request: Lichess drops the seek. Also a game starting (streamGame; a
	// rejoin once its game is there, so a rejoin that fails keeps the seek), an accept (acceptChallenge,
	// acceptRematch), a real-time challenge of ours (createOpenChallenge, challengePlayer), another token
	// or none, ChessConnector::switchUser. Not stopStream(): a seek outlives leaving a game.
	Q_INVOKABLE void cancelSeek();
	QVariantMap seekState() const { return _seek; }
	bool seeking() const { return !_seek.isEmpty(); }
	int seekWaitedSec() const { return _seekWaitedSec; }
	// Screenshot scenes: the waited time shown, the same every run (the count stops).
	Q_INVOKABLE void fixSeekWaited(int seconds);
	// Lichess's speed of a clock: limit + 40 × increment seconds: "ultraBullet", "bullet", "blitz",
	// "rapid", "classical".
	Q_INVOKABLE static QString speedOf(int limitSec, int incrementSec);
	// A gameStart event's game ({ gameId, source, speed, variant, rated, hasMoved, … }) that is the
	// running seek's.
	bool isSeekGame(const QJsonObject& game) const;
	// Tests: Lichess closed the seek's request (paired or expired): Your games is asked quietly (not
	// the visible list's refresh) every pollMs for its game, given up after fallbackMs.
	void setSeekTiming(int fallbackMs, int pollMs);
	// A "challenge" event ({ challenge, compat }) as a row of incomingChallenges.
	static QVariantMap parseChallenge(const QJsonObject& challengeEvent);
	// One line of the event stream (tests feed lines here).
	void handleEventStreamLine(const QByteArray& line);
	// registerAsPlayer: POST /accept on our own open challenge first (needed for the Board API
	// stream of an open challenge; not possible for a challenge we sent to a user), as myColor.
	Q_INVOKABLE void waitForGameStart(const QString& gameId, bool registerAsPlayer = true, const QString& myColor = {});
	// After a restart: wait again for our own open challenge. We play the side the friend's link doesn't.
	Q_INVOKABLE void resumeOpenChallenge(const QString& gameId, const QString& friendUrl);

	// LI-04: GET /api/account/playing (real-time and correspondence games); no-op without a token.
	Q_INVOKABLE void refreshOngoingGames();
	// Back into one of them: its stream starts with gameFull (all moves), then gameStarted as for any game.
	// Until then rejoiningGameId() is the game's; a stream that ends first gives rejoinFailed (not
	// networkError), and stopStream() drops the rejoin quietly.
	Q_INVOKABLE void rejoinGame(const QString& gameId);
	QString rejoiningGameId() const { return _rejoiningGameId; }
	QVariantList ongoingGames() const { return _ongoingGames; }
	bool ongoingGamesLoading() const { return _ongoingGamesLoading; }
	// The list came from Lichess (or setOngoingGames) at least once for this account: while a refresh is in
	// flight, ongoingGames() is still the last list, not an empty one that hasn't loaded yet (BL-24).
	bool ongoingGamesLoaded() const { return _ongoingGamesLoaded; }
	bool isOngoingGame(const QString& gameId) const; // in ongoingGames()
	QString ongoingGamesError() const { return _ongoingGamesError; }
	void setOngoingGames(const QVariantList& games); // tests and screenshot scenes (no network), from C++
	// Screenshot mode: the refreshes the app starts on its own (refreshAccount, refreshOngoingGames)
	// do nothing, and our moves aren't posted (the premove scenes play one); the scenes' token is a
	// placeholder, and no scene creates or joins a game.
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
	// LI-16: the last game was paired by our real-time seek (matched in the event stream or Your games while it
	// waited; a rejoin of it later isn't known as one). seekAgain(): the same seek again (startSeek: its clock,
	// rating and variant; a Chess960 one gets a new position). Changes with canRematchChanged.
	bool canSeekAgain() const { return canRematch() && _lastGame.fromSeek; }
	void seekAgain(); // via ChessConnector::newOpponent()
	// After the game: listen for the opponent's rematch offer until stopStream(). Lichess sends it
	// on the event stream as a challenge to us (emits rematchOffered, then maybe rematchOfferCanceled).
	void watchForRematch();
	void acceptRematch();  // the offer's id is the new game's id: gameStarted follows
	void declineRematch();
	// A "challenge" event's challenge that is the opponent's rematch offer after lastGameId.
	static bool isRematchOffer(const QJsonObject& challenge, const QString& lastGameId, const QString& username);
	const RematchInfo& lastGame() const { return _lastGame; }
	// LI-07: the current game's players, from its gameFull (set before gameStarted); empty before one.
	const LichessPlayer& gamePlayer(bool white) const { return white ? _white : _black; }
	// LI-14: the started game's gameFull as it came (id, rated, speed, clock, daysPerTurn, createdAt, the
	// players, …), set before gameStarted; {} once a refused game is dropped.
	const QJsonObject& currentGameFull() const { return _gameFull; }
	// LI-11: what /api/users says of the players' flairs (fetchPlayerFlairs), for the players of
	// gamePlayer() of the game that started; gamePlayersChanged says so.

	// Settings of the game described by a Board API "gameFull" stream line, from our side.
	static RematchInfo parseRematchInfo(const QJsonObject& gameFull, const QString& username);
	Q_INVOKABLE void postMove(const QString& gameId, const QString& uciMove);
	// Our move is on our board and on its way (postMove() calls it; tests call it directly): the
	// stream's echo of it isn't the opponent's move.
	void trackSentMove(const QString& uciMove) { _sentMoves.append(uciMove); }
	Q_INVOKABLE void resign(const QString& gameId);
	Q_INVOKABLE void offerDraw(const QString& gameId, bool accept);
	Q_INVOKABLE void requestTakeback(const QString& gameId, bool accept);
	Q_INVOKABLE void abortGame(const QString& gameId);
	void claimVictory(const QString& gameId); // allowed once opponentGone's countdown ends
	Q_INVOKABLE void stopStream();
	Q_INVOKABLE void cancelChallenge(const QString& challengeId); // challengeCanceled at once

	Q_INVOKABLE QString gameIdFromUrl(const QString& urlOrId) const;
	// The side a player link is for: "white" / "black" from …?color=… or …/<id>/white, else "".
	static QString colorFromUrl(const QString& url);

	static QString encryptToken(const QString& plaintext);
	static QString decryptToken(const QString& base64Ciphertext);
	static bool    resolveIsWhite(const QString& myColor, const QString& whiteId, const QString& username);

	QString currentGameId() const { return _currentGameId; }
	// Tests and the takeback scene: the game of the lines fed to handleGameStreamLine(), without a stream.
	void setCurrentGameId(const QString& gameId) { _currentGameId = gameId; }
	// The lichess-new-opponent scene (LI-16): the game fed next was paired by a seek, without the event stream.
	void setSeekGameId(const QString& gameId) { _seekGameId = gameId; }

	// One line of the Board API game stream (from _gameStream; tests feed lines here).
	// Returns false once the game has ended: the stream is closed, the rest not read.
	bool handleGameStreamLine(const QByteArray& line);

signals:
	void tokenValidated(bool ok, QString username);
	void accountFetched(QString username, QVariantMap ratings);
	void authExpired(); // Lichess answered 401 to a call made with our token
	void challengeCreated(QString gameId, QString joinUrl);
	// moves: the game's moves so far (UCI), e.g. of a game rejoined; empty for a new game. They are
	// the start's, not sent again with opponentMoveReceived (BL-18). variant: gameFull's ("standard",
	// "chess960", …: any, the receiver refuses what it can't play); startFen: its initialFen, "" for the
	// standard start (LI-01A).
	void gamePlayersChanged(); // LI-11: gamePlayer()'s flairs came (after gameStarted)
	void gameStarted(QString gameId, bool playingAsWhite, QString opponentName, QString opponentAvatarUrl, QStringList moves,
	                 QString variant, QString startFen);
	void opponentMoveReceived(QString uciMove); // the opponent's new move(s) after the start
	// The game's moves changed other than by new moves: an accepted takeback (a shorter list), a
	// move of ours that Lichess refused, or a list that disagrees with ours. All the moves, as Lichess has them.
	void moveListReplaced(QString uciMoves, bool moveRefused); // moveRefused: Lichess refused our move
	// LI-02: the game's clock, after gameStarted; correspondence: limitMs is the time for a move.
	void clockStarted(qint64 limitMs, qint64 incrementMs, bool correspondence);
	void clockUpdated(qint64 whiteMs, qint64 blackMs, bool whiteToMove, bool running); // Lichess's times
	void gameEnded(QString status, QString winner);
	void networkError(QString message);
	void rejoiningChanged();                          // rejoinGame() started, or ended (a game, a failure, stopStream)
	void acceptingChanged();                          // acceptingChallengeId() changed
	void rejoinFailed(QString gameId, QString message); // rejoinGame()'s stream ended before its game started
	void challengeCanceled();
	void directChallengeSent(QString challengeId, QString opponentName); // we wait for the answer
	void directChallengeLeft(QString opponentName); // it stays on Lichess: the game is in ongoingGames once accepted
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
	void incomingChallengesChanged();
	void challengeReceived(QString challengeId); // BL-10: a new one (not one resent after a reconnect, nor the rematch bar's)
	void challengeAcceptFailed(QString challengeId, QString message); // BL-10: Lichess refused Accept
	void rematchOfferIdChanged();
	// LI-06: the running game's stream was lost (false; it reconnects on its own) or is back (true,
	// once its gameFull has handed over what changed: the moves follow through the gate).
	void gameConnectionChanged(bool connected);
	// LI-06: Lichess itself refused the stream (a 401, a game it no longer streams): no more
	// reconnects. networkError says why.
	void gameStreamStopped();
	void seekChanged();
	void seekWaitedChanged();
	void seekFailed(QString message); // LI-09: Lichess refused the seek, or ended it unpaired
	void seekSent(QString message);   // LI-09: a correspondence seek is on Lichess

private:
	QNetworkRequest makeRequest(const QString& path) const;
	void checkAuthExpired(QNetworkReply* reply, bool unauthorized);
	void postGameAction(const QString& path);
	void createOpenChallengeSec(int limitSec, int incrementSec, int days,
	                            const QString& color, const QString& variant, const QString& startFen = {});
	void challengeUser(const RematchInfo& game, const QString& color);
	void handleDirectChallengeLine(const QByteArray& line);
	void stopDirectChallenge();
	void stopWaitingForDirectChallenge(); // stop probing for the game; keeps the reply
	void gameStreamEnded(QNetworkReply::NetworkError error, const QString& errorString, bool authExpired); // before gameFull
	void setRejoiningGameId(const QString& gameId);
	void setAcceptingId(const QString& challengeId);
	void stopAll(bool keepAccept); // stopStream(); keepAccept: streamGame() of the game being accepted
	void fetchPlayerFlairs(); // LI-11: POST /api/users for the game's players' flairs (one request a game)
	void gameStreamRefused(bool authExpired, int httpStatus); // mid-game: Lichess said no, no more reconnects
	bool resyncMoves(const QStringList& moves); // a gameFull after a reconnect; true: our move was lost
	void applyStreamMoves(const QStringList& moves); // a gameState's moves against ours
	void startClock(const QJsonObject& gameFull);
	void updateClock(const QJsonObject& state, bool gameOver); // a gameFull's state, or a gameState
	void replaceMoveList(const QStringList& moves, bool moveRefused = false);

	void handleRematchOffer(const QString& type, const QJsonObject& challenge);
	void addIncomingChallenge(const QJsonObject& challengeEvent);
	void removeIncomingChallenge(const QString& challengeId);
	void dropChallengesNotResent(); // after a reconnect: the ones Lichess didn't send again are gone
	bool isIncomingChallenge(const QString& challengeId) const;
	void setRematchOfferId(const QString& id);
	void checkGameReady();
	void stopProbe(); // abort the /api/board/game/stream probe of checkGameReady()
	void onGameStartDetected(const QString& gameId);
	void cancelOngoingGamesRefresh(); // drop the /api/account/playing request in flight
	void endSeek(bool notify);        // close the seek's request, stop its fallback; seekChanged if notify
	void seekRequestFinished(QNetworkReply* reply);
	void pollSeekGame();              // the fallback: Your games asked quietly for the seek's game
	bool findSeekGame(const QVariantList& games); // a game there is the seek's: it starts
	// One rule for a gameStart and a row of Your games: a game Lichess paired (lobby, pool), new (not
	// known when the seek began, nor on the board or being accepted, no move of ours yet), of the seek's
	// speed, variant and rating. An empty speed or a missing rating: not told, not against it.
	bool seekMatches(const QString& gameId, const QString& source, const QString& speed, const QString& variant,
	                 const QVariant& rated, bool hasMoved) const;
	bool challengeWaiting() const;    // ours, created and waiting for the opponent (the challenge dialog)
	void openGameStream(const QString& gameId); // streamGame() without ending the seek (a rejoin)
	void updateSeekWaited();

	QNetworkAccessManager _nam;
	QString               _token;
	QString               _username;
	QString               _currentGameId;
	QString               _rejoiningGameId;   // rejoinGame(): until its gameStarted, a failure or stopStream()
	bool                  _playingAsWhite  = true;
	bool                  _lastOppDraw     = false;
	bool                  _lastOppTakeback = false;
	QString               _waitForGameId;
	QStringList           _lastMovesList;   // the moves the stream has shown
	QStringList           _sentMoves;       // ours, sent but not in the stream yet (our board has them)
	QStringList           _postsInFlight;   // ours whose POST has no answer yet (LI-06)
	LichessStream* _gameStream       = nullptr; // the Board API game stream; reconnects once gameFull came (LI-06)
	LichessStream* _eventStream      = nullptr; // BL-10: the account's events, open while there is a token
	bool           _eventStreamFresh = false;   // (re)opened: its first line marks it connected
	QVariantList   _incomingChallenges;
	QSet<QString>  _challengesResent;           // since the event stream came back
	QTimer         _challengesSettle;           // then drop the ones not resent
	QTimer         _ongoingRefresh;             // gameStart / gameFinish elsewhere: one refresh for a burst
	QTimer         _eventStreamRetry;           // Lichess refused the event stream (not a 401): try again later
	QString        _acceptingId;                // acceptingChallengeId(): its gameStart isn't "elsewhere"
	bool           _acceptedGameStarted = false; // Lichess's gameStart for it came: the row goes when the accept ends
	QNetworkReply* _probeReply       = nullptr;
	QNetworkReply* _openChallengeReply   = nullptr; // POST /api/challenge/open in flight
	QNetworkReply* _acceptReply          = nullptr; // acceptChallenge's POST in flight (then the stream)
	bool           _gameFullSeen         = false;   // the game stream has sent gameFull: the game is on
	int            _stops                = 0;       // stopAll() calls: a game refused in gameStarted stops it (LI-01A)
	bool           _hasClock             = false;   // this game has one: real time or correspondence
	bool           _correspondence       = false;
	int            _moveTimeoutMs        = 10000;
	QString        _baseUrl              = QStringLiteral("https://lichess.org");
	QString        _assetBaseUrl         = QStringLiteral("https://lichess1.org");
	QNetworkReply* _directChallengeReply = nullptr; // keepAliveStream: open while the challenge waits
	QString        _directOpponentName;             // who it was sent to
	bool           _directKeepAlive      = true;    // needsKeepAlive(): we wait while the request is open
	QString        _directChallengeId;
	bool           _directChallengeAccepted = false;
	bool           _directChallengeDone     = false; // a {"done":...} line arrived
	QTimer         _pollTimer;
	RematchInfo    _lastGame;
	LichessPlayer  _white, _black;               // the current game's players (LI-07)
	QJsonObject    _gameFull;                    // the current game's gameFull (LI-14)
	QNetworkReply* _flairReply = nullptr;        // fetchPlayerFlairs() in flight (LI-11)
	bool           _watchingForRematch = false; // keep the event stream open for the opponent's offer
	QString        _rematchOfferId;             // the opponent's offer we got; empty = none
	QVariantList   _ongoingGames;
	bool           _ongoingGamesLoading = false;
	bool           _ongoingGamesLoaded  = false;
	QString        _ongoingGamesError;
	QNetworkReply* _ongoingGamesReply = nullptr; // the refresh in flight: a newer one replaces it
	bool           _offline = false;
	// LI-09: the real-time seek.
	QVariantMap    _seek;                       // seekState()
	QNetworkReply* _seekReply = nullptr;        // POST /api/board/seek, open while the seek waits
	QSet<QString>  _seekGamesBefore;            // Your games when it began: not its game
	QString        _seekGameId;                 // LI-16: the game the last seek was paired with (its gameFull: fromSeek)
	bool           _seekClosedByLichess = false; // its request ended on Lichess's side: the fallback runs
	bool           _seekConnectionLost = false; // ended by the network, not Lichess: the fallback's message
	QNetworkReply* _seekPollReply = nullptr;    // the fallback's quiet GET /api/account/playing
	QTimer         _seekWaitedTick;             // seekWaitedSec while seeking
	int            _seekWaitedSec = 0;
	bool           _seekWaitedFixed = false;    // fixSeekWaited: screenshot scenes
	QTimer         _seekFallback;               // the fallback's end: no game came
	QTimer         _seekPoll;                   // the fallback's next look at Your games
};
