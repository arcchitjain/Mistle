#ifndef _QTILSAPI_H
#define _QTILSAPI_H

#include "Primitives.h"

#ifdef QTILS_DYNAMIC
# ifdef QTILS_EXPORT
#  define QTILS_API __declspec(dllexport)
# else
#  define QTILS_API __declspec(dllimport)
# endif
#else
# define QTILS_API 
#endif

#pragma warning( disable: 4251 )

#endif //_QTILSAPI_H
