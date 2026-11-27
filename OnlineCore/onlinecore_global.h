#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(ONLINECORE_LIB)
#  define ONLINECORE_EXPORT Q_DECL_EXPORT
# else
#  define ONLINECORE_EXPORT Q_DECL_IMPORT
# endif
#else
# define ONLINECORE_EXPORT
#endif
