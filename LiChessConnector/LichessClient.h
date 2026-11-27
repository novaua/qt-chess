#pragma once
#include "lichessconnector_global.h"
#include "OnlineClient.h"

// LI-18: an OnlineClient on lichess.org (a LichessServer of its own). The app's, until OnlineHub makes the
// clients (LI-18 step 3), and the Lichess tests'.
class LICHESSCONNECTOR_EXPORT LichessClient : public OnlineClient
{
public:
	explicit LichessClient(QObject* parent = nullptr);
};
