#include "pch.h"
#include "WeWiPacket.h"

#include <QtCore/QDataStream>

using namespace WewiPkt;

Framer::Framer(int maxPayload)
	: _maxPayload(maxPayload)
{
}

void Framer::reset()
{
	_buf.clear();
}

void Framer::feed(const QByteArray& chunk)
{
	if (!chunk.isEmpty())
		_buf += chunk;
}

bool Framer::tryExtract(quint16& opcode, QByteArray& payload, Status& st)
{
	st = Status::NeedMore;

	if (_buf.size() < kHdrSize)
		return false;

	const uchar* p = reinterpret_cast<const uchar*>(_buf.constData());
	const quint32 magic = qFromLittleEndian<quint32>(p + 0);
	const quint16 ver = qFromLittleEndian<quint16>(p + 4);
	const quint16 op = qFromLittleEndian<quint16>(p + 6);
	const quint32 len = qFromLittleEndian<quint32>(p + 8);

	if (magic != kMagic)
	{
		st = Status::BadMagic;
		_buf.clear(); // drop garbage to resync
		return false;
	}

	if (ver != kVersion)
	{
		st = Status::BadVersion;
		_buf.clear();
		return false;
	}

	if (len > static_cast<quint32>(_maxPayload))
	{
		st = Status::TooBig;
		_buf.clear();
		return false;
	}

	if (_buf.size() < kHdrSize + static_cast<int>(len))
		return false; // NeedMore

	opcode = op;
	payload = _buf.mid(kHdrSize, static_cast<int>(len));
	_buf.remove(0, kHdrSize + static_cast<int>(len));

	st = Status::Ok;
	return true;
}

QByteArray WewiPkt::make(quint16 opcode, QByteArrayView payload)
{
	QByteArray buf;
	buf.resize(kHdrSize + static_cast<int>(payload.size()));

	uchar* p = reinterpret_cast<uchar*>(buf.data());
	qToLittleEndian<quint32>(kMagic, p + 0);
	qToLittleEndian<quint16>(kVersion, p + 4);
	qToLittleEndian<quint16>(opcode, p + 6);
	qToLittleEndian<quint32>(payload.size(), p + 8);

	if (!payload.empty())
		::memcpy(p + kHdrSize, payload.data(), static_cast<size_t>(payload.size()));

	return buf;
}

QByteArray WewiPkt::makeText(quint16 opcode, const QString& textUtf8)
{
	const QByteArray utf8 = textUtf8.toUtf8();
	return make(opcode, utf8);
}
