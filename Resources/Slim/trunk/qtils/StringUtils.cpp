
#include "qlobal.h"

#include "StringUtils.h"

string StringUtils::Trim(const string &input, const string &trim, bool trim_leading /* = true */, bool trim_trailing /* = true */) {
	size_t first = 0;
	size_t last = input.length() - 1;

	if(trim_leading)
		first = input.find_first_not_of(trim);
	if(trim_trailing)
		last = input.find_last_not_of(trim);
	if(first == string::npos || last == string::npos)
		return string();
	else
		return input.substr(first,last-first+1);
}

string* StringUtils::Tokenize(const string &input, uint32 &numtokens, const string &delimiters, const string &trim) {
	// first count number of tokens
	numtokens = StringUtils::CountTokens(input, delimiters);
	string *tokens = new string[numtokens];

	size_t pos = 0, newpos, first;
	for(uint32 curtok = 0; curtok < numtokens; curtok++) {
		newpos = input.find_first_of(delimiters, pos);
		string token = input.substr(pos, newpos-pos);
		first = token.find_first_not_of(trim);
		tokens[curtok] = token.substr(first,token.find_last_not_of(trim)-first+1);
		pos = newpos+1;
	}

	return tokens;
}

double* StringUtils::TokenizeDouble(char *input, uint32 &numtokens, const string &delimiters, const string &trim) {
	// first count number of tokens
	numtokens = StringUtils::CountTokens(input, delimiters);
	double *tokens = new double[numtokens];
	char* bp = input;
	char *bpN;
	string inputstr = input;

	size_t pos = 0, newpos, first;
	for(uint32 curtok = 0; curtok < numtokens; curtok++) {
		newpos = inputstr.find_first_of(delimiters, pos);
		string token = inputstr.substr(pos, newpos-pos);
		first = token.find_first_not_of(trim);

		tokens[curtok] = strtod(bp,&bpN);
		bp = bpN + 1;
		
		pos = newpos+1;
	}

	return tokens;
}

float* StringUtils::TokenizeFloat(char *input, uint32 &numtokens, const string &delimiters, const string &trim) {
	// first count number of tokens
	numtokens = StringUtils::CountTokens(input, delimiters);
	float *tokens = new float[numtokens];
	char* bp = input;
	char *bpN;
	string inputstr = input;

	size_t pos = 0, newpos, first;
	for(uint32 curtok = 0; curtok < numtokens; curtok++) {
		newpos = inputstr.find_first_of(delimiters, pos);
		string token = inputstr.substr(pos, newpos-pos);
		first = token.find_first_not_of(trim);

		tokens[curtok] = (float) strtod(bp,&bpN);
		bp = bpN + 1;

		pos = newpos+1;
	}

	return tokens;
}

uint32* StringUtils::TokenizeUint32(const string &input, uint32 &numtokens, const string &delimiters, const string &trim) {
	// first count number of tokens
	numtokens = StringUtils::CountTokens(input, delimiters);
	uint32 *tokens = new uint32[numtokens];

	size_t pos = 0, newpos, first;
	for(uint32 curtok = 0; curtok < numtokens; curtok++) {
		newpos = input.find_first_of(delimiters, pos);
		string token = input.substr(pos, newpos-pos);
		first = token.find_first_not_of(trim);
		tokens[curtok] = atoi(token.substr(first,token.find_last_not_of(trim)-first+1).c_str());	
		pos = newpos+1;
	}

	return tokens;
}

uint16* StringUtils::TokenizeUint16(const string &input, uint32 &numtokens, const string &delimiters, const string &trim) {
	// first count number of tokens
	numtokens = StringUtils::CountTokens(input, delimiters);
	uint16 *tokens = new uint16[numtokens];

	size_t pos = 0, newpos, first;
	for(uint32 curtok = 0; curtok < numtokens; curtok++) {
		newpos = input.find_first_of(delimiters, pos);
		string token = input.substr(pos, newpos-pos);
		first = token.find_first_not_of(trim);
		tokens[curtok] = atoi(token.substr(first,token.find_last_not_of(trim)-first+1).c_str());	
		pos = newpos+1;
	}

	return tokens;
}


uint32 StringUtils::CountTokens(const string &input, const string &delimiters) {
	uint32 numtokens = 1;	// base count, even if no delimiter is found
	size_t pos = 0, newpos;
	while((newpos = input.find_first_of(delimiters, pos)) != string::npos) {
		numtokens++;
		pos = newpos+1;
	}
	return numtokens;
}
