//
// Copyright (c) 2005-2012, Matthijs van Leeuwen, Jilles Vreeken, Koen Smets
// All rights reserved.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
// 
// Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
// Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials // provided with the distribution.
// Neither the name of the Universiteit Utrecht, Universiteit Antwerpen, nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
// 
#ifndef _PRIMITIVES_H
#define _PRIMITIVES_H

#define MEGABYTE 1048576
#define KILOBYTE 1024

#if defined(_MSC_VER) && defined(_WINDOWS)

typedef unsigned long long	uint64;
typedef unsigned int		uint32;
typedef unsigned short int	uint16;
typedef unsigned char		uint8;

typedef signed long long	int64;
typedef signed int			int32;
typedef signed short int	int16;
typedef signed char			int8;
	
#ifndef max
#define max(a,b) (((a)>(b))?(a):(b))
#endif
#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

#define INT8_MIN_VALUE		0x80
#define INT8_MAX_VALUE		0x7F
#define UINT8_MIN_VALUE		0x00
#define UINT8_MAX_VALUE		0xFF

#define SBYTE_MIN_VALUE		INT8_MIN_VALUE
#define SBYTE_MAX_VALUE		INT8_MAX_VALUE
#define BYTE_MIN_VALUE		UINT8_MIN_VALUE
#define BYTE_MAX_VALUE		UINT8_MAX_VALUE
#define CHAR_MIN_VALUE		UINT8_MIN_VALUE
#define CHAR_MAX_VALUE		UINT8_MAX_VALUE

#define INT16_MIN_VALUE		0x8000
#define INT16_MAX_VALUE		0x7FFF
#define UINT16_MIN_VALUE	0x0000
#define UINT16_MAX_VALUE	0xFFFF

#define SHORT_MIN_VALUE		INT16_MIN_VALUE
#define SHORT_MAX_VALUE		INT16_MAX_VALUE
#define USHORT_MIN_VALUE	UINT16_MIN_VALUE
#define USHORT_MAX_VALUE	UINT16_MAX_VALUE

#define INT32_MIN_VALUE		0x80000000
#define INT32_MAX_VALUE		0x7FFFFFFF
#define UINT32_MIN_VALUE	0x00000000
#define UINT32_MAX_VALUE	0xFFFFFFFF

#define INT64_MIN_VALUE		0x8000000000000000
#define INT64_MAX_VALUE		0x7FFFFFFFFFFFFFFF
#define UINT64_MIN_VALUE	0x0000000000000000
#define UINT64_MAX_VALUE	0xFFFFFFFFFFFFFFFF

#define LONG_MIN_VALUE		INT64_MIN_VALUE
#define LONG_MAX_VALUE		INT64_MAX_VALUE
#define ULONG_MIN_VALUE		UINT64_MIN_VALUE
#define ULONG_MAX_VALUE		UINT64_MAX_VALUE

#define FLOAT_MIN_VALUE		-3.402823e38f
#define FLOAT_MAX_VALUE		3.402823e38f

#define DOUBLE_MIN_VALUE	-1.79769313486231e308
#define DOUBLE_MAX_VALUE	1.79769313486231e308

#endif // defined (_WINDOWS)


#if defined (__GNUC__) && defined (__unix__)
	// Fix 'LONG' doest not name a type
	typedef unsigned int LONG;

	// Fix conflict with std::min and std::max
	#include <algorithm>
	using std::min;
	using std::max;

	#include <cstdint>
	typedef uint64_t uint64;
	typedef uint32_t uint32;
	typedef uint16_t uint16;
	typedef uint8_t uint8;

	typedef int64_t int64;
	typedef int32_t int32;
	typedef int16_t int16;
	typedef int8_t int8;

	#define INT16_MIN_VALUE INT16_MIN
	#define INT16_MAX_VALUE INT16_MAX
	#define UINT16_MIN_VALUE 0
	#define UINT16_MAX_VALUE UINT16_MAX

	#define INT32_MIN_VALUE INT32_MIN
	#define INT32_MAX_VALUE INT32_MAX
	#define UINT32_MIN_VALUE 0 
	#define UINT32_MAX_VALUE UINT32_MAX

	#define INT64_MIN_VALUE INT64_MIN
	#define INT64_MAX_VALUE INT64_MAX
	#define UINT64_MIN_VALUE 0
	#define UINT64_MAX_VALUE UINT64_MAX

	#include <cfloat>
	#define DOUBLE_MIN_VALUE DBL_MIN
	#define DOUBLE_MAX_VALUE DBL_MAX

#endif // defined (__GNUC__) && defined (__unix__)

#endif //_PRIMITIVES_H
