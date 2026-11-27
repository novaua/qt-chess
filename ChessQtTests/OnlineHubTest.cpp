#include <QJsonObject>
#include <QTemporaryDir>
#include <CppUnitTest.h>
#include "OnlineHub.h"
#include "OnlineProviderPlugin.h"
#include "OnlineServer.h"
#include "TestSupport.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// LI-18: OnlineHub takes the online servers' plugins: one client each, the first by "order" the current one;
// a plugin of another coreVersion, without an id, or with an id already there is skipped.
namespace ChessQtTests
{
	// A server that is only its id: nothing here goes to the network.
	class FakeServer : public OnlineServer
	{
	public:
		explicit FakeServer(const QString& id) : _id(id) {}
		QString id() const override { return _id; }
		QString name() const override { return _id.toUpper(); }
		QString apiUrl() const override { return QStringLiteral("http://127.0.0.1:1"); }
		QString assetUrl() const override { return apiUrl(); }
		QString webUrl() const override { return apiUrl(); }
		QString gameUrl(const QString& gameId, const QString&) const override { return webUrl() + '/' + gameId; }
		QString gameIdFromUrl(const QString&) const override { return {}; }
		QString colorFromUrl(const QString&) const override { return {}; }
		bool playsVariant(const QString&) const override { return true; }
		bool canPlay(const QString&, const QString&, const QString&) const override { return true; }
		QString seekRateLimitMessage() const override { return {}; }
		OnlineAuth* createAuth(QObject*) const override { return nullptr; }

	private:
		QString _id;
	};

	class FakePlugin : public OnlineProviderPlugin
	{
	public:
		explicit FakePlugin(const QString& id) : _id(id) {}
		OnlineServer* createServer(const QString&) const override { return new FakeServer(_id); }

	private:
		QString _id;
	};

	static QJsonObject metadata(const QString& id, int order = 10, int coreVersion = OnlineHub::CoreVersion)
	{
		return QJsonObject{ { "id", id }, { "name", id.toUpper() }, { "order", order }, { "coreVersion", coreVersion } };
	}

	TEST_CLASS(OnlineHubTests)
	{
	public:
		TEST_METHOD(NoPlugin_NothingAvailable)
		{
			OnlineHub hub;
			Assert::IsFalse(hub.available());
			Assert::IsNull(hub.current());
			Assert::IsNull(hub.client("lichess"));
			Assert::AreEqual(0, (int)hub.providers().size());
		}

		TEST_METHOD(Plugin_ItsClientIsCurrent)
		{
			OnlineHub hub;
			FakePlugin plugin("alpha");
			int changes = 0;
			QObject::connect(&hub, &OnlineHub::currentChanged, [&]() { ++changes; });
			Assert::IsTrue(hub.addPlugin(&plugin, metadata("alpha")));
			Assert::IsTrue(hub.available());
			Assert::IsNotNull(hub.current());
			Assert::IsTrue(hub.current() == hub.client("alpha"));
			Assert::AreEqual(QString("alpha"), hub.current()->server().id());
			Assert::AreEqual(1, changes);
			const QVariantMap provider = hub.providers().first().toMap();
			Assert::AreEqual(QString("alpha"), provider.value("id").toString());
			Assert::AreEqual(QString("ALPHA"), provider.value("name").toString());
		}

		TEST_METHOD(OtherCoreVersion_Skipped)
		{
			OnlineHub hub;
			FakePlugin plugin("alpha");
			Assert::IsFalse(hub.addPlugin(&plugin, metadata("alpha", 10, OnlineHub::CoreVersion + 1)));
			Assert::IsFalse(hub.available());
		}

		TEST_METHOD(SameIdTwice_SecondSkipped)
		{
			OnlineHub hub;
			FakePlugin first("alpha"), second("alpha");
			Assert::IsTrue(hub.addPlugin(&first, metadata("alpha")));
			OnlineClient* const client = hub.current();
			Assert::IsFalse(hub.addPlugin(&second, metadata("alpha", 1)));
			Assert::AreEqual(1, (int)hub.clients().size());
			Assert::IsTrue(client == hub.current(), L"the first stays");
		}

		TEST_METHOD(NoId_Skipped)
		{
			OnlineHub hub;
			FakePlugin plugin("alpha");
			Assert::IsFalse(hub.addPlugin(&plugin, metadata("")));
			Assert::IsFalse(hub.available());
		}

		// The current server is the first by its plugin's order, whichever was loaded first.
		TEST_METHOD(Order_LowestIsCurrent)
		{
			OnlineHub hub;
			FakePlugin later("beta"), first("alpha");
			Assert::IsTrue(hub.addPlugin(&later, metadata("beta", 20)));
			Assert::IsTrue(hub.addPlugin(&first, metadata("alpha", 10)));
			Assert::AreEqual(QString("alpha"), hub.current()->server().id());
			Assert::AreEqual(2, (int)hub.clients().size());
			Assert::AreEqual(QString("beta"), hub.providers().at(1).toMap().value("id").toString());
		}

		// A folder without plugins (the app shipped without them): no server, no failure.
		TEST_METHOD(LoadPlugins_EmptyFolder_None)
		{
			QTemporaryDir dir;
			OnlineHub hub;
			Assert::AreEqual(0, hub.loadPlugins(dir.path()));
			Assert::AreEqual(0, hub.loadPlugins(dir.path() + "/missing"));
			Assert::IsFalse(hub.available());
		}
	};
}
