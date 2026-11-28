#pragma once
#include "LichessServer.h"
#include "OnlineProviderPlugin.h"
#include <QObject>

// LI-18: LichessOnline.dll as the app's plugin for lichess.org (OnlineHub loads it from the folder "online"
// beside the app). Its metadata: lichess.json.
class LichessPlugin : public QObject, public OnlineProviderPlugin
{
	Q_OBJECT
	Q_PLUGIN_METADATA(IID OnlineProviderPlugin_iid FILE "lichess.json")
	Q_INTERFACES(OnlineProviderPlugin)

public:
	// Lichess has one address: `address` is ignored.
	OnlineServer* createServer(const QString&) const override { return new LichessServer; }
};
