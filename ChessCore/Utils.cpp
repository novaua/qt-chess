#include "stdafx.h"
#include "Utils.h"

using namespace std;

void split(const string &s, char delim, vector<string> &elems) {
    stringstream ss;
    ss.str(s);
    string item;
    while (getline(ss, item, delim))
    {
        if (!item.empty())
            elems.push_back(item);
    }
}

vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

bool match_command(const std::string &command, const std::string & commandsLine)
{
    return begins_with(commandsLine, command);
}