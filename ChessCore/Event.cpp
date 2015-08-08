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