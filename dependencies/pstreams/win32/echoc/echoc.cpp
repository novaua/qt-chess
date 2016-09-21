/*
$Id: echoc.cpp,v 1.1.2.2 2004/10/01 07:08:43 francisandre Exp $
*/
#include <map>
#include <stdexcept>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>

#include <pstreams/pstream.h>

template class redi::basic_pstreambuf<char>;
template class redi::pstream_common<char>;
template class redi::basic_pstream<char>;
template class redi::basic_ipstream<char>;
template class redi::basic_opstream<char>;
template class redi::basic_rpstream<char>;

using namespace std;
class Integer {
public:
	static string	toString(int i) {
		stringstream	s;
		s << right << i;
		return s.str();
	}
	static string	toString(int i, int digits) {
		stringstream	s;
		s.fill('0');s.width(digits);
		s << right << i;
		return s.str();
	}
};
int main(int argc, char* argv[]) {
	using namespace redi;

	int status = -1;

	try {

		ios_base::openmode mode = ios_base::binary;

		string command =
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		"../echos/Debug/echos.exe";
#endif
		
		pstream	echos	(command, mode);
		
		for (int no = 0; no < 256; no++) {
			echos << (unsigned char) no;
		}
		echos << peof;

		unsigned char c;
		if (!(echos >> c)) {
			return 1;
		}
		cout << Integer::toString(c,3) << " ";
		no = 1;
		while (echos >> c) {
			if  ((no++ % 10) == 0) cout << endl;
			cout << Integer::toString(c,3) << " ";
		}
		cout << endl;

		echos.close();
		status = echos.rdbuf()->status();

		cout << "status = " << status << endl;
	} catch(exception& e) {
		cerr << e.what() << endl;
		return 1;
	} catch(...) {
		cerr << "Uncaught exception" << endl;
		return 1;
	}
	return status;
}

