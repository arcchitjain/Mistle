#ifndef __BLOBAL_H
#define __BLOBAL_H

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

#if defined (__GNUC__)
	#include <cstring>
#endif
#include <stdio.h>
#include <math.h>
#include <map>
#include <set>
#include <list>
#include <string>
#include <vector>

using std::string;
using std::vector;

// Dit moet -na- STL shit staan
// note-to-mvl: stl is niet cool
#ifdef _MEMLEAK
# ifdef _DEBUG
#	define new DEBUG_NEW
# endif 
#endif

#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
#include <glibc_s.h>
#define I64d "lu"
#else
#define I64d "I64d"
#endif

#endif // __BLOBAL_H
