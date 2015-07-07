#include "stdafx.h"
#include "Game.h"

namespace Chess
{
	Game::Game()
	{
	}

	Game::~Game()
	{
	}

	class GameImplementation : public Game
	{
		Board _board;
		int _movesCount;

	public:
		GameImplementation(bool whiteFirst);

		void CheckNotify(const Action &listener);
		void MateNotify(const Action &listener);
		void EnPassantNotify(const Action &listener);
		void PawnPromotionNotify(const std::function<void(int)> &listener);

		void RegisterBoardChanged(const std::function<Move> &listener);

		std::vector<Move> GetPossibleMoves(int index);
		std::vector<Move> GetGameRecord();

		void Save(const std::string &path);
		void Load(const std::string &path);

		void DoMove(EMoves from, EMoves to);
		void UndoMove();
	};

	GameAptr Game::StartNewGame(bool whiteFirst)
	{
		return GameAptr(new GameImplementation(whiteFirst));
	}

	GameImplementation::GameImplementation(bool whiteFirst)
	{
	}

	void GameImplementation::CheckNotify(const Action &listener)
	{

	}
	void GameImplementation::MateNotify(const Action &listener)
	{

	}

	void GameImplementation::EnPassantNotify(const Action &listener)
	{

	}
	void GameImplementation::PawnPromotionNotify(const std::function<void(int)> &listener)
	{

	}

	void GameImplementation::RegisterBoardChanged(const std::function<Move> &listener)
	{

	}

	std::vector<Move> GameImplementation::GetPossibleMoves(int index)
	{
		std::vector<Move> result;

		return result;
	}

	std::vector<Move> GameImplementation::GetGameRecord()
	{
		std::vector<Move> result;

		return result;
	}

	void GameImplementation::Save(const std::string &path)
	{

	}

	void GameImplementation::Load(const std::string &path)
	{



	}

	void GameImplementation::DoMove(EMoves from, EMoves to)
	{

	}

	void GameImplementation::UndoMove()
	{
	}
}