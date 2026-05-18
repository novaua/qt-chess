**Chess App**

User Profile & Multiplayer Transport

*Design Specification for Claude Code Implementation*

| Document Version | 1.0 |
| :---- | :---- |
| **Date** | 2026-05-13 |
| **Status** | Ready for Implementation |
| **Tech Stack** | Qt 6 / QML / C++ / Windows |
| **Target Agent** | Claude Code (autonomous) |

# **1\. Overview & Goals**

This document specifies two closely related features to be implemented in the Qt/QML Chess application:

* User Profile Manager — local profile with display name, avatar, and encrypted Lichess OAuth token

* Multiplayer Transport Layer — abstracted IGameTransport interface with LAN (TCP) and Lichess online implementations

Both features are designed to be implemented sequentially by Claude Code in a single session. The profile system is a prerequisite for the Lichess transport.

*Claude Code must not skip steps or reorder tasks. Each phase builds on the previous one. Build and verify after each phase.*

# **2\. Architecture Overview**

## **2.1 Module Structure**

All new code lives under src/ in the existing Qt project. No external Qt modules beyond what is already in CMakeLists.txt are required except QNetworkAccessManager (already in Qt::Network).

| Module | File(s) | Responsibility |
| :---- | :---- | :---- |
| UserProfileManager | user\_profile\_manager.h/.cpp | Load/save profile JSON, encrypt token, expose to QML |
| IGameTransport | i\_game\_transport.h | Abstract interface for all transport types |
| LanTransport | lan\_transport.h/.cpp | QTcpServer \+ QTcpSocket peer-to-peer LAN |
| LichessTransport | lichess\_transport.h/.cpp | Lichess REST \+ SSE streaming transport |
| LichessClient | lichess\_client.h/.cpp | Low-level HTTP calls to Lichess API |
| SettingsScreen.qml | qml/SettingsScreen.qml | Profile edit UI: name, avatar picker, token validation |
| LobbyScreen.qml | qml/LobbyScreen.qml | LAN host/join \+ Lichess seek/challenge UI |

## **2.2 Dependency Graph**

QML Layer (SettingsScreen, LobbyScreen, BoardView)

    |

    v

GameController  \<--\>  UserProfileManager

    |

    v

IGameTransport (interface)

    |--- LanTransport      (QTcpServer / QTcpSocket)

    |--- LichessTransport  (LichessClient \-\> QNetworkAccessManager)

    |--- LocalTransport    (Stockfish process, existing)

# **3\. UserProfileManager**

## **3.1 Data Model**

The profile is stored as a single JSON file. Location is resolved at runtime using QStandardPaths::AppDataLocation. The directory is created if it does not exist.

File path: \<AppData\>/ChessApp/profile.json

| JSON Field | C++ Type | Default | Notes |
| :---- | :---- | :---- | :---- |
| displayName | QString | "Player" | UTF-8, 1–32 chars, validated on save |
| avatarIndex | int | 0 | 0–11, selects built-in SVG avatar |
| lichessUsername | QString | "" | Populated after successful token validation |
| lichessTokenEncrypted | QString | "" | Base64-encoded DPAPI ciphertext |
| lichessAvatarUrl | QString | "" | Cached from /api/account response |
| gamesPlayed | int | 0 | Incremented by GameController on game end |
| gamesWon | int | 0 | Incremented by GameController on win |
| gamesDrawn | int | 0 | Incremented by GameController on draw |

## **3.2 C++ Class Interface**

class UserProfileManager : public QObject {

    Q\_OBJECT

    Q\_PROPERTY(QString displayName READ displayName

               WRITE setDisplayName NOTIFY profileChanged)

    Q\_PROPERTY(int avatarIndex READ avatarIndex

               WRITE setAvatarIndex NOTIFY profileChanged)

    Q\_PROPERTY(QString lichessUsername READ lichessUsername

               NOTIFY profileChanged)

    Q\_PROPERTY(bool lichessConnected READ lichessConnected

               NOTIFY profileChanged)

    Q\_PROPERTY(int gamesPlayed READ gamesPlayed NOTIFY profileChanged)

    Q\_PROPERTY(int gamesWon READ gamesWon NOTIFY profileChanged)

    Q\_PROPERTY(int gamesDrawn READ gamesDrawn NOTIFY profileChanged)

public:

    Q\_INVOKABLE void load();

    Q\_INVOKABLE void save();

    Q\_INVOKABLE void validateAndSaveLichessToken(const QString& token);

    Q\_INVOKABLE void clearLichessToken();

