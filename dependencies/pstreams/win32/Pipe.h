#ifndef	_Pipe_h_
#define _Pipe_h_
/*
$Id: Pipe.h,v 1.1.2.4 2005/04/06 16:09:20 francisandre Exp $
*/

#include <stdexcept>
#include <pstreams/config.h>

namespace redi {
	class PSTREAMS_API Pipe {
	public:
	#if		defined(REDI_OS_UNIX)
		typedef int		desc;
	#elif	defined(REDI_OS_WIN32)
		typedef HANDLE	desc;
	#endif
	private:
		desc		readend;
		desc		writend;

	public:
		enum PipeSide {
			ReadSide,
			WriteSide,
		};

		class Exception: public std::exception {
		public:
			Exception(const std::string& text);
			~Exception();
		};

		Pipe(desc r = INVALID_HANDLE_VALUE, desc w = INVALID_HANDLE_VALUE);
		~Pipe();


        int	write(const void* buffer, std::streamsize size);
        int	read(void* buffer, std::streamsize size);

		void open() throw (Exception);
		Pipe toReadPipe() ;
		Pipe toWritePipe() ;

		void close() throw (Exception);
		void closeReadEndPoint() throw (Exception);
		void closeWriteEndPoint() throw (Exception);

		const desc& in() const { return readend; }
		const desc& out() const { return writend; }

		bool valid(PipeSide s) const;
	};
}

#endif
/*
$Log: Pipe.h,v $
Revision 1.1.2.4  2005/04/06 16:09:20  francisandre
Add include <pstreams/config.h>

Revision 1.1.2.3  2004/10/01 07:08:43  francisandre
add RCS variables: $Id and $Log

*/