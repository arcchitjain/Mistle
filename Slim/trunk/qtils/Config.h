// Config.h
// Class for reading named values from configuration files
//     Richard J. Wagner     v2.1   24 May 2004  wagnerr@umich.edu
//     Matthijs van Leeuwen  v10.9  9 March 2010 mleeuwen@cs.uu.nl	
// and Jilles Vreeken                            jvreeken@cs.uu.nl
// 

// Copyright (c) 2004 Richard J. Wagner
//       and (c) 2006-2010 Jilles Vreeken & Matthijs van Leeuwen.
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

#ifndef CONFIG_H
#define CONFIG_H

#include "QtilsApi.h"

#include <string>
#include <map>
#include <iostream>
#include <fstream>
#include <sstream>

using std::string;

class QTILS_API Config {
// Data
protected:
	string mDelimiter;  // separator between key and value
	string mComment;    // separator between value and comments
	string mSentry;     // optional string to signal end of file
	std::map<string,string> mContents;  // extracted keys and values

	bool		mComboInsts;
	unsigned	mNumInsts;
	unsigned	mComboLength;
	
	typedef std::map<string,string>::iterator mapi;
	typedef std::map<string,string>::const_iterator mapci;

// Methods
public:
	Config( string filename,
	            string delimiter = "=",
	            string comment = "#",
				string sentry = "EndConfig" );
	Config(Config *config);
	Config();
	
	// Search for key and read value or optional default value
	template<class T> T Read( const string& key ) const;  // call as read<T>
	template<class T> T Read( const string& key, const T& value ) const;
	template<class T> bool ReadInto( T& var, const string& key ) const;
	template<class T> bool ReadInto( T& var, const string& key, const T& value ) const;
	
	// Modify keys and values
	template<class T> void Set( string key, const T& value );
	void Remove( const string& key );
	void Clear();
	
	// Check whether key exists in configuration
	bool KeyExists( const string &key ) const;
	bool KeyExists( const char* key ) const;
	
	// Check or change configuration syntax
	string GetDelimiter() const { return mDelimiter; }
	string GetComment() const { return mComment; }
	string GetSentry() const { return mSentry; }
	string SetDelimiter( const string& s )
		{ string old = mDelimiter;  mDelimiter = s;  return old; }  
	string SetComment( const string& s )
		{ string old = mComment;  mComment = s;  return old; }
	
	// Write or read configuration
	void ReadFile(const string &confName);
	void WriteFile(const string &confName);
	friend std::ostream& operator<<( std::ostream& os, const Config& cf );
	friend std::istream& operator>>( std::istream& is, Config& cf );

	static void StrToLower(string &s);
	static void StrToUpper(string &s);

	Config* GetInstance(unsigned i);
	unsigned GetNumInstances();
	static unsigned CountNumVals(const string &vals);

	void DetermineNumInsts();

protected:
	template<class T> static string T_as_string( const T& t );
	template<class T> static T String_as_T( const string& s );
	static void Trim( string& s );

// Exception types
public:
	struct file_not_found {
		string filename;
		file_not_found( const string& filename_ = string() )
			: filename(filename_) {} };
	struct key_not_found {  // thrown only by T read(key) variant of read()
		string key;
		key_not_found( const string& key_ = string() )
			: key(key_) {} };
};


/* static */
template<class T>
string Config::T_as_string( const T& t )
{
	// Convert from a T to a string
	// Type T must support << operator
	std::ostringstream ost;
	ost << t;
	return ost.str();
}


/* static */
template<class T>
T Config::String_as_T( const string& s )
{
	// Convert from a string to a T
	// Type T must support >> operator
	T t;
	std::istringstream ist(s);
	ist >> t;
	return t;
}


/* static */
template<>
inline string Config::String_as_T<string>( const string& s )
{
	// Convert from a string to a string
	// In other words, do nothing
	return s;
}


/* static */
template<>
inline bool Config::String_as_T<bool>( const string& s )
{
	// Convert from a string to a bool
	// Interpret "false", "F", "no", "n", "0" as false
	// Interpret "true", "T", "yes", "y", "1", "-1", or anything else as true
	bool b = true;
	string sup = s;
	for( string::iterator p = sup.begin(); p != sup.end(); ++p )
		*p = toupper(*p);  // make string all caps
	if( sup==string("FALSE") || sup==string("F") ||
	    sup==string("NO") || sup==string("N") ||
	    sup==string("0") || sup==string("NONE") )
		b = false;
	return b;
}


template<class T>
T Config::Read( const string& key ) const
{
	// Read the value corresponding to key
	string lcKey = key;
	StrToLower(lcKey);
	mapci p = mContents.find(lcKey);
	if( p == mContents.end() ) throw key_not_found(key);
	return String_as_T<T>( p->second );
}


template<class T>
T Config::Read( const string& key, const T& value ) const
{
	// Return the value corresponding to key or given default value
	// if key is not found
	string lcKey = key;
	StrToLower(lcKey);
	mapci p = mContents.find(lcKey);
	if( p == mContents.end() ) return value;
	return String_as_T<T>( p->second );
}


template<class T>
bool Config::ReadInto( T& var, const string& key ) const
{
	// Get the value corresponding to key and store in var
	// Return true if key is found
	// Otherwise leave var untouched
	mapci p = mContents.find(key);
	bool found = ( p != mContents.end() );
	if( found ) var = String_as_T<T>( p->second );
	return found;
}


template<class T>
bool Config::ReadInto( T& var, const string& key, const T& value ) const
{
	// Get the value corresponding to key and store in var
	// Return true if key is found
	// Otherwise set var to given default
	mapci p = mContents.find(key);
	bool found = ( p != mContents.end() );
	if( found )
		var = String_as_T<T>( p->second );
	else
		var = value;
	return found;
}


template<class T>
void Config::Set( string key, const T& value )
{
	// Add a key with given value
	string v = T_as_string( value );
    Trim(key);
	StrToLower(key);
	Trim(v);
	mContents[key] = v;
	return;
}

#endif  // CONFIG_H
