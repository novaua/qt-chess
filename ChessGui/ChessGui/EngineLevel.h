#pragma once
#include <chrono>

namespace Chess {

	class EngineLevel {
	public:
		explicit EngineLevel(int level = 3) : _level(level) {}

		// Maps UI level 1-5 to a target Elo, evenly spaced across Stockfish's
		// supported UCI_Elo range (1320-3190): 1320, 1787, 2255, 2722, 3190.
		int Elo() const { return 1320 + (_level - 1) * (3190 - 1320) / 4; }
		std::chrono::milliseconds MoveTime() const { return std::chrono::milliseconds(_level * 500); }

	private:
		int _level; // 1–5
	};
}
