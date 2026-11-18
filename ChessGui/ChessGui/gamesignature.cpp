#include "stdafx.h"
#include "gamesignature.h"

#include <QMessageAuthenticationCode>

// Key 1, kept as two masked halves put together at run time: a search of the program's strings doesn't
// find it, nothing more (the plan's §2.5). NEVER change it: every game signed with it would be set aside.
static const unsigned char Key1Masked[32] = {
	0x46, 0xf8, 0x29, 0x6c, 0x56, 0xd4, 0xa6, 0xc1, 0xae, 0x2c, 0x3a, 0xe1, 0xa3, 0xff, 0xda, 0xfe,
	0xf0, 0x2f, 0xc5, 0x3b, 0x36, 0x62, 0x8a, 0xdc, 0x43, 0xd5, 0x1f, 0x63, 0x37, 0x2d, 0xea, 0x1b,
};
static const unsigned char Key1Mask[32] = {
	0x20, 0x43, 0x77, 0xa5, 0x5b, 0xae, 0xa8, 0xc6, 0x4c, 0x7a, 0x87, 0x54, 0x6b, 0x08, 0x57, 0xb1,
	0xdd, 0xf8, 0x18, 0x4b, 0x66, 0xac, 0xb2, 0xdf, 0x7e, 0x3b, 0xc3, 0xd6, 0x4a, 0x7b, 0x94, 0x78,
};

QByteArray GameSignature::key(int number)
{
	if (number != 1)
		return {};
	QByteArray k(32, Qt::Uninitialized);
	for (int i = 0; i < 32; ++i)
		k[i] = char(Key1Masked[i] ^ Key1Mask[i]);
	return k;
}

static QByteArray domainOf(GameSignature::Kind kind)
{
	switch (kind) {
	case GameSignature::Kind::Game:       return QByteArrayLiteral("chess++ game\n");
	case GameSignature::Kind::Rating:     return QByteArrayLiteral("chess++ rating\n");
	case GameSignature::Kind::Statistics: return QByteArrayLiteral("chess++ statistics\n");
	}
	return {};
}

QByteArray GameSignature::mac(int keyNumber, Kind kind, const QByteArray& payload)
{
	const QByteArray k = key(keyNumber);
	if (k.isEmpty())
		return {};
	QMessageAuthenticationCode code(QCryptographicHash::Sha256, k);
	code.addData(domainOf(kind));
	code.addData(payload);
	return code.result();
}

QString GameSignature::sign(Kind kind, const QByteArray& payload)
{
	return QString::number(CurrentKey) + QLatin1Char(':')
		+ QString::fromLatin1(mac(CurrentKey, kind, payload).toHex());
}

int GameSignature::keyOf(const QString& signature)
{
	const int colon = (int)signature.indexOf(QLatin1Char(':'));
	if (colon <= 0)
		return 0;
	bool ok = false;
	const int number = signature.left(colon).toInt(&ok);
	return ok && number > 0 ? number : 0;
}

bool GameSignature::verify(Kind kind, const QByteArray& payload, const QString& signature)
{
	const int number = keyOf(signature);
	const QByteArray expected = mac(number, kind, payload);
	if (expected.isEmpty())
		return false; // no signature, or a key this copy doesn't know
	const QByteArray given = QByteArray::fromHex(signature.mid(signature.indexOf(QLatin1Char(':')) + 1).toLatin1());
	if (given.size() != expected.size())
		return false;
	unsigned char diff = 0; // the same time whichever byte differs
	for (int i = 0; i < expected.size(); ++i)
		diff |= (unsigned char)(expected[i] ^ given[i]);
	return diff == 0;
}
