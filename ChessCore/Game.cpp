#include "stdafx.h"
#include "Game.h"
#include "Move.h"
#include "Piece.h"
#include "ChessException.h"
#include "Serializer.h"

namespace Chess
{
	Game::Game()
		:_whiteFirst(true)
	{
		InitBoard();
	}

	void Game::InitBoard()
	{
		_board.reset(new Board());

		if (!_boardPositionsCache)
		{
			_boardPositionsCache = std::make_shared<BoardPositionsCache>(_board);
		}
		else
		{
			_boardPositionsCache->SetBoard(_board);
		}

		_history = std::make_shared<MovesHistory>();

		_gameState = { _board, _history, _boardPositionsCache };
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
		auto moves = MoveGeneration::GenerateMoves(_gameState, (BoardPosition)index, IsWhiteMove() ? LIGHT : DARK);
		return moves;
	}

	const MovesHistory &Game::GetGameRecord() const
	{
		return *_history;
	}

	void Game::Restart(bool whiteFirst)
	{
		{
			std::lock_guard<std::mutex> lg(_lock);

			_whiteFirst = whiteFirst;
			*_history = {};
			_captured = {};

			_board->Initialize();
		};

		std::vector<BoardPosition> range(64);
		for (int i = 0; i < 64; ++i)
		{
			range[i] = BoardPosition(i);
		}

		NotifyBoardChangesListeners(range);
	}

	void Game::EndGame()
	{
		{
			std::lock_guard<std::mutex> lg(_lock);

			InitBoard();
			_captured = {};
			_loadedHistory = {};
		}

		std::vector<BoardPosition> range(64);
		for (int i = 0; i < 64; ++i)
		{
			range[i] = BoardPosition(i);
		}

		NotifyBoardChangesListeners(range);
	}

	HistoryPlayerAptr Game::MakePlayer()
	{
		auto player = std::make_shared<HistoryPlayer>(shared_from_this());
		player->Play(_loadedHistory);
		assert(_loadedHistory.size()>0 && "Expected loaded history upon play!");
		return player;
	}

	void Game::Save(const std::string &path)
	{
		auto moveCount = GetMoveCount();
		std::ofstream outFileStream(path, std::ios::out | std::ios::binary);

		outFileStream.write(SaveGameHeader, strlen(SaveGameHeader)*sizeof(char));

		BinarySerializer::Serialize(moveCount, outFileStream);

		for (auto move : *_history)
		{
			BinarySerializer::Serialize(move, outFileStream);
		}
	}

	void ValidateHeader(std::ifstream &fs, const std::string &header)
	{
		std::vector<char> buffer(header.size() * sizeof(char), '\0');
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

	void Game::NotifyBoardChangesListeners(std::vector<BoardPosition> indexes)
	{
		for (auto listener : _boardChangesListeners)
		{
			if (listener)
			{
				for each (auto index in indexes)
				{
					listener(index, _board->At(index));
				}
			}
		}
	}

	void Game::NotifyActionsListeners(const EventBase &event)
	{
		for (auto listener : _gameActionsListeners)
		{
			if (listener)
			{
				listener(event);
			}
		}
	}

	void Game::DoMove(BoardPosition from, BoardPosition to)
	{
		AssureMove(from, to);

		auto side = _board->At(from).Color;
		Move move = { from, to };
		std::vector<Move> lastAsked;
		{
			std::lock_guard<std::mutex> lg(_lastAskedLock);
			lastAsked.swap(_lastAskedAllowedMovesList);
		}

		bool moveSet = false;
		for each (auto element in lastAsked)
		{
			if (element.To == to && element.From == from)
			{
				move = element;
				moveSet = true;
				break;
			}
		}

		if (!moveSet)
		{
			MoveGeneration::Validate(_gameState, move, side);
		}

		Move complementalMove;
		std::vector<BoardPosition> changedPositions = { from, to };
		if (MoveGeneration::AddComplementalMove(*_board, move, complementalMove))
		{
			// This is an inline generated move so not added to history
			// applies for Castling and En Passant
			_board->DoMove(complementalMove);
			changedPositions.push_back(complementalMove.From);
			changedPositions.push_back(complementalMove.To);
		}

		auto historyMove = _board->DoMove(move);

		historyMove.Id = GetMoveCount();

		NotifyBoardChangesListeners(changedPositions);

		if (historyMove.IsCapturingMove())
		{
			_captured.push(historyMove.To.Piece);
		}

		_history->push_back(historyMove);

		GameChecks check(_gameState);
		if (check.IsInCheck(OppositeSideOf(side)))
		{
			if (check.IsCheckMate(OppositeSideOf(side)))
			{
				NotifyActionsListeners(EtCheckMate);
			}
			else
			{
				NotifyActionsListeners(EtCheck);
			}
		}
	}

	void Game::UndoMove()
	{
		if (GetMoveCount() == 0)
		{
			throw CannotUndoException();
		}

		// undoing move
		HistoryMove topMove = *_history->rbegin();
		Move undoMove{ topMove.To.Position, topMove.From.Position };
		_board->DoMove(undoMove);

		// placing removed item
		Piece lastPiece = {};
		if (topMove.IsCapturingMove())
		{
			lastPiece = _captured.top();
			_captured.pop();
			_board->Place(topMove.To.Position, lastPiece);
			assert(topMove.To.Piece == lastPiece && "Has to be the same Piece!");
		}

		_history->pop_back();
		NotifyBoardChangesListeners({ undoMove.From, undoMove.To });
	}

	bool Game::IsWhiteMove()
	{
		auto isEven = GetMoveCount() % 2 == 0;
		return _whiteFirst ? isEven : !isEven;
	}

	int Game::GetMoveCount()
	{
		std::lock_guard<std::mutex> lg(_lock);
		return _history->size();
	}

	void Game::AssureMove(BoardPosition from, BoardPosition to)
	{
		auto fromPiece = _board->At(from);
		auto toPiece = _board->At(to);

		if (!CanMoveFrom(from) || fromPiece.Color == toPiece.Color)
		{
			throw ChessException("Invalid move!");
		}
	}

	bool Game::CanMoveFrom(BoardPosition from)
	{
		auto fromPiece = _board->At(from);
		return !(fromPiece.Color == LIGHT && !IsWhiteMove()
			|| fromPiece.Color == DARK && IsWhiteMove()
			|| fromPiece.Color == CEMPTY);
	}

	Piece Game::GetPieceAt(int index)
	{
		return _board->At((BoardPosition)index);
	}

	std::vector<Move> &Game::GetAllowedMoves(int index)
	{
		auto lastAskedAllowedMovesList =
			(0 < index && index < 64 && CanMoveFrom(BoardPosition(index)))
			? GetPossibleMoves(index)
			: std::vector<Move>();
		{
			std::lock_guard<std::mutex> lg(_lastAskedLock);
			_lastAskedAllowedMovesList.swap(lastAskedAllowedMovesList);
			return _lastAskedAllowedMovesList;
		}
	}
}
