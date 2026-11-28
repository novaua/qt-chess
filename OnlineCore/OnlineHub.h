#pragma once
#include "onlinecore_global.h"
#include "OnlineClient.h" // complete: a Q_PROPERTY's type
#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QVariantList>

class OnlineProviderPlugin;

// LI-18: the online servers the app has: one plugin each (OnlineProviderPlugin), loaded from the folder
// "online" beside the app, and a client for each (OnlineClient). QML singleton `Online`. Without a plugin there is no client:
// `available` is false and the online parts of the app are hidden, local play goes on.
class ONLINECORE_EXPORT OnlineHub : public QObject
{
	Q_OBJECT
	// The client the Online page and the rest of the app use (the first server by its plugin's "order");
	// null without one. Step 5 lets the Online page switch it.
	Q_PROPERTY(OnlineClient* current READ current NOTIFY currentChanged)
	Q_PROPERTY(bool available READ available NOTIFY currentChanged)
	// The loaded servers, in order: { id, name } each.
	Q_PROPERTY(QVariantList providers READ providers NOTIFY providersChanged)

public:
	// What a plugin's "coreVersion" must be: raised when OnlineServer or OnlineProviderPlugin change.
	static constexpr int CoreVersion = 1;

	explicit OnlineHub(QObject* parent = nullptr);
	~OnlineHub() override;

	// Every plugin in `dir` (*.dll): its metadata first, then the DLL. Returns how many were added; what is
	// skipped is logged (another coreVersion, an id already there, no id, a DLL that doesn't load).
	int loadPlugins(const QString& dir);
	// One from code (the tests'), with the metadata its JSON would have; false when skipped, as above.
	bool addPlugin(const OnlineProviderPlugin* plugin, const QJsonObject& metadata);

	OnlineClient* current() const { return _clients.isEmpty() ? nullptr : _clients.first().client; }
	bool available() const { return current() != nullptr; }
	// The client of the server with that id ("lichess"); null when its plugin isn't loaded.
	Q_INVOKABLE OnlineClient* client(const QString& serverId) const;
	QList<OnlineClient*> clients() const;
	QVariantList providers() const;

signals:
	void currentChanged();
	void providersChanged();

private:
	struct Entry
	{
		QString       id;
		QString       name;
		int           order = 0;
		OnlineClient* client = nullptr; // a child of the hub
	};
	// The reason a plugin with this metadata is skipped; "" when it is taken.
	QString refusal(const QJsonObject& metadata) const;

	QList<Entry> _clients; // by order, then id
};
