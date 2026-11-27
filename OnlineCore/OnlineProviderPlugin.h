#pragma once
#include "onlinecore_global.h"
#include <QtPlugin>
#include <QString>

class OnlineServer;

// LI-18: an online server as a Qt plugin: a DLL in the app's online\ folder (LichessOnline.dll, …) whose
// class implements this and declares
//   Q_PLUGIN_METADATA(IID OnlineProviderPlugin_iid FILE "<server>.json")
//   Q_INTERFACES(OnlineProviderPlugin)
// Its JSON: { "id": "lichess", "name": "Lichess", "order": 10, "coreVersion": 1 }. OnlineHub reads it before it
// loads the DLL, and skips a plugin of another coreVersion (OnlineHub::CoreVersion) or a second one of an id.
class OnlineProviderPlugin
{
public:
	virtual ~OnlineProviderPlugin() = default;
	// The server at `address` ("": the plugin's own default, e.g. https://lichess.org); a new object, the
	// caller's (OnlineHub keeps it in a std::shared_ptr).
	virtual OnlineServer* createServer(const QString& address) const = 0;
};

#define OnlineProviderPlugin_iid "org.chessplusplus.OnlineProvider/1"
Q_DECLARE_INTERFACE(OnlineProviderPlugin, OnlineProviderPlugin_iid)
