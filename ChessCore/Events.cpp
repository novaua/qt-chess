#include "stdafx.h"
#include "Events.h"

using namespace Chess;

EventBase::EventBase(EventType et)
	:_type(et)
{

}

EventType EventBase::GetType() const
{
	return _type;
}

PawnPromotionEvent::PawnPromotionEvent(EventType et, int index, int color)
	:EventBase(et), _index(index), _color(color)
{
}

int PawnPromotionEvent::GetIndex() const
{
	return _index;
}

int PawnPromotionEvent::GetColor() const
{
	return _color;
}
