#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

// What Settings shows about the app: version and build, the data and log folders, and the chess
// engine (found by a short "uci" run of stockfish.exe, the way UciConnector finds it). Also the
// clipboard for QML, which has none (LI-10: a Lichess game's link).
class AppInfo : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString version    READ version    CONSTANT) // "0.1.2"
    Q_PROPERTY(QString buildInfo  READ buildInfo  CONSTANT) // "Built Oct 29, 2026 · Qt 6.10.2" (+ " · Debug")
    Q_PROPERTY(QString qtVersion  READ qtVersion  CONSTANT)
    Q_PROPERTY(QUrl    dataFolder READ dataFolder CONSTANT)
    Q_PROPERTY(QUrl    logFolder  READ logFolder  CONSTANT)
    // Windows Settings → Accessibility → Visual effects → Animation effects is off (read at start).
    Q_PROPERTY(bool    windowsAnimationsOff READ windowsAnimationsOff CONSTANT)
    // "checking" until probeEngine() is done, then "ok" or "missing" (none found, or it didn't answer).
    Q_PROPERTY(QString engineState READ engineState NOTIFY engineChanged)
    // An engine answered (BL-25): what the analysis needs. Not while it is still being looked for.
    Q_PROPERTY(bool    engineFound READ engineFound NOTIFY engineChanged)
    Q_PROPERTY(QString engineName  READ engineName  NOTIFY engineChanged) // "Stockfish 17.1"
    Q_PROPERTY(QString enginePath  READ enginePath  NOTIFY engineChanged)

public:
    explicit AppInfo(QObject* parent = nullptr);

    static QString version();
    static QString buildInfo();
    static QString qtVersion();
    static QUrl    dataFolder();
    static QUrl    logFolder();
    static QString logFolderPath();
    static bool    windowsAnimationsOff();

    QString engineState() const { return _engineState; }
    bool    engineFound() const { return _engineState == QStringLiteral("ok"); }
    QString engineName()  const { return _engineName; }
    QString enginePath()  const { return _enginePath; }

    // Starts the engine with "uci" for its name. AppServices::start() asks first (BL-25: the Analyse
    // buttons need it, not only Settings). Once found, that's it for the run; while it is "missing",
    // a later call looks again (Settings, a game page's moves panel): a first start that took longer
    // than the 5 s (an antivirus scan of a new stockfish.exe) mustn't turn the analysis off for the
    // session. The state stays "missing" while it looks again; one probe at a time.
    Q_INVOKABLE void probeEngine();

    // LI-10: text onto the clipboard (QML has none of its own), e.g. a Lichess game's link; `message`
    // ("Link copied") comes back with copied() for the window's short message ("": none). ok: false when
    // the clipboard doesn't hold the text after (Windows' clipboard held open by another program).
    Q_INVOKABLE void copyToClipboard(const QString& text, const QString& message = {});

signals:
    void engineChanged();
    void copied(const QString& message, bool ok);

private:
    void probeNext();
    void setEngine(const QString& name); // "" = none answered

    QString     _engineState = QStringLiteral("checking");
    QString     _engineName;
    QString     _enginePath;
    QStringList _candidates;   // not tried yet
    bool        _probing = false;
};
