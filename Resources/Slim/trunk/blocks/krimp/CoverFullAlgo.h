#ifndef __COVERFULLALGO_H
#define __COVERFULLALGO_H

/*
	:: CoverFull ::
		Naive cover strategy: try to cover each db row using each element c from CodeTable iteratively

	CFCodeTable	  -  CoverFull
		The main, brute-forcish, full cover strategy
		Pre- and Post-Prune compatible

	CFOCodeTable  -  CoverFull Orderly
		For testing code table orders
		Pre- and Post-Prune compatible

	CFMCodeTable  -  CoverFull Minimal
		Cover each row from DB up till the new code table element. 
		IF it has actually been used, also cover using the rest of the code table.
		Pre- and Post-Prune compatible

	CFMZCodeTable  -  CoverFull Minimal ZeroSkip
		Considered legacy-algorithm since CoverPartial.
		Similar to CFMCodeTable, but up till the new element skips code table elements of count zero.
		Useless (as in, slower) in combination with prune strategies.

	CFACodeTable  -  CoverFull Alternative
		Playground for editing the CF algorithm
		
*/

#include "KrimpAlgo.h"

class CodeTable;

class CoverFullAlgo : public KrimpAlgo {
public:
	CoverFullAlgo(CodeTable *ct);
	virtual ~CoverFullAlgo();

protected:

};

#endif // __COVERFULLALGO_H
