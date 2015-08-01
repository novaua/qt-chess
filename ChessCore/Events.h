#pragma once

namespace Chess
{
	enum EventType
	{
		EtEmpty,
		
		PawnPromotion,
		EnPassant,
		Castling,
		
		EtCheck,
		EtMate,
	};

	class EventBase
	{
		EventType _type;

	public:
		EventBase(EventType et = EtEmpty);
		EventType GetType();
	};
}
