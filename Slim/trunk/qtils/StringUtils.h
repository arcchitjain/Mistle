#ifndef _STRINGUTILS_H
#define _STRINGUTILS_H

#include "QtilsApi.h"

class QTILS_API StringUtils
{
public:
	static string* Tokenize(const string &input, uint32 &numtokens, const string &delimiters = " ", const string &trim = "");

	static double* TokenizeDouble(char *input, uint32 &numtokens, const string &delimiters = " ", const string &trim = "");
	static float* TokenizeFloat(char *input, uint32 &numtokens, const string &delimiters = " ", const string &trim = "");

	static uint32* TokenizeUint32(const string &input, uint32 &numtokens, const string &delimiters = " ", const string &trim = "");
	static uint16* TokenizeUint16(const string &input, uint32 &numtokens, const string &delimiters = " ", const string &trim = "");

	// Trims leading and trailing characters (given in `trim') from `input'
	static string Trim(const string &input, const string &trim, bool trim_leading = true, bool trim_trailing = true);

	static uint32 CountTokens(const string &input, const string &delimiters);
};

#endif // _STRINGUTILS_H
