#ifdef __PCDEV
#ifndef __PATTERN_H
#define __PATTERN_H

#include "../BassApi.h"

class BASS_API Pattern {
public:
	Pattern();
	Pattern(const Pattern& p);
	virtual ~Pattern();
	virtual Pattern*	Clone() const =0;

	// Calculates (approximately) how much memory the Pattern uses
	virtual uint32		GetMemUsage() const =0;

	virtual bool		Equals(Pattern *p) const =0;

protected:
};
#endif // __PATTERN_H

#endif