    QString decryptedToken() const;  // Used internally by LichessClient

signals:

    void profileChanged();

    void lichessValidationResult(bool success, QString username);

};

*Register as singleton context property: engine.rootContext()-\>setContextProperty("userProfile", \&mgr);*

## **3.3 Token Encryption (Windows DPAPI)**

Use CryptProtectData / CryptUnprotectData from \<wincrypt.h\>. No third-party crypto library required. Wrap in two static helpers:

// Returns base64-encoded encrypted bytes, or empty string on failure

static QString encryptToken(const QString& plaintext);

// Returns decrypted plaintext, or empty string on failure

static QString decryptToken(const QString& base64Ciphertext);

* Call encryptToken() before writing lichessTokenEncrypted to JSON

* Call decryptToken() when GameController or LichessClient needs the live token

* Never log or expose the decrypted token value

* If CryptUnprotectData fails (different Windows user, moved machine), treat as no token — call clearLichessToken()

## **3.4 Lichess Token Validation Flow**

validateAndSaveLichessToken() must perform the following steps synchronously via QNetworkAccessManager with event loop (or async with a local QEventLoop):

1. Send GET https://lichess.org/api/account with header Authorization: Bearer \<token\>

2. If HTTP 200: parse JSON, extract username and optional title fields

3. Call encryptToken(token), store result in lichessTokenEncrypted

4. Store username in lichessUsername

5. Call save()

6. Emit lichessValidationResult(true, username)

7. If HTTP 401 or network error: emit lichessValidationResult(false, "") — do NOT save token

# **4\. IGameTransport Interface**

All multiplayer and AI game modes communicate through a single abstract interface. GameController holds a pointer to the active IGameTransport and never accesses concrete implementations directly.

class IGameTransport : public QObject {

    Q\_OBJECT

public:

    virtual \~IGameTransport() \= default;

    virtual void sendMove(const QString& uciMove) \= 0;

    virtual void sendResign() \= 0;

    virtual void sendDrawOffer() \= 0;

    virtual void sendDrawResponse(bool accepted) \= 0;

    virtual void sendChatMessage(const QString& text) \= 0;

    virtual void disconnect() \= 0;

signals:

    void moveReceived(QString uciMove);

    void gameStarted(QString startFen, bool playingAsWhite);

    void gameEnded(QString result);  // "white" | "black" | "draw"

    void drawOffered();

    void chatReceived(QString sender, QString text);

    void connectionLost(QString reason);

    void opponentInfo(QString name, QString avatarUrl);

};

# **5\. LanTransport**

## **5.1 Protocol**

Messages are newline-delimited JSON sent over a plain QTcpSocket connection. Each message is a single UTF-8 JSON line terminated by \\n.

| type | Payload Fields | Description |
| :---- | :---- | :---- |
| hello | name: string | First message after connect — sends display name |
| start | fen: string, color: "white"|"black" | Host sends after hello exchange to begin game |
| move | uci: string | A UCI move string e.g. e2e4, e7e8q |
| resign | (none) | Sender forfeits the game |
| draw\_offer | (none) | Sender offers a draw |
| draw\_accept | (none) | Receiver accepts the draw offer |
| draw\_decline | (none) | Receiver declines the draw offer |
| chat | text: string | In-game chat message, max 500 chars |
| ping | (none) | Keep-alive every 10s; other side replies pong |
| pong | (none) | Keep-alive reply |

## **5.2 C++ Class**

class LanTransport : public IGameTransport {

    Q\_OBJECT

public:

    // Host mode

    Q\_INVOKABLE void startHosting(quint16 port \= 5555);

    Q\_INVOKABLE void stopHosting();

    Q\_INVOKABLE QString localAddress() const; // Returns LAN IP for display

    // Client mode

    Q\_INVOKABLE void connectToHost(const QString& ip, quint16 port \= 5555);

    // IGameTransport overrides

    void sendMove(const QString& uciMove) override;

    void sendResign() override;

    void sendDrawOffer() override;

    void sendDrawResponse(bool accepted) override;

    void sendChatMessage(const QString& text) override;

    void disconnect() override;

signals:

    void clientConnected(QString opponentName);

    void hostingStarted(QString ip, quint16 port);

    void connectionFailed(QString reason);

};

## **5.3 Implementation Notes**

* Use QTcpServer for host mode; accept exactly one connection then stop listening

* Use QDataStream or raw readLine() — prefer readLine() since messages are newline-delimited

