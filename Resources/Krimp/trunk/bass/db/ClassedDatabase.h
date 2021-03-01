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
#ifndef __CLASSEDDATABASE_H
#define __CLASSEDDATABASE_H

#include "Database.h"

class BASS_API ClassedDatabase : public Database {
public:
	ClassedDatabase();
	ClassedDatabase(ItemSet **itemsets, uint32 numRows, bool binSizes, uint32 numTransactions = 0, uint64 numItems = 0);
	ClassedDatabase(Database *db, bool extractClassLabels = true);
	ClassedDatabase(ClassedDatabase *database);
	~ClassedDatabase();

	Database**			SplitOnClassLabels();
	virtual Database**	SplitOnItemSet(ItemSet *is);

	virtual void		SwapRows(uint32 row1, uint32 row2);

	void				SetClassLabel(uint32 row, uint16 cl) { mClassLabels[row] = cl; }
	void				SetClassLabels(uint16 *classLabels);
	void				ExtractClassLabels();

	uint16*				GetClassLabels()			{ return mClassLabels; }
	uint16				GetClassLabel(uint32 row)	{ return mClassLabels[row]; }
	uint32*				GetClassSupports()			{ return mClassSupports; }

	virtual DatabaseType GetType() { return DatabaseClassed; }

protected:
	uint16*				mClassLabels;
	uint32*				mClassSupports;

};

#endif // __CLASSEDDATABASE_H
