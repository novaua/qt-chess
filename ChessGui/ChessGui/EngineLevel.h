#pragma once
#include <algorithm>
#include <chrono>
#include <iterator>

namespace Chess {

	class EngineLevel {
	public:
		static constexpr int Count = 5;

		explicit EngineLevel(int level = 3) : _level(std::clamp(level, 1, Count)) {}

		int Level() const { return _level; }

		// Stockfish's weakest UCI_Elo (1320) is calibrated against engines, not people: it
		// never hangs a piece and beats beginners easily. Levels 1-2 therefore don't limit the
		// engine; it lists its best moves at full strength and PickWeakMove() chooses among
		// them at random, so bigger mistakes come up regularly. Levels 3-5 use UCI_Elo.
		bool UsesEngineElo() const { return Profile().elo > 0; }

		int Elo() const { return Profile().elo; }
		std::chrono::milliseconds MoveTime() const { return std::chrono::milliseconds(Profile().moveTimeMs); }

		// Weak-move picker settings, used only when !UsesEngineElo().
		int CandidateCount() const { return Profile().candidates; }
		int SearchDepth() const { return Profile().depth; }
		int TemperatureCp() const { return Profile().temperatureCp; }

		// Shown in the level picker, the help screen and the robot avatar's tooltip.
		// Levels 1-2 have no engine Elo, so their rating is a rough human estimate.
		const char* Name() const { return Profile().name; }
		int DisplayElo() const { return UsesEngineElo() ? Profile().elo : Profile().estimatedElo; }
		const char* Hint() const { return Profile().hint; }

	private:
		struct LevelProfile {
			int elo;            // 0 → weak-move picker instead of UCI_Elo
			int moveTimeMs;
			int candidates;
			int depth;
			int temperatureCp;  // higher → worse moves are picked more often
			int estimatedElo;   // weak-picker levels only
			const char* name;
			const char* hint;
		};

		const LevelProfile& Profile() const
		{
			static constexpr LevelProfile Profiles[] = {
				{    0,    0, 12, 6, 250,  600, "Beginner",
				  "Often leaves pieces undefended and can miss a checkmate. Great for learning how the pieces move." },
				{    0,    0,  8, 8, 100, 1000, "Novice",
				  "Plays sensible moves but blunders now and then. Punish its mistakes to win." },
				{ 1320,  500,  0, 0,   0,    0, "Club Player",
				  "Stockfish's weakest built-in setting. Rarely gives material away for free." },
				{ 2000, 1500,  0, 0,   0,    0, "Expert",
				  "Strong, solid play. Expect to be punished for every inaccuracy." },
				{ 3190, 2500,  0, 0,   0,    0, "Grandmaster",
				  "Stockfish at full strength. Stronger than any human player." },
			};
			static_assert(std::size(Profiles) == Count, "one profile per level");
			return Profiles[_level - 1];
		}

		int _level; // 1–Count
	};
}
