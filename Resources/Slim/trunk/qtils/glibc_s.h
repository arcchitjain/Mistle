#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

#ifndef __GLIBC_S_H
#define __GLIBC_S_H

#if defined (__APPLE__) && defined (__MACH__)
typedef int error_t;
#endif

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

#endif // (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

