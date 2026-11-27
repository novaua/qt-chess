#include "stdafx.h"
#include "OnlineHub.h"
#include "OnlineClient.h"
#include "OnlineProviderPlugin.h"

#include <QDebug>
#include <QDir>
#include <QPluginLoader>
#include <algorithm>

OnlineHub::OnlineHub(QObject* parent)
	: QObject(parent)
{
}

OnlineHub::~OnlineHub() = default;

QString OnlineHub::refusal(const QJsonObject& metadata) const
{
	const QString id = metadata.value(QStringLiteral("id")).toString();
	if (id.isEmpty())
		return QStringLiteral("no id");
	const int version = metadata.value(QStringLiteral("coreVersion")).toInt();
	if (version != CoreVersion)
		return QStringLiteral("coreVersion %1, the app's is %2").arg(version).arg(CoreVersion);
	if (client(id))
		return QStringLiteral("a server with the id \"%1\" is there already").arg(id);
	return {};
}

int OnlineHub::loadPlugins(const QString& dir)
{
	int added = 0;
	const QDir folder(dir);
	const QStringList files = folder.entryList({ QStringLiteral("*.dll") }, QDir::Files, QDir::Name);
	for (const QString& file : files) {
		QPluginLoader loader(folder.absoluteFilePath(file));
		const QJsonObject meta = loader.metaData();
		if (meta.value(QStringLiteral("IID")).toString() != QLatin1String(OnlineProviderPlugin_iid)) {
			qWarning() << "OnlineHub: not a server plugin, skipped:" << file;
			continue;
		}
		const QJsonObject metadata = meta.value(QStringLiteral("MetaData")).toObject();
		const QString why = refusal(metadata);
		if (!why.isEmpty()) {
			qWarning() << "OnlineHub: plugin skipped:" << file << "|" << why;
			continue;
		}
		QObject* instance = loader.instance(); // the DLL stays loaded: its server's code is in it
		const auto* plugin = qobject_cast<OnlineProviderPlugin*>(instance);
		if (!plugin) {
			qWarning() << "OnlineHub: plugin didn't load:" << file << "|" << loader.errorString();
			continue;
		}
		if (addPlugin(plugin, metadata)) {
			qInfo() << "OnlineHub: server" << metadata.value(QStringLiteral("id")).toString() << "from" << file;
			++added;
		}
	}
	return added;
}

bool OnlineHub::addPlugin(const OnlineProviderPlugin* plugin, const QJsonObject& metadata)
{
	const QString why = refusal(metadata);
	if (!plugin || !why.isEmpty()) {
		qWarning() << "OnlineHub: plugin skipped |" << (plugin ? why : QStringLiteral("none"));
		return false;
	}
	std::shared_ptr<OnlineServer> server(plugin->createServer({}));
	if (!server) {
		qWarning() << "OnlineHub: plugin made no server:" << metadata.value(QStringLiteral("id")).toString();
		return false;
	}
	OnlineClient* const before = current();
	Entry entry;
	entry.id = metadata.value(QStringLiteral("id")).toString();
	entry.name = metadata.value(QStringLiteral("name")).toString(entry.id);
	entry.order = metadata.value(QStringLiteral("order")).toInt();
	entry.client = new OnlineClient(server, this);
	const auto at = std::find_if(_clients.begin(), _clients.end(), [&](const Entry& e) {
		return e.order > entry.order || (e.order == entry.order && e.id > entry.id);
	});
	_clients.insert(at, entry);
	emit providersChanged();
	if (current() != before)
		emit currentChanged();
	return true;
}

OnlineClient* OnlineHub::client(const QString& serverId) const
{
	for (const Entry& e : _clients)
		if (e.id == serverId)
			return e.client;
	return nullptr;
}

QList<OnlineClient*> OnlineHub::clients() const
{
	QList<OnlineClient*> list;
	for (const Entry& e : _clients)
		list.append(e.client);
	return list;
}

QVariantList OnlineHub::providers() const
{
	QVariantList list;
	for (const Entry& e : _clients)
		list.append(QVariantMap{ { QStringLiteral("id"), e.id }, { QStringLiteral("name"), e.name } });
	return list;
}
