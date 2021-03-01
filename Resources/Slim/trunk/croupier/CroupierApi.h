#ifndef _CROUPIERAPI_H
#define _CROUPIERAPI_H

#include <Primitives.h>

#ifdef CROUPIER_DYNAMIC
# ifdef CROUPIER_EXPORT
#  define CROUPIER_API __declspec(dllexport)
# else
#  define CROUPIER_API __declspec(dllimport)
# endif
#else
# define CROUPIER_API 
#endif

#pragma warning( disable: 4251 )

#endif //_CROUPIERAPI_H
