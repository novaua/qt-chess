#pragma once
#include <chrono>

namespace Chess {

	class EngineLevel {
	public:
		explicit EngineLevel(int level = 3) : _level(level) {}

		int SkillLevel() const { return (_level - 1) * 5; }
		std::chrono::milliseconds MoveTime() const { return std::chrono::milliseconds(_level * 500); }

	private:
		int _level; // 1–5
	};
}
