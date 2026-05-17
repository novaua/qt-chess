#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(LICHESSCONNECTOR_LIB)
#  define LICHESSCONNECTOR_EXPORT Q_DECL_EXPORT
# else
#  define LICHESSCONNECTOR_EXPORT Q_DECL_IMPORT
# endif
#else
# define LICHESSCONNECTOR_EXPORT
#endif
