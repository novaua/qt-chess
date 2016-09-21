#ifndef	_Program_h_
#define _Program_h_

#include <string>

using namespace std;

class Program {
	string	path;
	string	directory;
	string	name;
	string	extension;

public:
	Program();
	Program(const string& path);
	~Program();

	const string& getPath() const { return path; }
};


#endif
