#pragma once
#include <QtCore/QByteArray>
#include <QtCore/qendian.h>

namespace WewiPkt
{
	// Header: magic(4) + ver(2) + opcode(2) + length(4)  => 12 bytes
	inline constexpr quint32 kMagic = 0x57455749; // 'WEWI'
	inline constexpr quint16 kVersion = 1;
	inline constexpr int     kHdrSize = 12;

	enum class Status : quint8
	{
		Ok,
		NeedMore,
		BadMagic,
		BadVersion,
		TooBig,
	};

	class Framer
	{
	public:
		explicit Framer(int maxPayload = 1 << 20);

		void reset();
		void feed(const QByteArray& chunk);
		bool tryExtract(quint16& opcode, QByteArray& payload, Status& st);

	private:
		QByteArray _buf;
		int        _maxPayload;
	};

	// Build a packet buffer
	QByteArray make(quint16 opcode, QByteArrayView payload);
	QByteArray makeText(quint16 opcode, const QString& textUtf8);
}
