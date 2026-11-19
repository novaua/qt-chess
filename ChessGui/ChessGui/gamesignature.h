#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>

// BL-20: the signature of what the app keeps on disk (a game, the rating, the statistics), so a file
// edited by hand isn't taken in: HMAC-SHA256 with a key built into the app, the same in every copy, so
// a profile moved to another PC or Windows account verifies there too. A deterrent against editing a
// file, not security: the key is in the program (docs/BL-20-GameFilesPlan.md §2.5).
//
// A signature is "<key number>:<64 hex>". Key 1 never changes (every kept game would fail its check
// after an update); a new key would be added beside it: files signed with any known key verify and are
// signed again with the newest when next written. A key number this copy doesn't know fails.
class GameSignature
{
public:
	// What is signed: a domain per kind, so one kind's signature never fits another's.
	enum class Kind { Game, Rating, Statistics };

	static constexpr int CurrentKey = 1;

	static QString sign(Kind kind, const QByteArray& payload);
	static bool verify(Kind kind, const QByteArray& payload, const QString& signature);
	// A JSON object signed as it is written (canonical): a game record, the rating, the statistics.
	static QString sign(Kind kind, const QJsonObject& json) { return sign(kind, canonical(json)); }
	static bool verify(Kind kind, const QJsonObject& json, const QString& signature)
	{
		return verify(kind, canonical(json), signature);
	}
	// What is signed: the object's compact JSON. QJsonObject keeps its keys sorted and Qt writes numbers
	// in the shortest form that reads back the same, so the object read from a file gives the same bytes
	// again (GameSignatureTest guards it).
	static QByteArray canonical(const QJsonObject& json);
	// The key number a signature names; 0 when it names none.
	static int keyOf(const QString& signature);
	// Signed with the newest key (else it is signed again when next written).
	static bool isCurrent(const QString& signature) { return keyOf(signature) == CurrentKey; }

private:
	static QByteArray key(int number); // empty: no such key
	static QByteArray mac(int keyNumber, Kind kind, const QByteArray& payload);
};
