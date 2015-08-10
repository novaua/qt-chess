#pragma once

namespace Chess
{
	enum EventType
	{
		EtEmpty,
		
		EtPawnPromotion,
		EtEnPassant,
		EtCastling,
		
		EtCheck,
		EtCheckMate,
	};

	class EventBase
	{
		EventType _type;

	public:
		EventBase(EventType et = EtEmpty);
		EventType GetType() const;
	};
}
