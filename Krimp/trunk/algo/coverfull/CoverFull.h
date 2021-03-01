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
#ifndef __COVERFULL_H
#define __COVERFULL_H

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

#include "../Algo.h"

class CodeTable;

class CoverFull : public Algo {
public:
	CoverFull(CodeTable *ct);
	virtual ~CoverFull();

protected:

};

#endif // __COVERFULL_H
