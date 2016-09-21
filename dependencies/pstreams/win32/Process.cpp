/*
$Id: Process.cpp,v 1.1.2.6 2005/04/06 16:09:21 francisandre Exp $
*/
#include <iostream>
#include <cassert>

#include <pstreams/win32/Process.h>
using namespace std;

namespace redi {
	void Process::init() {
		started = false;envlen = 0;arglen = 0;
		error = -1; status_ = UNDEF;

#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		ZeroMemory( &startup, sizeof(startup) );
		startup.cb = sizeof(startup);
		ZeroMemory( &pid, sizeof(pid) );
#endif
	}

	Process::Process() 
	{
		init();
	}
	Process::Process(const string& command) :
	command(command)
	{
		init();
	}
	Process::Process(const string& program, Arguments arguments) :
	program(program),
	arguments(arguments)
	{
		init();
		for(Arguments::const_iterator i = arguments.begin(); i != arguments.end(); i++) {
			arglen += i->size() + 1;
		}
	}
	Process::Process(const string& command, void* in, void* out, void* err) :
	command(command) {
		init();
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		startup.hStdError	= err;
		startup.hStdOutput	= out;
		startup.hStdInput	= in;
		startup.dwFlags |= STARTF_USESTDHANDLES;
		args = NULL; env = NULL;
#endif
	}
	Process::Process(const string& program, Arguments arguments, void* in, void* out, void* err) :
	program(program),
	arguments(arguments)
	{
		init();
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		startup.hStdError	= err;
		startup.hStdOutput	= out;
		startup.hStdInput	= in;
		startup.dwFlags |= STARTF_USESTDHANDLES;
		args = NULL; env = NULL;
		for(Arguments::const_iterator i = arguments.begin(); i != arguments.end(); i++) {
			arglen += i->size() + 1;
		}
#endif
	}
	Process::Exception::Exception(const string& text) :
		exception("Process::Exception: ") {
	}
	Process::Exception::~Exception() {
	}
	void Process::setArguments(const Arguments& args){
		for(Arguments::const_iterator i = args.begin(); i != args.end(); i++) {
			arglen += i->size() + 1;
		}
		arguments.insert(arguments.end(), args.begin(), args.end());
	}
	void Process::addArgument(const string& arg){
		arglen += arg.size() + 1;
		arguments.push_back(arg);
	}
	void Process::addArgument(const char* arg){
		arglen += strlen(arg) + 1;
		arguments.push_back(arg);
	}
	void Process::addArgument(const vector<string> args) {
		for(vector<string>::const_iterator i = args.begin(); i != args.end(); i++) {
			arglen += i->size() + 1;
		}
		arguments.insert(arguments.end(), args.begin(), args.end());
	}
	void Process::addVariable(const string& var, const string& value){
		environment.push_back(var + "=" + value);
		envlen += var.size() + 1 + value.size() + 1;
	}
	void Process::start() throw() {		// Start the child process. 
		const char* appl = NULL;
		if  (!program.empty()) {
			appl = program.c_str();
		}
		if( CreateProcess(appl,					// Application name
			args,								// Application arguments
			&psec,								// Process handle not inheritable. 
			&tsec,								// Thread handle not inheritable. 
			TRUE,								// Set handle inheritance to FALSE. 
			CREATE_DEFAULT_ERROR_MODE,          // No creation flags. 
			env,								// Use parent's environment block. 
			NULL,								// Use parent's starting directory. 
			&startup,							// Pointer to STARTUPINFO structure.
			&pid )								// Pointer to PROCESS_INFORMATION structure.
		) 
		{
			WaitForInputIdle(pid.hProcess, INFINITE);
			CloseHandle(pid.hThread);
			started = true;
		}
	}
	bool Process::spawn() {
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		ZeroMemory( &psec, sizeof(psec) );
		psec.nLength = sizeof(psec);
		psec.bInheritHandle = TRUE;

		ZeroMemory( &tsec, sizeof(tsec) );
		tsec.nLength = sizeof(tsec);
		tsec.bInheritHandle = TRUE;

		if  (command.empty()) {
			if  (!arguments.empty()) {
				args = new char[ program.size() + 1 + arglen + 1];
				LPSTR	argv = args;
			
				args = static_cast<char*>(memcpy(args, program.data(), program.size()));
				args+=program.size();*args++ = ' ';

				for(Arguments::iterator i = arguments.begin(); i != arguments.end(); i++) {
					std::memcpy(args, i->data(), i->size());args+=i->size();*args++=' ';
				}
				*args = 0;
				args = argv;
			}

			if  (!environment.empty()) {

				env = new char[envlen + 1];
				LPSTR	block = env;
				for(Environment::iterator i = environment.begin(); i != environment.end(); i++) {

					std::strcpy(block, i->c_str());block+=i->size() + (size_t)1;
				}
				*block = 0;
			}
		} else {
			args = const_cast<char*>(command.c_str());
		}

		start();
#endif
		return started;
	}
	Process::ChildState Process::wait(bool nohang, int milli) {
		ChildState		exitcode = UNDEF;
#if		defined(REDI_OS_UNIX)
		switch (::waitpid(ppid, &status_, nohang ? WNOHANG : 0)) {
			case 0 :
				// nohang was true and process has not exited
				exitcode = ALIVE;
				break;
			case -1 :
				error = errno;
				break;
			default :
				// process has exited
				ppid_ = 0;
				exitcode = EXITED;
				break;
		}
#elif	defined(REDI_OS_WIN32)
		DWORD	waitresult;
		if  (started) {
			// Wait until child process exits.
			try {
				do{
					waitresult = WaitForSingleObject(pid.hProcess, milli);
				} while(waitresult == WAIT_TIMEOUT);
		        
				switch(waitresult) {
					case WAIT_FAILED:
						exitcode = EXITED;
					break;
					case WAIT_TIMEOUT:
						exitcode = ACTIVE;
					break;
					case  WAIT_OBJECT_0:

						GetExitCodeProcess(pid.hProcess, &status_);

						if  (status_ == STILL_ACTIVE) {
							exitcode = ACTIVE;
						} else {
							exitcode = EXITED;
						}
						CloseHandle( pid.hProcess );
					break;
					default:
						assert(0);
					break;
				}
			} catch(...) {
				cerr << "Process::wait(bool nohang, int milli) : unhandled exception" << endl;
			}
		}
	#endif
			return exitcode;
	}
	Process::ChildState Process::state() {
#if		defined(REDI_OS_UNIX)
#elif	defined(REDI_OS_WIN32)
		DWORD	waitresult;
		waitresult = GetExitCodeProcess(pid.hProcess, &status_);

		if  (status_ == STILL_ACTIVE)  {
			return ACTIVE;
		} else {
			return EXITED;
		}
#endif
	}
	bool Process::stop() {
#if		defined(REDI_OS_UNIX)
		if (::kill(ppid_, signal))
			error = errno;
			return false;
		else {
			// TODO call exited() to check for exit and clean up? leave to user?
			return true;
		}
#elif	defined(REDI_OS_WIN32)
			return true;//FIXME
#endif
	}
	int Process::getLastError() const {
#if		defined(REDI_OS_UNIX)
		return error;
#elif	defined(REDI_OS_WIN32)
		return ::GetLastError();
#endif
	}
}
/*
$Log: Process.cpp,v $
Revision 1.1.2.6  2005/04/06 16:09:21  francisandre
Add include <pstreams/config.h>

Revision 1.1.2.5  2004/11/08 15:31:51  francisandre
Fix Process(const string& file, const vector<string> args)

Revision 1.1.2.4  2004/10/01 07:12:59  francisandre
fix a missing elif

Revision 1.1.2.3  2004/10/01 07:08:43  francisandre
add RCS variables: $Id and $Log

*/
