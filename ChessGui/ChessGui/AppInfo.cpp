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

    const std::vector<std::string> candidates = UciConnector::EngineCandidates();
    if (candidates.empty()) {
        finishProbe({});
        return;
    }
    _enginePath = QDir::toNativeSeparators(QString::fromStdString(candidates.front()));

    _probe = new QProcess(this);
    auto buffer = std::make_shared<QByteArray>();
    auto name = std::make_shared<QString>();
    connect(_probe, &QProcess::readyReadStandardOutput, this, [this, buffer, name]() {
        buffer->append(_probe->readAllStandardOutput());
        qsizetype end;
        while ((end = buffer->indexOf('\n')) >= 0) {
            const QString line = QString::fromUtf8(buffer->left(end)).trimmed();
            buffer->remove(0, end + 1);
            if (line.startsWith(QStringLiteral("id name ")))
                *name = line.mid(8);
            else if (line == QStringLiteral("uciok")) {
                _probe->write("quit\n");
                finishProbe(*name);
                return;
            }
        }
    });
    connect(_probe, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart)
            finishProbe({});
    });
    connect(_probe, &QProcess::finished, this, [this]() { finishProbe({}); });
    // Stockfish answers in milliseconds; don't wait forever on something that isn't a UCI engine.
    QTimer::singleShot(5000, this, [this]() { finishProbe({}); });

    _probe->start(QString::fromStdString(candidates.front()), {});
    _probe->write("uci\n");
}

// The first call decides; the process is left to quit on its own (or killed if it never answered).
void AppInfo::finishProbe(const QString& name)
{
    if (_engineState != QStringLiteral("checking"))
        return;
    _engineName = name;
    _engineState = name.isEmpty() ? QStringLiteral("missing") : QStringLiteral("ok");
    if (name.isEmpty())
        qWarning() << "AppInfo: no UCI engine answered" << _enginePath;
    if (_probe) {
        QProcess* probe = _probe;
        _probe = nullptr;
        probe->disconnect(this);
        connect(probe, &QProcess::finished, probe, &QObject::deleteLater);
        if (probe->state() == QProcess::NotRunning)
            probe->deleteLater();
        else if (name.isEmpty())
            probe->kill();
        else
            QTimer::singleShot(2000, probe, [probe]() { probe->kill(); }); // "quit" was sent
    }
    emit engineChanged();
}
