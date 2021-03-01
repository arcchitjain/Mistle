#ifndef _BASSAPI_H
#define _BASSAPI_H

#include <Primitives.h>

#ifdef BASS_DYNAMIC
# ifdef BASS_EXPORT
#     define BASS_API __declspec(dllexport)
#     define BASS_TEMPLATE_API 
# else
#     define BASS_API __declspec(dllimport)
#     define BASS_TEMPLATE_API extern
# endif
#else
# define BASS_API 
# define BASS_TEMPLATE_API 
#endif

#pragma warning( disable: 4251 )

#endif //_BASSAPI_H
