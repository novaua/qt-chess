#include "stdafx.h"

#include "fakeengine.h"

#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// FakeUci.exe: the UI tests' engine (BL-05 step 3). The app runs it instead of Stockfish when
// CHESS_ENGINE_PATH names it (UciConnector::EngineCandidates). See fakeengine.h for its moves.
//   FAKEUCI_MOVES="e2e4 e7e5 g1f3"   the game it follows (its own half of it)
//   FAKEUCI_DELAY_MS=2000            how long it thinks before each answer
static std::string Env(const char* name)
{
	char* value = nullptr;
	size_t size = 0;
	std::string text;
	if (_dupenv_s(&value, &size, name) == 0 && value) {
		text = value;
		free(value);
	}
	return text;
}

int main()
{
	std::vector<std::string> script;
	std::istringstream moves(Env("FAKEUCI_MOVES"));
	for (std::string move; moves >> move;)
		script.push_back(move);
	const int delayMs = std::atoi(Env("FAKEUCI_DELAY_MS").c_str());

	FakeEngine engine(script, delayMs);
	engine.Run(std::cin, std::cout);
	return 0;
}
