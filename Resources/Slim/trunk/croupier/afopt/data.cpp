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
