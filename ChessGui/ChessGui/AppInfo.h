#pragma once
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>

// What Settings shows about the app: version and build, the data and log folders, and the chess
// engine (found by a short "uci" run of stockfish.exe, the way UciConnector finds it).
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
    QString engineName()  const { return _engineName; }
    QString enginePath()  const { return _enginePath; }

    // Starts the engine once with "uci" for its name; once per run (it doesn't change while the app runs).
    Q_INVOKABLE void probeEngine();

signals:
    void engineChanged();

private:
    void probeNext();
    void setEngine(const QString& name); // "" = none answered

    QString     _engineState = QStringLiteral("checking");
    QString     _engineName;
    QString     _enginePath;
    QStringList _candidates;   // not tried yet
    bool        _probed = false;
};
