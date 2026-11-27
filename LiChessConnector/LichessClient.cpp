#include "stdafx.h"
#include "LichessClient.h"
#include "LichessServer.h"

LichessClient::LichessClient(QObject* parent)
	: OnlineClient(std::make_shared<LichessServer>(), parent)
{
}
