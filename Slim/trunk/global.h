#ifndef __GLOBAL_H
#define __GLOBAL_H

// global application settings

// global defines, includes, typedefs
//#define _CRT_SECURE_NO_DEPRECATE 

// VS03 wil bm graag boven hebben staan

// switch memleak detection during debug on/off, scheelt veel proc.tijd
#ifdef _DEBUG
# ifndef _MEMLEAK
#  define _MEMLEAK
# endif
#endif


// dont fuck met volgorde!
#ifdef _MEMLEAK
# ifdef _DEBUG
#  ifdef _WINDOWS
#	define _CRTDBG_MAP_ALLOC
#  endif
# endif
#endif

#define _CRT_RAND_S
#include <stdlib.h>
#ifdef _MEMLEAK
# ifdef _DEBUG
#  ifdef _WINDOWS
#	include <crtdbg.h>
#   define DEBUG_NEW new(_NORMAL_BLOCK ,__FILE__, __LINE__)
#  else
#   define DEBUG_NEW new
#  endif
# else
#  define DEBUG_NEW new
# endif
#else
# define DEBUG_NEW new
#endif

#include <stdio.h>
#include <math.h>
#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>
#include <deque>
#if defined (_MSC_VER) 
	#include <hash_map>
#elif defined (__GNUC__)
	#include <unordered_map>
#endif
#include "AlignedAllocator.h"

using std::string;
#if defined (_MSC_VER)
	using stdext::hash_map;
#elif defined (__GNUC__)
	using std::unordered_map;
#endif

// Dit moet -na- STL shit staan
// note-to-mvl: stl is niet cool
#ifdef _MEMLEAK
# ifdef _DEBUG
#	define new DEBUG_NEW
# endif 
#endif

#if _MSC_VER < 1400
/*#define _itoa(a,b,c) itoa(a,b,c)
#define strcat_s(a,b,c) strcat(a,c)
//#define _fopen_s(&a,b,c) a=fopen(b,c)
#define sprintf_s(a,b,c) sprintf(a,c)
#define sprintf_s(a,b,c,d) sprintf(a,c,d)
#define sprintf_s(a,b,c,d,e) sprintf(a,c,d,e)
#define sprintf_s(a,b,c,d,e,f) sprintf(a,c,d,e,f)
#define sprintf_s(a,b,c,d,e,f,g) sprintf(a,c,d,e,f,g)
#define sprintf_s(a,b,c,d,e,f,g,h) sprintf(a,c,d,e,f,g,h)
#define sprintf_s(a,b,c,d,e,f,g,h,i) sprintf(a,c,d,e,f,g,h,i)
#define sprintf_s(a,b,c,d,e,f,g,h,i,j) sprintf(a,c,d,e,f,g,h,i,j)
#define sprintf_s(a,b,c,d,e,f,g,h,i,j,k) sprintf(a,c,d,e,f,g,h,i,j,k)*/
#pragma warning( disable : 4005 )
#endif

#if 0
#define DEBUG(x) x
#else
#define DEBUG(x)
#endif

#if(defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
#include <glibc_s.h>
#define I64d "lu"
#else
#define I64d "I64d"
#endif

#endif // __GLOBAL_H
