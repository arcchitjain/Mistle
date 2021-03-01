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
#if defined (__unix__)

#ifndef __GLIBC_S_H
#define __GLIBC_S_H

#include <cstdio>
#include <cstdlib>
#include <cstddef>
#include <ctime>
#include <errno.h>

extern "C" {

int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...);
error_t fopen_s(FILE** pFile, const char *filename, const char *mode);
error_t localtime_s(struct tm* _tm, const time_t *time);
int fscanf_s(FILE *stream, const char *format, ...);
int sscanf_s(const char *buffer, const char *format, ...);
char* itoa(int value, char* result, int base);
char* i64toa(int64_t value, char* result, int base);
error_t _itoa_s(int value, char *buffer, size_t sizeInCharacters, int radix);
char *_itoa(int value, char *str, int radix);
char *_i64toa(int64_t value, char *str, int radix);
error_t memcpy_s(void *dest, size_t numberOfElements, const void *src, size_t count);
error_t strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource);
int printf_s(const char *format, ...);
error_t _fcvt_s(char* buffer, size_t sizeInBytes, double value, int count, int *dec, int *sign);
int fprintf_s(FILE *stream, const char *format, ...);
int mkpath(const char *pathname, mode_t mode);

}

#endif // __GLIBC_S_H

#endif // defined (__unix__)