* localAddress(): iterate QNetworkInterface::allAddresses(), return first IPv4 non-loopback address

* Host assigns colors: host plays White, client plays Black (for v1 — color negotiation is a future feature)

* Ping/pong timer: QTimer every 10000ms; if no pong within 15s, emit connectionLost("timeout")

* On disconnect: emit connectionLost("opponent disconnected") and set socket to nullptr

# **6\. LichessTransport & LichessClient**

## **6.1 LichessClient — Low-Level HTTP**

LichessClient wraps QNetworkAccessManager and handles all Lichess API calls. It is used by both LichessTransport and UserProfileManager (for token validation).

class LichessClient : public QObject {

    Q\_OBJECT

public:

    explicit LichessClient(UserProfileManager\* profile, QObject\* parent \= nullptr);

    void getAccount();                         // GET /api/account

    void seekGame(const QString& timeControl); // POST /api/seek

    void streamGame(const QString& gameId);   // GET /api/board/game/stream/{id}

    void postMove(const QString& gameId,

                  const QString& uciMove);    // POST /api/board/game/{id}/move/{uci}

    void resign(const QString& gameId);       // POST /api/board/game/{id}/resign

    void offerDraw(const QString& gameId);    // POST /api/board/game/{id}/draw/yes

    void declineDraw(const QString& gameId);  // POST /api/board/game/{id}/draw/no

    void sendChat(const QString& gameId,

                  const QString& text);       // POST /api/board/game/{id}/chat

signals:

    void accountReceived(QJsonObject account);

    void gameStarted(QString gameId, QString fen, QString color);

    void gameEventReceived(QJsonObject event);  // Raw NDJSON event

    void seekFailed(QString reason);

    void networkError(QString reason);

};

*SSE stream (streamGame): use QNetworkReply::readyRead \+ readLine() in a loop. Set Http2AllowedAttribute to false. Parse each non-empty line as QJsonDocument.*

## **6.2 Lichess Game Event Types**

The stream emits NDJSON events. LichessTransport must handle these event types from the gameFull and gameState objects:

| type value | Key Fields | Action in LichessTransport |
| :---- | :---- | :---- |
| gameFull | white.name, black.name, initialFen, state.moves | Emit gameStarted; determine playingAsWhite by comparing white.id to lichessUsername |
| gameState | moves (space-sep UCI), status, winner | Diff moves list to find new move; emit moveReceived; if status \!= started emit gameEnded |
| chatLine | username, text, room | If room \== player emit chatReceived |

## **6.3 LichessTransport Class**

class LichessTransport : public IGameTransport {

    Q\_OBJECT

public:

    explicit LichessTransport(LichessClient\* client, QObject\* parent \= nullptr);

    Q\_INVOKABLE void seekGame(const QString& timeControl \= "10+0");

    Q\_INVOKABLE void cancelSeek();

    // IGameTransport overrides

    void sendMove(const QString& uciMove) override;

    void sendResign() override;

    void sendDrawOffer() override;

    void sendDrawResponse(bool accepted) override;

    void sendChatMessage(const QString& text) override;

    void disconnect() override;

signals:

    void seekingOpponent();

    void seekCancelled();

};

# **7\. QML Screens**

## **7.1 SettingsScreen.qml**

This screen is accessed from the main menu. It binds to the userProfile context property.

| Section | Contents |
| :---- | :---- |
| Avatar Row | GridView showing 12 built-in chess-piece SVG avatars (qrc:/avatars/). Tapping selects and updates avatarIndex via userProfile.avatarIndex. |
| Display Name | TextField bound to userProfile.displayName. Max 32 chars. Save button calls userProfile.save(). |
| Lichess Section | TextInput for token (echoMode: Password). Validate button calls userProfile.validateAndSaveLichessToken(). Status row shows green check \+ username on success, red X on failure. Clear button calls userProfile.clearLichessToken(). |
| Stats Row | Read-only display of gamesPlayed, gamesWon, gamesDrawn. Non-editable. |

*Connections { target: userProfile; function onLichessValidationResult(success, username) { ... } } — show inline success/error, do not use dialog boxes.*

## **7.2 LobbyScreen.qml**

The lobby presents three multiplayer modes as tab buttons: vs Computer (existing), LAN, and Lichess Online.

| Tab | UI Elements |
| :---- | :---- |
| LAN | Two sub-buttons: Host Game / Join Game. Host shows local IP \+ port. Join shows IP TextField \+ Connect button. Both call methods on LanTransport instance. |
| Lichess | If \!userProfile.lichessConnected: show warning text and link to Settings. If connected: Time control dropdown (1+0, 3+2, 5+0, 10+0). Seek button calls lanTransport.seekGame(). Animated spinner during seek with Cancel button. |

