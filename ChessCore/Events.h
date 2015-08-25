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

	class PawnPromotionEvent: public EventBase
	{
	public:
		PawnPromotionEvent(EventType et, int index, int color);
		int GetIndex() const;
		int GetColor() const;

	private:
		int _index;
		int _color;
	};
}
