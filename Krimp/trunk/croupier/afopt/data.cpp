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
/*----------------------------------
File     : data.cpp
Contents : data set management
----------------------------------*/

#include "../clobal.h"

#include <vector>
#include <algorithm>
using namespace std;

#include <db/Database.h>

#include "data.h"
#include "fsout.h"

using namespace Afopt;

Transaction::Transaction(const Transaction &tr)
{
  length = tr.length;
  count = tr.count;
  t = new int[tr.length];
  memcpy(t, tr.t, sizeof(uint32) * length);
	//for(int i=0; i< length; i++)
    //t[i] = tr.t[i];
}

Data::Data(Database *db, Output *out)
{
  out->setParameters(db->HasBinSizes());
  out->printParameters();
  row = 0;
  numRows = db->GetNumRows();
  rows = db->GetRows();
  values = new uint32[db->GetMaxSetLength()];
}

Data::~Data()
{
	delete[] values;
}

Transaction *Data::getNextTransaction()
{
	if(row == numRows) {
		row = 0;
		return NULL;
	}

	ItemSet *is = rows[row];
	is->GetValuesIn(values);
	int len = is->GetLength();

	// put items in Transaction structure
	Transaction *t = new Transaction(len,is->GetUsageCount());
	memcpy(t->t, values, sizeof(uint32) * len);
	//for(int i=0; i<len; i++)
	//	t->t[i] = values[i];

	++row;

	return t;
}
