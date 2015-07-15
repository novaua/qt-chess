#pragma once

namespace Chess
{
	enum EventType
	{
		EtEmpty,
		EtCheck,
		EtMate,
		PawnPromotion,
		CannotUndo,
	};

	class EventBase
	{
		EventType _type;
	
	public:
		EventBase(EventType et = EtEmpty);
		EventType GetType();
	};
};