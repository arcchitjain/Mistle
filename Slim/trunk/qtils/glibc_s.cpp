#if (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))

#include "glibc_s.h"

#include <cstdarg>
#include <cstdlib>
#include <cstring>
#include <libgen.h>
#include <sys/stat.h>

// @see: Run-Time Library Reference, sprintf_s, _sprintf_s_l, swprintf_s, _swprintf_s_l
//       http://msdn.microsoft.com/en-us/library/ce3zzk1k%28VS.80%29.aspx
// @see: Linux Programmer's Manual, printf(3)
int sprintf_s(char *buffer, size_t sizeOfBuffer, const char *format, ...) {
	if ((buffer == NULL) || (format == NULL)) {
		errno = EINVAL;
		return -1;
	}
	va_list ap;
	va_start(ap, format);
	int n = vsnprintf(buffer, sizeOfBuffer, format, ap);
	va_end(ap);
	return n;
}

// @see: Run-Time Library Reference, fopen_s, _wfopen_s
//       http://msdn.microsoft.com/en-us/library/z5hh6ee9%28VS.80%29.aspx
// @see: Linux Programmer's Manual, fopen(3)
error_t fopen_s(FILE** pFile, const char *filename, const char *mode) {
	if ((pFile == NULL) || (filename == NULL) || (mode == NULL))
		return EINVAL;
	*pFile = fopen(filename, mode);
	return (*pFile == NULL);
}

// @see: Run-Time Library Reference, localtime_s, _localtime32_s, _localtime64_s
//       http://msdn.microsoft.com/en-us/library/a442x3ye%28VS.80%29.aspx
// @see: Linux Programmer's Manual, ctime(3) 
error_t localtime_s(struct tm* _tm, const time_t *time) {
	if (_tm == NULL) return EINVAL;
	if ((_tm) && ((time == NULL) || (time < 0) /*|| (time > _MAX__TIME64_T)*/)) { // TODO
		_tm->tm_sec = -1;
		_tm->tm_min = -1;
		_tm->tm_hour = -1;
		_tm->tm_mday = -1;
		_tm->tm_year = -1;
		_tm->tm_wday = -1;
		_tm->tm_yday = -1;
		_tm->tm_isdst = -1;
		return EINVAL;
	}
	return (localtime_r(time, _tm) == NULL);
}

// @see: Run-Time Library Reference, fscanf_s, _fscanf_s_l, fwscanf_s, _fwscanf_s_l
//       http://msdn.microsoft.com/en-us/library/6ybhk9kc%28VS.80%29.aspx
// @see: Linux Programmer's Manual, scanf(3)
int fscanf_s(FILE *stream, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int n = vfscanf(stream, format, ap);
	va_end(ap);
	return n;
} 

// @see: Run-Time Library Reference, sscanf_s, _sscanf_s_l, swscanf_s, _swscanf_s_l 
//       http://msdn.microsoft.com/en-us/library/t6z7bya3%28VS.80%29.aspx 
// @see: Linux Programmer's Manual, scanf(3)
int sscanf_s(const char *buffer, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int n = vsscanf(buffer, format, ap);
	va_end(ap);
	return n;
}

// @see: Run-Time Library Reference, memcpy_s, wmemcpy_s   
//       http://msdn.microsoft.com/en-us/library/wes2t00f%28VS.80%29.aspx 
// @see: Linux Programmer's Manual, memcpy(3)
error_t memcpy_s(void *dest, size_t numberOfElements, const void *src, size_t count) {
	if (dest == NULL)
		return EINVAL;
	if (src == NULL)
		return EINVAL;
	if (numberOfElements < count)
		return ERANGE;
	return (memcpy(dest, src, count) == NULL);
}

// @see: Run-Time Library Reference, strcpy_s, wcscpy_s, _mbscpy_s 
//       http://msdn.microsoft.com/en-us/library/td1esda9%28VS.80%29.aspx 
// @see: Linux Programmer's Manual, strcpy(3)
error_t strcpy_s(char *strDestination, size_t numberOfElements, const char *strSource) {
	if (strDestination == NULL)
		return EINVAL;
	if (strSource == NULL)
		return EINVAL;
	if (numberOfElements == 0 /*|| numberOfElements too small*/) // TODO
		return ERANGE;
	return (strcpy(strDestination, strSource) == NULL);
}

