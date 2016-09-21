#ifndef _REDI_CONFIG_H_SEEN_
#define _REDI_CONFIG_H_SEEN_

#if defined(_WIN32) || defined(WIN32)
	#define REDI_OS_WIN32
	#ifndef WIN32
		#define WIN32
	#endif
	#if		defined(PSTREAMS_EXPORTS)
		#define PSTREAMS_API __declspec(dllexport)
	#elif	defined(PSTREAMS_IMPORTS)
		#define PSTREAMS_API __declspec(dllimport)
	#else
		#define PSTREAMS_API
	#endif

	#pragma warning(disable: 4290)
	// warning C4290: Spécification d'exception C++ ignorée sauf pour indiquer
	// qu'une fonction n'est pas __declspec(nothrow)
	
	typedef	void* HANDLE;

	#ifdef INVALID_HANDLE_VALUE
		#undef INVALID_HANDLE_VALUE
		#define INVALID_HANDLE_VALUE  ((HANDLE)((LONG_PTR)-1))
	#endif
#else
	#define PSTREAMS_API
	#define REDI_OS_UNIX
#endif
#endif