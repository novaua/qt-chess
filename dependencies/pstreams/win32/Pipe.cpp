/*
$Id: Pipe.cpp,v 1.1.2.7 2005/04/06 16:09:21 francisandre Exp $
*/
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

#include <pstreams/win32/Pipe.h>

using namespace std;


namespace redi {
	Pipe::Pipe(desc r, desc w) :
		readend(r),
		writend(w) {
	}
	void Pipe::open() throw (Pipe::Exception) {
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		SECURITY_ATTRIBUTES saAttr; 
		BOOL fSuccess = FALSE; 
		 
		// Set the bInheritHandle flag so pipe handles are inherited. 
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
		saAttr.bInheritHandle = TRUE; 
		saAttr.lpSecurityDescriptor = NULL; 

		if (! CreatePipe(&readend, &writend, &saAttr, 0)) 
			throw Exception("pipe creation failed");
	#endif
	}
	Pipe::~Pipe() {
	}
	Pipe::Exception::Exception(const string& text) :
		exception("Pipe::Exception: ") {
	}
	Pipe::Exception::~Exception() {
	}
	Pipe Pipe::toReadPipe() {
		
		BOOL result;
		desc	r;
		desc	w;
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		result = DuplicateHandle(GetCurrentProcess(), readend,
			GetCurrentProcess(),&r, 0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if  (!result)
			throw Exception("pipe duplication failed");

		result = DuplicateHandle(GetCurrentProcess(), writend,
			GetCurrentProcess(),&w, 0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if  (!result)
			throw Exception("pipe duplication failed");

		closeReadEndPoint();
		return Pipe(r,w);
	#endif
	}
	Pipe Pipe::toWritePipe() {
		
		BOOL result;
		desc	r;
		desc	w;
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		result = DuplicateHandle(GetCurrentProcess(), readend,
			GetCurrentProcess(),&r, 0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if  (!result)
			throw Exception("pipe duplication failed");

		result = DuplicateHandle(GetCurrentProcess(), writend,
			GetCurrentProcess(),&w, 0,
			FALSE,
			DUPLICATE_SAME_ACCESS);
		if  (!result)
			throw Exception("pipe duplication failed");

		closeWriteEndPoint();
		return Pipe(r,w);
	#endif
	}
    int Pipe::read(void* buffer, std::streamsize size) {
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		DWORD readed = -1; 
		BOOL result;

		result = ReadFile( readend, buffer, size, &readed, NULL);
		DWORD s = 0;
		if  (!result) {
			s = GetLastError();
		}
	#endif

		return readed;
	}
	int Pipe::write(const void* buffer, std::streamsize size) {
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		DWORD written = -1; 
		BOOL result = false;
		
		result = WriteFile(writend, buffer, size,  &written, NULL);
	    
		if  (!result) {
			DWORD s = GetLastError();
		}
	#endif
		return written;
	}
	void Pipe::close()  throw (Exception){
		closeReadEndPoint();
		closeWriteEndPoint();
	}
	void Pipe::closeReadEndPoint() throw (Exception){
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		if  (readend != INVALID_HANDLE_VALUE && !CloseHandle(readend))
			throw Exception("cannot close read end point");
		readend = INVALID_HANDLE_VALUE;
	#endif
	}
	void Pipe::closeWriteEndPoint()  throw (Exception){
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		if  (writend  != INVALID_HANDLE_VALUE && !CloseHandle(writend))
			throw Exception("cannot close write end point");
		writend = INVALID_HANDLE_VALUE;
	#endif
	}
	bool Pipe::valid(PipeSide ps) const {
	#if		defined(REDI_OS_UNIX)
		if  (ps == ReadSide) {
			return readend >= 0;
		} else
		if  (ps == WriteSide) {
			return writend >= 0;
		}
		else throw Exception("internal error");

	#elif	defined(REDI_OS_WIN32)
		if  (ps == ReadSide) {
			return readend != INVALID_HANDLE_VALUE;
		} else
		if  (ps == WriteSide) {
			return writend != INVALID_HANDLE_VALUE;
		}
		else throw Exception("internal error");
	#endif
	}
}
/*
$Log: Pipe.cpp,v $
Revision 1.1.2.7  2005/04/06 16:09:21  francisandre
Add include <pstreams/config.h>

Revision 1.1.2.6  2004/10/01 07:13:11  francisandre
fix a missing elif

Revision 1.1.2.5  2004/10/01 07:09:33  francisandre
add RCS variables: $Id and $Log

Revision 1.1.2.4  2004/10/01 07:06:10  francisandre
add RCS variables: $Id and $Log

*/