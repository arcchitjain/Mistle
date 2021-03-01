
#include <algorithm>

#include "qlobal.h"

#include "Config.h"

Config::Config( string filename, string delimiter,
			   string comment, string sentry )
			   : mDelimiter(delimiter), mComment(comment), mSentry(sentry), mComboInsts(false), mNumInsts(1), mComboLength(0)
{
	// Construct a Config, getting keys and values from given file
	ReadFile(filename);
}

Config::Config(Config *config) {
	mDelimiter	= config->mDelimiter;
	mComment	= config->mComment;
	mSentry		= config->mSentry;
	mContents	= config->mContents;
	mComboInsts = config->mComboInsts;
	mComboLength = config->mComboLength;
	mNumInsts	= config->mNumInsts;
}

Config::Config()
: mDelimiter( string(1,'=') ), mComment( string(1,'#') ), mSentry( string("EndConfig") ), mComboInsts(false), mNumInsts(1), mComboLength(0)
{
	// Construct a ConfigFile without a file; empty
}


void Config::Remove( const string& key )
{
	// Remove key and its value
	mContents.erase( mContents.find( key ) );
}

void Config::Clear() {
	mContents.clear();
}


bool Config::KeyExists( const string &key ) const
{
	// Indicate whether key is found
	string lcKey = key;
	StrToLower(lcKey);
	mapci p = mContents.find( lcKey );
	return ( p != mContents.end() );
}
bool Config::KeyExists( const char* key ) const
{
	string lcKey = string(key);
	StrToLower(lcKey);
	return KeyExists(lcKey);
}

Config* Config::GetInstance(unsigned inst) {	// here inst is base-0
	Config* instance = new Config(this);
	bool comboUse = false;
	unsigned comboVal = 0;

	for(mapci it = mContents.begin(); it != mContents.end(); ++it) {
		string val = it->second;
		unsigned numVals, useVal;
		if(val[0] == '[' || val[0] == '{') {
			numVals = Config::CountNumVals(val);
			useVal = inst % numVals;

			// val uitkiezen en in instance setten
			if(val[0] == '{' && comboUse == true) {
				useVal = comboVal;
			} else {
				inst = (inst - useVal) / numVals;

				if(val[0] == '{') {
					comboUse = true;
					comboVal = useVal;
				}
			}

			size_t from = 0, to = val.find('|',from), tmp=0, curVal=0;

			while(curVal < useVal && to != val.npos) {
				// andere value dan de huidige ophalen
				from = to;
				to = val.find('|',from+1);
				curVal++;
			}

			if(curVal+1 < useVal) {
				delete instance;
				throw string("Unknown value");
			}

			if(to == val.npos) {
				to = val.length()-1;
			}

			instance->Set(it->first, val.substr(from+1,to-from-1));
		} else
			instance->Set(it->first, it->second);
	}
	return instance;
}

unsigned Config::GetNumInstances() {
	return mNumInsts;
}


/* static */
void Config::Trim( string& s )
{
	// Remove leading and trailing whitespace
	static const char whitespace[] = " \n\t\v\r\f";
	s.erase( 0, s.find_first_not_of(whitespace) );
	s.erase( s.find_last_not_of(whitespace) + 1U );
}

void Config::ReadFile(const string &confName) {
	std::ifstream in( confName.c_str() );

	if( !in.good() ) throw file_not_found( confName ); 

	in >> (*this);
}

void Config::WriteFile(const string &confName) {
	std::ofstream out( confName.c_str() );

	if( !out.good() ) throw file_not_found( confName ); 

	out << (*this);

	out.close();
}

std::ostream& operator<<( std::ostream& os, const Config& cf )
{
	// Save a ConfigFile to os
	for( Config::mapci p = cf.mContents.begin();
		p != cf.mContents.end();
		++p )
	{
		os << p->first << " " << cf.mDelimiter << " ";
		os << p->second << std::endl;
	}
	return os;
}


