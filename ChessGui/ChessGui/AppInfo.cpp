#include "stdafx.h"
#include "AppInfo.h"
#include "AppConfig.h"
#include "UciConnector.h"

#include <QDate>
#include <QDebug>
#include <QDir>
#include <QLocale>
#include <QProcess>
#include <QTimer>

#include <windows.h>

// Set by Directory.Build.props (the release workflow stamps the git tag); a Qt Creator build has none.
#ifndef APP_VERSION_MAJOR
#define APP_VERSION_MAJOR 0
#define APP_VERSION_MINOR 0
#define APP_VERSION_PATCH 0
#endif

AppInfo::AppInfo(QObject* parent)
    : QObject(parent)
{
}

QString AppInfo::version()
{
    return QStringLiteral("%1.%2.%3").arg(APP_VERSION_MAJOR).arg(APP_VERSION_MINOR).arg(APP_VERSION_PATCH);
}

QString AppInfo::buildInfo()
{
    // __DATE__ is "Oct  9 2026" (the day this file was compiled).
    const QDate built = QLocale::c().toDate(QString::fromLatin1(__DATE__).simplified(), QStringLiteral("MMM d yyyy"));
    QString info = QStringLiteral("Built %1 · Qt %2")
        .arg(QLocale::c().toString(built, QStringLiteral("MMM d, yyyy")), qtVersion());
#ifdef _DEBUG
    info += QStringLiteral(" · Debug");
#endif
    return info;
}

QString AppInfo::qtVersion()
{
    return QString::fromLatin1(qVersion());
}

QUrl AppInfo::dataFolder()
{
    return QUrl::fromLocalFile(AppConfig::appDataDir());
}

QString AppInfo::logFolderPath()
{
    return AppConfig::appDataDir() + QStringLiteral("/logs");
}

bool AppInfo::windowsAnimationsOff()
{
    static const bool off = [] {
        BOOL animations = TRUE;
        return SystemParametersInfoW(SPI_GETCLIENTAREAANIMATION, 0, &animations, 0) && !animations;
    }();
    return off;
}

QUrl AppInfo::logFolder()
{
    QDir().mkpath(logFolderPath());
    return QUrl::fromLocalFile(logFolderPath());
}

void AppInfo::probeEngine()
{
    if (_probed)
        return;
    _probed = true;
    for (const std::string& path : UciConnector::EngineCandidates())
        _candidates.append(QString::fromStdString(path));
    probeNext();
}

// Like UciConnector::Init(): a candidate that doesn't start or answer gives way to the next one.
void AppInfo::probeNext()
{
    if (_candidates.isEmpty()) {
        qWarning() << "AppInfo: no UCI engine answered" << _enginePath;
        setEngine({});
        return;
    }
    const QString path = _candidates.takeFirst();
    _enginePath = QDir::toNativeSeparators(path);

    auto* probe = new QProcess(this);
    auto buffer = std::make_shared<QByteArray>();
    auto name = std::make_shared<QString>();
    auto done = std::make_shared<bool>(false);
    // Once per run: an answer is the engine, anything else tries the next candidate. The process is
    // left to quit on its own ("quit" was sent) or killed if it never answered.
    auto finish = [this, probe, done](const QString& engineName) {
        if (*done)
            return;
        *done = true;
        connect(probe, &QProcess::finished, probe, &QObject::deleteLater);
        if (probe->state() == QProcess::NotRunning)
            probe->deleteLater();
        else if (engineName.isEmpty())
            probe->kill();
        else
            QTimer::singleShot(2000, probe, [probe]() { probe->kill(); });
        if (engineName.isEmpty())
            probeNext();
        else
            setEngine(engineName);
    };

    connect(probe, &QProcess::readyReadStandardOutput, probe, [probe, buffer, name, finish]() {
        buffer->append(probe->readAllStandardOutput());
        qsizetype end;
        while ((end = buffer->indexOf('\n')) >= 0) {
            const QString line = QString::fromUtf8(buffer->left(end)).trimmed();
            buffer->remove(0, end + 1);
            if (line.startsWith(QStringLiteral("id name ")))
                *name = line.mid(8);
            else if (line == QStringLiteral("uciok")) {
                probe->write("quit\n");
                finish(name->isEmpty() ? QStringLiteral("UCI engine") : *name);
                return;
            }
        }
    });
    connect(probe, &QProcess::errorOccurred, probe, [finish](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finish({});
    });
    connect(probe, &QProcess::finished, probe, [finish]() { finish({}); });
    // Stockfish answers in milliseconds; don't wait forever on something that isn't a UCI engine.
    QTimer::singleShot(5000, probe, [finish]() { finish({}); });

    probe->start(path, {});
    probe->write("uci\n");
}

void AppInfo::setEngine(const QString& name)
{
    _engineName = name;
    _engineState = name.isEmpty() ? QStringLiteral("missing") : QStringLiteral("ok");
    emit engineChanged();
}
