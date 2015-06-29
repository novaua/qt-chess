#include "stdafx.h"
#include "ChessException.h"

using namespace Chess;
ChessException::ChessException(const std::string &what)
	:_message(what)
{
}

ChessException::~ChessException()
{
}

const char* ChessException::what() const
{
	return _message.c_str();
}