std::istream& operator>>( std::istream& is, Config& cf )
{
	// Load a ConfigFile from is
	// Read in keys and values, keeping internal whitespace
	typedef string::size_type pos;
	const string& delim  = cf.mDelimiter;  // separator
	const string& comm   = cf.mComment;    // comment
	const string& sentry = cf.mSentry;     // end of file sentry
	const pos skip = delim.length();        // length of separator
	bool blockComment = false;

	string nextline = "";  // might need to read ahead to see where value ends

	while( is || nextline.length() > 0 )
	{
		// Read an entire line at a time
		string line;
		if( nextline.length() > 0 )
		{
			line = nextline;  // we read ahead; use it now
			nextline = "";
		}
		else
		{
			std::getline( is, line );
		}

		// Ignore single line comments
		line = line.substr( 0, line.find(comm) );

		// Check for block comment start/end
		if(!blockComment) {
			size_t pos = line.find("/*");
			if(pos != string::npos) {
				size_t pos2 = line.find("*/");
				if(pos2 == string::npos) {
					line = line.substr( 0, pos );
					blockComment = true;
				} else {
					line = line.substr( 0, pos ) + line.substr( pos2+2 );
				}
			}
		} else {
			size_t pos = line.find("*/");
			if(pos == string::npos) {
				line = ""; // still in block, skip line
			} else {
				line = line.substr( pos+2 );
				blockComment = false;
			}
		}

		// Check for end of file sentry
		if( sentry != "" && line.find(sentry) != string::npos ) break;

		// Parse the line if it contains a delimiter
		pos delimPos = line.find( delim );
		if( delimPos < string::npos )
		{
			// Extract the key
			string key = line.substr( 0, delimPos );
			line.replace( 0, delimPos+skip, "" );

			// See if value continues on the next line
			// Stop at blank line, next line with a key, end of stream,
			// or end of file sentry
			bool terminate = false;
			while( !terminate && is )
			{
				std::getline( is, nextline );
				terminate = true;

				string nlcopy = nextline;
				Config::Trim(nlcopy);
				if( nlcopy == "" ) continue;

				nextline = nextline.substr( 0, nextline.find(comm) );
				if( nextline.find(delim) != string::npos )
					continue;
				if( sentry != "" && nextline.find(sentry) != string::npos )
					continue;

				nlcopy = nextline;
				Config::Trim(nlcopy);
				if( nlcopy != "" ) line += "\n";
				line += nextline;
				terminate = false;
			}

			// Store key and value
			Config::Trim(key);
			Config::Trim(line);
			Config::StrToLower(key);

			cf.mContents[key] = line;  // overwrites if key is repeated
		}
	}

	return is;
}

void Config::DetermineNumInsts() {
	// en nu sweep-instances-uitrekenen over de geladen values
	for( Config::mapci p = mContents.begin(); p != mContents.end(); ++p )	{
		unsigned numVals; 
		if(p->second[0] == '{' || p->second[0] == '[') {
			numVals = Config::CountNumVals(p->second);
			if(p->second[0] != '{' || mComboInsts == false)
				mNumInsts *= numVals;
			if(p->second[0] == '{') {
				if(mComboInsts == false) {
					mComboInsts = true;
					mComboLength = numVals;
				} else if(mComboLength != numVals)
					throw string("Linked Value Group Lengths Differ!");
			} 
		}
	}
}

unsigned Config::CountNumVals(const string &vals) {
	if(vals[0] == '{' || vals[0] == '[') {
		unsigned numLineVals = 1;
		size_t offset = 0;
		while( (offset = vals.find('|',offset)) != vals.npos ) {
			numLineVals++;
			offset++;
		}
		return numLineVals;
	} else
		throw string("Config::CountNumVals - Input does not seem to be a value-list.");
}

/* static */
void Config::StrToLower(string &s) {
	transform(s.begin(), s.end(),    // source
		s.begin(),           // destination
		tolower); 
}
/* static */
void Config::StrToUpper(string &s) {
	transform(s.begin(), s.end(),    // source
		s.begin(),           // destination
		toupper); 
}
