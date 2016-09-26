#pragma once

void splitsplit(const std::string &s, char delim, std::vector<std::string> &elems);

std::vector<std::string> split(const std::string &s, char delim);

template<class TContainer>
bool begins_with(const TContainer& input, const TContainer& match)
{
    return input.size() >= match.size() && equal(match.begin(), match.end(), input.begin());
}

bool match_command(const std::string &command, const std::string & commandsLine);