// @see: Run-Time Library Reference, printf_s, _printf_s_l, wprintf_s, _wprintf_s_l 
//       http://msdn.microsoft.com/en-us/library/239ffwa0%28VS.80%29.aspx 
// @see: Linux Programmer's Manual, printf(3)
int printf_s(const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int n = vprintf(format, ap);
	va_end(ap);
	return n;
} 

// @see: Run-Time Library Reference, _itoa_s, _i64toa_s, _ui64toa_s, _itow_s, _i64tow_s, _ui64tow_s
//       http://msdn.microsoft.com/en-us/library/0we9x30h%28VS.80%29.aspx
error_t _itoa_s(int value, char *buffer, size_t sizeInCharacters, int radix) {
	if (buffer == NULL)
		return EINVAL;
	if (sizeInCharacters <= 0)
		return EINVAL;
	if (sizeInCharacters <= 0 /*length of the result string required*/) // TODO
		return EINVAL;
	if ((radix < 2) || (radix > 36))
		return EINVAL;
	return (itoa(value, buffer, radix) == NULL);
}

// @see: Run-Time Library Reference, _fcvt_s
//       http://msdn.microsoft.com/en-us/library/ty40w2f3%28VS.80%29.aspx
// @see: Linux Programmer's Manual, ecvt(3)
error_t _fcvt_s(char* buffer, size_t sizeInBytes, double value, int count, int *dec, int *sign) {
	if (buffer == NULL)
		return EINVAL;
	if (sizeInBytes <= 0)
		return EINVAL;
	if (dec == NULL)
		return EINVAL;
	if (sign == NULL);
		return EINVAL;
	char* tmp_buffer = fcvt(value, count, dec, sign);
	return (strncpy(buffer, tmp_buffer, sizeInBytes) == NULL);
}

// see: Run-Time Library Reference, fprintf_s, _fprintf_s_l, fwprintf_s, _fwprintf_s_l
//      http://msdn.microsoft.com/en-us/library/ksf1fzyy%28VS.80%29.aspx
// @see: Linux Programmer's Manual, printf(3)
int fprintf_s(FILE *stream, const char *format, ...) {
	va_list ap;
	va_start(ap, format);
	int n = vfprintf(stream, format, ap);
	va_end(ap);
	return n;
}

// @see: Run-Time Library Reference, _itoa, _i64toa, _ui64toa, _itow, _i64tow, _ui64tow 
//       http://msdn.microsoft.com/en-us/library/yakksftt%28VS.80%29.aspx 
char *_itoa(int value, char *str, int radix) {
	return itoa(value, str, radix);
}

char *_i64toa(int64_t value, char *str, int radix) {
	return i64toa(value, str, radix);
}

/**
 * C++ version 0.4 char* style "itoa":
 * Written by Lukás Chmela
 * Released under GPLv3.
 * @see: http://www.jb.man.ac.uk/~slowe/cpp/itoa.html
 */
char* itoa(int value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }
	
	char* ptr = result, *ptr1 = result, tmp_char;
	int tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

char* i64toa(int64_t value, char* result, int base) {
	// check that the base if valid
	if (base < 2 || base > 36) { *result = '\0'; return result; }
	
	char* ptr = result, *ptr1 = result, tmp_char;
	int64_t tmp_value;

	do {
		tmp_value = value;
		value /= base;
		*ptr++ = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz" [35 + (tmp_value - value * base)];
	} while ( value );

	// Apply negative sign
	if (tmp_value < 0) *ptr++ = '-';
	*ptr-- = '\0';
	while(ptr1 < ptr) {
		tmp_char = *ptr;
		*ptr--= *ptr1;
		*ptr1++ = tmp_char;
	}
	return result;
}

// make parent directories as needed
int mkpath(const char *pathname, mode_t mode) {
	int n = mkdir(pathname, mode);
	if (n) {
		switch (errno) {
		case EEXIST:
			return n;
		default:
			char* path = strdup(pathname);
			char* dir = dirname(path);
			mkpath(dir, mode);
			free(path);
		return mkdir(pathname, mode);
		}
	}
}

#endif // (defined (__unix__) || (defined (__APPLE__) && defined (__MACH__)))
