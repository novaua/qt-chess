#pragma once
namespace Chess
{
	class ChessException : public std::exception
	{
		std::string _message;
	public:
		ChessException(const std::string &message);

		const char* what() const;

		~ChessException();
	};

}

