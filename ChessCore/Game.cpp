#include "stdafx.h"
#include "Game.h"
#include "Move.h"
#include "Piece.h"
#include "ChessException.h"
#include "Serializer.h"

namespace Chess
{
	Game::Game()
		:_boardAptr(std::make_shared<Board>()),

		//this may be made persistent
		_boardPositionsCacheAptr(std::make_shared<BoardPositionsCache>()),
		_historyAptr(std::make_shared<MovesHistory>()),
		_whiteFirst(true),
		_checkMate(false)
	{
		_gameState = { _boardAptr, _historyAptr, _boardPositionsCacheAptr };
	}

	void Game::NotifyFullBoardReinit()
	{
		std::vector<BoardPosition> range(64);
		for (int i = 0; i < 64; ++i)
		{
			range[i] = BoardPosition(i);
		}

		NotifyBoardChangesListeners(range);
	}

	Game::~Game()
	{
	}

	void Game::RegisterGameActionsListeners(const GameActionListener& listener)
	{
		_gameActionsListeners.push_back(listener);
	}

	void Game::RegisterBoardChanged(const BoardChangesListener& listener)
	{
		_boardChangesListeners.push_back(listener);
	}

	std::vector<Move> Game::GetPossibleMoves(int index)
	{
		auto side = IsWhiteMove() ? PieceColors::Light : PieceColors::Dark;

		auto moves = MoveGeneration::GenerateMoves(_gameState, (BoardPosition)index, side);
		MoveGeneration::ExcludeCheckMoves(_gameState, moves, side);
		return moves;
	}

	const MovesHistory& Game::GetGameRecord() const
	{
		return *_historyAptr;
	}

	void Game::Restart(bool whiteFirst)
	{
		{
			std::lock_guard<std::mutex> lg(_lock);

			_whiteFirst = whiteFirst;
			_checkMate = false;
			*_historyAptr = {};
			_captured = {};

			_boardAptr->Initialize();
		};

		NotifyFullBoardReinit();
	}

	void Game::EndGame()
	{
		Restart(_whiteFirst);
	}

	HistoryPlayerAptr Game::MakePlayer()
	{
		auto player = std::make_shared<HistoryPlayer>(shared_from_this());
		player->Play(_loadedHistory);
		assert(_loadedHistory.size() > 0 && "Expected loaded history upon play!");
		return player;
	}

	void Game::Save(const std::string& path)
	{
		auto moveCount = GetMoveCount();
		std::ofstream outFileStream(path, std::ios::out | std::ios::binary);

		outFileStream.write(SaveGameHeader, strlen(SaveGameHeader) * sizeof(char));

		BinarySerializer::Serialize(moveCount, outFileStream);

		for (auto move : *_historyAptr)
		{
			BinarySerializer::Serialize(move, outFileStream);
		}
	}

	void ValidateHeader(std::ifstream& fs, const std::string& header)
	{
		std::vector<char> buffer(header.size() * sizeof(char), '\0');
		fs.read(&buffer[0], header.size() * sizeof(char));

		std::string fileHeader(buffer.begin(), buffer.end());
		if (fileHeader != header)
		{
			throw ChessException("Invalid Game Save file format!");
		}
	}