# **8\. Implementation Task List**

Claude Code must execute tasks in this exact order. Do not proceed to the next task until the current one compiles and runs without errors.

| \# | Phase | Task | Verify |
| :---- | :---- | :---- | :---- |
| 1 | Profile | Create user\_profile\_manager.h with Q\_PROPERTY declarations and signal signatures | File compiles |
| 2 | Profile | Implement load() and save() using QJsonDocument \+ QStandardPaths | Profile saves/loads from AppData |
| 3 | Profile | Implement DPAPI encryptToken() / decryptToken() static helpers on Windows | Round-trip test in main() |
| 4 | Profile | Implement validateAndSaveLichessToken() using QNetworkAccessManager, parse username | Test with real PAT token |
| 5 | Profile | Register UserProfileManager as context property in main.cpp | QML can read displayName |
| 6 | QML | Create SettingsScreen.qml with avatar grid (placeholder rectangles), name field, Lichess section | Screen renders without errors |
| 7 | QML | Add 12 built-in avatar SVGs to qrc, wire GridView to avatarIndex property | Selecting avatar persists on save/reload |
| 8 | Transport | Create i\_game\_transport.h with pure virtual interface and all signals | Compiles; no implementation yet |
| 9 | LAN | Implement LanTransport host mode: QTcpServer, accept one client, hello/start exchange | Two instances can connect on localhost |
| 10 | LAN | Implement LanTransport client mode: connect, send hello, receive start | Client receives gameStarted signal |
| 11 | LAN | Implement sendMove / moveReceived, ping/pong keepalive, disconnect handling | Moves relay correctly; disconnect emits signal |
| 12 | QML | Create LobbyScreen.qml LAN tab — host shows IP, join shows connect form | Host/join flow reaches gameStarted |
| 13 | Lichess | Implement LichessClient: getAccount, bearer header injection from UserProfileManager | GET /api/account returns username |
| 14 | Lichess | Implement LichessClient: seekGame (POST /api/seek) and streamGame (SSE readLine loop) | Seek creates a game; stream delivers events |
| 15 | Lichess | Implement LichessClient: postMove, resign, offerDraw, declineDraw, sendChat | All POST calls return 200 |
| 16 | Lichess | Implement LichessTransport wrapping LichessClient, map NDJSON events to IGameTransport signals | Full game playable via Lichess API |
| 17 | QML | Add Lichess tab to LobbyScreen.qml — time control, seek spinner, cancel | Full seek-to-game flow in UI |
| 18 | Wire | GameController selects transport based on lobby choice; BoardView works for all three modes | All 3 modes play a complete game |

# **9\. Constraints & Rules for Claude Code**

## **9.1 Build Rules**

* Run cmake \--build build \--parallel after every phase (every 2-3 tasks). Fix all errors before continuing.

* Do not add any Qt modules to CMakeLists.txt beyond Qt::Network and Qt::Widgets (already present).

* All new .cpp files must be added to CMakeLists.txt target\_sources() before building.

## **9.2 Code Style**

* Header files use \#pragma once

* No raw new/delete — use QObject parent ownership or smart pointers

* Signals are always in the signals: section, never emitted from outside the class

* QML files: use Connections { } blocks for C++ signal handling, never direct property binding on signals

* All QML-exposed methods use Q\_INVOKABLE, not slots

## **9.3 Security**

* The decryptedToken() method must never be called from QML — only from C++ internal code

* Do not log token values, even partially, in any debug output

* Lichess token input field must use echoMode: TextInput.Password in QML

## **9.4 Error Handling**

* All QNetworkReply::finished connections must check for error() before reading data

* JSON parsing: always check QJsonDocument::isNull() before accessing fields

* LAN: handle QTcpSocket::errorOccurred signal; emit connectionLost with the error string

* DPAPI failure: do not crash — treat as no token and call clearLichessToken()

# **10\. Out of Scope (Future Work)**

The following are explicitly excluded from this implementation session and must not be implemented unless requested:

* Color negotiation in LAN games (host is always White for v1)

* Lichess tournament or puzzle integration

* Multiple local profiles (single profile only)

* Local image file avatar upload (built-in SVGs only)

* Chess clock synchronization for LAN games

* Spectator mode

* Cross-platform DPAPI equivalent (Linux/macOS)

***End of Specification***