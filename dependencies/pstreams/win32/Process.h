#ifndef	_Process_h_
#define _Process_h_
/*
$Id: Process.h,v 1.1.2.5 2005/04/06 16:09:21 francisandre Exp $
*/
#include <pstreams/config.h>

#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
	#ifdef	DWORD
	#undef	DWORD
	#endif

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

#include <list>
#include <vector>
#include <string>
#include <stdexcept>
using namespace std;

namespace redi {
	typedef	std::vector<std::string>		Arguments;
	typedef	std::vector<std::string>		Environment;

	class  PSTREAMS_API Process {
	private:
		bool				started;
		Arguments			arguments;
		Environment			environment;
		int					error;
		std::string			command;
		std::string			program;

#if		defined(REDI_OS_UNIX)
		pid_t				ppid;
		int					status_;
#elif	defined(REDI_OS_WIN32)
		PROCESS_INFORMATION pid;
		STARTUPINFO         startup;
		SECURITY_ATTRIBUTES	psec;	//process level
		SECURITY_ATTRIBUTES	tsec;	//thread level
		DWORD				status_;
		char*				args;
		char*				env;
		size_t				envlen;
		size_t				arglen;
#endif

	private:
		void init();

	public:
		enum ChildState {
			UNDEF		= -1,
			ACTIVE		= 0,
			EXITED		= 1,
		};
		
		class Exception: public exception {
		public:
			Exception(const std::string& text);
			~Exception();
		};
		Process();
		Process(const std::string& command);
		Process(const std::string& program, Arguments args);
	#if		defined(REDI_OS_UNIX)
	#elif	defined(REDI_OS_WIN32)
		Process(const std::string& command, void* in, void* out, void* err);
		Process(const std::string& program, Arguments args, void* in, void* out, void* err);
	#endif

		void setArguments(const Arguments& arguments);
		void addArgument(const std::string& arg);
		void addArgument(const char* arg);
		void addArgument(const std::vector<std::string> arguments);
		void addVariable(const std::string& var, const std::string& value);

		bool spawn();
		bool active() const { return started; };
		bool stop();
		ChildState state();
		ChildState wait(bool nohang = true, int milli = INFINITE);
		
		int	status() { return status_; }
		int getLastError() const;

	private:
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		void start() throw();
#endif

	};
}
#endif
/*
$Log: Process.h,v $
Revision 1.1.2.5  2005/04/06 16:09:21  francisandre
Add include <pstreams/config.h>

Revision 1.1.2.4  2004/11/08 15:31:51  francisandre
Fix Process(const string& file, const vector<string> args)

Revision 1.1.2.3  2004/10/01 07:08:43  francisandre
add RCS variables: $Id and $Log

*/