	void Game::Load(const std::string& path)
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
				for (auto index : indexes)
				{
					listener(index, _boardAptr->At(index));
				}
			}
		}
	}

	void Game::NotifyActionsListeners(const EventBase& event)
	{
		for (auto listener : _gameActionsListeners)
		{
			if (listener)
			{
				listener(event);
			}
		}
	}

	void Game::DoMove(const Move& move1)
	{
		auto move = move1;
		auto side = _boardAptr->At(move.From).Color;
		MoveGeneration::Validate(_gameState, move, side);

		Move complementalMove;
		std::vector<BoardPosition> changedPositions = { move.From, move.To };
		if (MoveGeneration::AddComplementalMove(*_boardAptr, move, complementalMove))
		{
			// This is an inline generated move so not added to history
			// applies for Castling and En Passant
			_boardAptr->DoMove(complementalMove);
			changedPositions.push_back(complementalMove.From);
			changedPositions.push_back(complementalMove.To);
		}

		auto historyMove = _boardAptr->DoMove(move);

		historyMove.Id = GetMoveCount();

		NotifyBoardChangesListeners(changedPositions);

		if (historyMove.IsCapturingMove())
		{
			_captured.push(historyMove.To.Piece);
		}

		_historyAptr->push_back(historyMove);
		GameChecks check(_gameState);

		int ppIndex = move.PromotedTo.IsEmpty() ? check.IsInPawnPromotion(side) : -1;
		if (ppIndex >= 0)
		{
			auto pawnPromotionEvent = PawnPromotionEvent(EtPawnPromotion, ppIndex, (int)side);

			pawnPromotionEvent.OnPromoted = [this](const PositionPiece& positionPiece)
			{
				_historyAptr->rbegin()->PromotedTo = positionPiece.Piece;

				_boardAptr->Place(positionPiece.Position, positionPiece.Piece);
				NotifyBoardChangesListeners({ positionPiece.Position });
			};

			NotifyActionsListeners(pawnPromotionEvent);
		}
		else if (check.IsInCheck(OppositeSideOf(side)))
		{
			if (check.IsCheckMate(OppositeSideOf(side)))
			{
				NotifyActionsListeners(EtCheckMate);
				_checkMate = true;
			}
			else
			{
				NotifyActionsListeners(EtCheck);
			}
		}
	}

	void Game::DoMove(BoardPosition from, BoardPosition to)
	{
		DoMove({ from, to });
	}

	void Game::UndoMove()
	{
		if (GetMoveCount() == 0)
		{
			throw CannotUndoException();
		}

		// undoing move
		HistoryMove topMove = *_historyAptr->rbegin();
		Move undoMove{ topMove.To.Position, topMove.From.Position };
		_boardAptr->DoMove(undoMove);

		// placing removed item
		Piece lastPiece = {};
		if (topMove.IsCapturingMove())
		{
			lastPiece = _captured.top();
			_captured.pop();
			_boardAptr->Place(topMove.To.Position, lastPiece);
			assert(topMove.To.Piece == lastPiece && "Has to be the same Piece!");
		}

		_historyAptr->pop_back();
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
		return (int)_historyAptr->size();
	}

	void Game::AssureMove(BoardPosition from, BoardPosition to)
	{
		auto fromPiece = _boardAptr->At(from);
		auto toPiece = _boardAptr->At(to);

		if (!CanMoveFrom(from) || fromPiece.Color == toPiece.Color)
		{
			throw ChessException("Invalid move!");
		}
	}

	bool Game::CanMoveFrom(BoardPosition from)
	{
		auto fromPiece = _boardAptr->At(from);
		return !(fromPiece.Color == PieceColors::Light && !IsWhiteMove()
			|| fromPiece.Color == PieceColors::Dark && IsWhiteMove()
			|| fromPiece.Color == PieceColors::Empty);
	}

	Piece Game::GetPieceAt(int index) const
	{
		return _boardAptr->At(index);
	}

	std::vector<Move> Game::GetAllowedMoves(int index)
	{
		return (0 <= index && index < 64 && CanMoveFrom(BoardPosition(index)))
			? GetPossibleMoves(index)
			: std::vector<Move>();
	}

	void Game::RegisterLogger(const LoggerCallback& loggerCallback)
	{
		_logger = loggerCallback;
	}

	void Game::Log(const std::string& message)
	{
		if (_logger)
		{
			_logger(message);
		}
	}

	std::string Game::MakeFen()
	{
		std::string fen;
		int emptyCount = 0;

		for (int i = 0; i < 64; ++i) {
			auto piece = GetPieceAt(i);

			if (piece.IsEmpty()) {
				emptyCount += 1;
			}
			else if (0 < emptyCount)
			{
				fen += std::to_string(emptyCount);
				emptyCount = 0;
			}

			if (!piece.IsEmpty())
				fen += piece.ToString();

			if (0 < i && i < 63 && (i + 1) % 8 == 0) {
				if (0 < emptyCount)
				{
					fen += std::to_string(emptyCount);
					emptyCount = 0;
				}

				fen += "/";
			}
		}

		fen += IsWhiteMove() ? " w" : " b";
		fen += " KQkq";	// ToDo: serch history for castling possibility
		fen += " -";	//Todo: serch for el-pasant possibility
		fen += " " + std::to_string(GetMoveCount() + 1 / 2);
		fen += " " + std::to_string(GetMoveCount() + 1);

		return fen;
	}
}
