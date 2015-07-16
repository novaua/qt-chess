#include "stdafx.h"
#include "Game.h"
#include "Move.h"
#include "Piece.h"
#include "ChessException.h"
#include "Serializer.h"

namespace Chess
{
	Game::Game()
		: _board(new Board()),
		_whiteFirst(true)
	{
	}

	Game::~Game()
	{
	}

	void Game::RegisterGameActionsListeners(const GameActionListener & listener)
	{
		_gameActionsListeners.push_back(listener);
	}

	void Game::RegisterBoardChanged(const BoardChangesListener &listener)
	{
		_boardChangesListeners.push_back(listener);
	}

	std::vector<Move> Game::GetPossibleMoves(int index)
	{
		auto moves = MoveGeneration::GenerateMoves(*_board, (BoardPosition)index, IsWhiteMove() ? LIGHT : DARK);
		return moves;
	}

	const GameHistory &Game::GetGameRecord() const
	{
		return _history;
	}

	void Game::Restart(bool whiteFirst)
	{
		_whiteFirst = whiteFirst;
		_history.empty();
		_captured.empty();

		_board.reset(new Board());
	}

	void Game::Play(const GameHistory &gameHistory)
	{
		for each (auto historyMove in _loadedHistory)
		{
			std::this_thread::sleep_for(std::chrono::seconds(AutoPlayMoveWaitSeconds));
			auto move = historyMove.ToMove();
			DoMove(move.From, move.To);
		}
	}

	void Game::Save(const std::string &path)
	{
		auto moveCount = GetMoveCount();
		std::ofstream outFileStream(path, std::ios::out | std::ios::binary);

		outFileStream.write(SaveGameHeader, sizeof(SaveGameHeader));
		BinarySerializer::Serialize(moveCount, outFileStream);

		for (auto move : _history)
		{
			BinarySerializer::Serialize(move, outFileStream);
		}
	}

	void ValidateHeader(std::ifstream &fs, const std::string &header)
	{
		std::vector<char> buffer(header.size() * sizeof(char) + 1, '\0');
		fs.read(&buffer[0], header.size() * sizeof(char));

		std::string fileHeader(buffer.begin(), buffer.end());
		if (fileHeader != header)
		{
			throw ChessException("Invalid Game Save file format!");
		}
	}

	void Game::Load(const std::string &path)
	{
		std::ifstream fs(path, std::ios::in | std::ios::binary);
		ValidateHeader(fs, SaveGameHeader);

		auto count = BinarySerializer::Deserialize<int>(fs);
		for (auto i = 0; i < count; ++i)
		{
			_loadedHistory.push_back(BinarySerializer::Deserialize<HistoryMove>(fs));
		}
	}

	void NotifyBoardChanged(const BoardChangesListeners & boardChangesListeners, const Move &move)
	{
		for (auto listener : boardChangesListeners)
		{
			if (listener)
			{
				listener(move);
			}
		}
	}

	void Game::DoMove(BoardPosition from, BoardPosition to)
	{
		AssureMove(from, to);

		auto historyMove = _board->DoMove({ from, to }, false);
		historyMove.Id = GetMoveCount();

		NotifyBoardChanged(_boardChangesListeners, { from, to });
		if (historyMove.IsCapturingMove())
		{
			_captured.push(historyMove.To.Piece);
		}

		_history.push_back(historyMove);
	}

	void Game::UndoMove()
	{
		if (GetMoveCount() == 0)
		{
			throw CannotUndoException();
		}

		// undoing move
		HistoryMove topMove = *(_history.rbegin());
		Move undoMove{ topMove.To.Position, topMove.From.Position };
		_board->DoMove(undoMove, true);

		// placing removed item
		Piece lastPiece = {};
		if (topMove.IsCapturingMove())
		{
			lastPiece = _captured.top();
			_captured.pop();
			_board->Place(topMove.To.Position, lastPiece);
			assert(topMove.To.Piece == lastPiece && "Has to be the same Piece!");
		}

		_history.pop_back();
		NotifyBoardChanged(_boardChangesListeners, undoMove);
	}

	bool Game::IsWhiteMove()
	{
		auto isEven = GetMoveCount() % 2 == 0;
		return _whiteFirst ? isEven : !isEven;
	}

	int Game::GetMoveCount() const
	{
		return _history.size();
	}

	void Game::AssureMove(BoardPosition from, BoardPosition to)
	{
		auto fromPiece = _board->At(from);

		if (fromPiece.Color == LIGHT && !IsWhiteMove()
			|| fromPiece.Color == DARK && IsWhiteMove()
			|| fromPiece.Color == CEMPTY)
		{
			throw ChessException("Invalid move!");
		}
	}
}