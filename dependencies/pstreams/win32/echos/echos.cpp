/*
$Id: echos.cpp,v 1.1.2.3 2004/11/08 15:32:18 francisandre Exp $
*/
#include <iostream>
#include <fcntl.h>
#include <io.h>

#if defined(_WIN32) || defined(WIN32)
	#define REDI_OS_WIN32
	#ifndef WIN32
		#define WIN32
	#endif
#else
	#define REDI_OS_UNIX
#endif


#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
	#define NOATOM
	#define NOGDI
	#define NOGDICAPMASKS
	#define NOMETAFILE
	#define NOMINMAX
	#define NOMSG
	#define NOOPENFILE
	#define NORASTEROPS
	#define NOSCROLL
	#define NOSOUND
	#define NOSYSMETRICS
	#define NOTEXTMETRIC
	#define NOWH
	#define NOCOMM
	#define NOKANJI
	#define NOCRYPT
	#define NOMCX
	#include <windows.h> 
#endif


using namespace std;

int main(int argc, char* argv[], char* env[]) { 
	int rc = 0;
	char*	mode = "i";

	try {
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		HANDLE	output	= GetStdHandle(STD_OUTPUT_HANDLE); 
		HANDLE	input	= GetStdHandle(STD_INPUT_HANDLE); 
		HANDLE	error	= GetStdHandle(STD_ERROR_HANDLE); 

	if ((input	== INVALID_HANDLE_VALUE) || (output	== INVALID_HANDLE_VALUE)) 
		return -2;

		int result;
		result = setmode( fileno( stdin ), O_BINARY );
		result = setmode( fileno( stdout ), O_BINARY );
		result = setmode( fileno( stderr ), O_BINARY );
#endif

		if  (argc >= 2) {
			mode = argv[1];
		}

		cin.unsetf(ios::skipws);
		cout.unsetf(ios::skipws);

		if  (*mode == 'i') {
			unsigned char c;
			while(cin >> c) {
				cout << c;
			}
		} else
		if  (*mode == 'o') {
			unsigned char	array[256];
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
			DWORD		written;
			for(int no = 0; no < sizeof(array);no++) {
				array[no] = no;
			}
			WriteFile(output, array, sizeof(array), &written, NULL);
#endif
		}
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
	CloseHandle(input);
	CloseHandle(output);
	CloseHandle(error);
#endif
	} catch (exception& e) {
		cout << e.what() << endl;
		rc = -3;
	} catch (...) {
		cout << "Uncaught exception" << endl;
		rc = -4;
	}
	
	return rc;
}
