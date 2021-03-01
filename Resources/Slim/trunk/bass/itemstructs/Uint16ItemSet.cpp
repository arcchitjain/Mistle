
#include "../Bass.h"
#include "../isc/ItemTranslator.h"
#include "BAI32ItemSet.h"

#include "Uint16ItemSet.h"

Uint16ItemSet::Uint16ItemSet(const Uint16ItemSet *iset) : ItemSet(*iset) {
	mLength = iset->mLength;
	mSet = new uint16[mLength];
	memcpy(mSet,iset->mSet, sizeof(uint16) * mLength);
}
Uint16ItemSet::Uint16ItemSet(uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mStdLength = stdLen;
	mUsageCount = cnt;
	mPrevUsageCount = cnt;
	mSupport = freq;
	mLength = 0;
	mSet = NULL;
	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;
//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];
}

Uint16ItemSet::Uint16ItemSet(uint16 elem, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mStdLength = stdLen;
	mUsageCount = cnt;
	mPrevUsageCount = cnt;
	mSupport = freq;
	mLength = 1;
	mSet = new uint16[1];
	mSet[0] = elem;

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;
//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];
}

Uint16ItemSet::Uint16ItemSet(const uint16 *set, uint32 setlen, uint32 cnt, uint32 freq, uint32 numOcc, double stdLen, uint32 *occurrences) {
	mStdLength = stdLen;
	mUsageCount = cnt;
	mPrevUsageCount = cnt;
	mSupport = freq;
	mLength = setlen;
	mSet = new uint16[mLength];
	memcpy(mSet, set, sizeof(uint16) * mLength);

	mDBOccurrence = occurrences;
	mNumDBOccurs = numOcc;
//	mUsageFlags = new bool[mNumDBOccurs];
//	mPrevUsageFlags = new bool[mNumDBOccurs];
}

Uint16ItemSet::~Uint16ItemSet() {
	delete[] mSet;
}

Uint16ItemSet *Uint16ItemSet::Clone() const {
	return new Uint16ItemSet(this);
}

uint32 Uint16ItemSet::GetMemUsage() const {
	return sizeof(Uint16ItemSet) + 16 + (mLength * sizeof(uint16));
}
uint32 Uint16ItemSet::MemUsage(const uint32 setLen) {
	return sizeof(Uint16ItemSet) + 16 + (setLen * sizeof(uint16));
}

void Uint16ItemSet::AddItemToSet(uint16 val) {
	if(this->IsItemInSet(val))
		return;
	++mLength;
	uint16 *set = new uint16[mLength];
	uint32 idx = 0;
	uint32 i = 0;
	for(i=0; i<mLength-1; i++)
		if(val > mSet[i])
			set[i] = mSet[i];
		else
			break;
	set[i++] = val;
	for(; i<mLength; i++)
		set[i] = mSet[i-1];
	delete[] mSet;
	mSet = set;
}
void Uint16ItemSet::RemoveItemFromSet(uint16 val) { 
	if(!this->IsItemInSet(val))
		return;
	--mLength;
	uint16 *set = new uint16[mLength];
	uint32 idx = 0;
	for(uint32 i=0; i<=mLength; i++)
		if(mSet[i] != val)
			set[idx++] = mSet[i];
	delete[] mSet;
	mSet = set;
}
void Uint16ItemSet::RemoveDoubleItems() {
	if(mLength <= 1) return;

	uint16 *newValues = new uint16[mLength];
	uint32 newLen = 1;
	newValues[0] = mSet[0];
	for(uint32 i=1; i<mLength; i++)
		if(mSet[i] != newValues[newLen-1])
			newValues[newLen++] = mSet[i];

	if(newLen != mLength) {
		delete[] mSet;
		mLength = newLen;
		mSet = new uint16[mLength];
		memcpy(mSet, newValues, sizeof(uint16) * mLength);
	}
	delete[] newValues;
}

bool Uint16ItemSet::IsItemInSet(uint16 val) const {
	for(uint32 i=0; i<mLength; i++)
		if(mSet[i] >= val)
			return mSet[i] == val;
	return false;
}

int8 Uint16ItemSet::CmpLexicographically(ItemSet *is)  {
	uint32 isLen = is->GetLength();
	uint32 minLen = min(mLength, isLen);
	uint16 *set = ((Uint16ItemSet*) is)->GetSet();
	for(uint32 i=0; i<minLen; i++)
		if(mSet[i] < set[i])
			return -1;
		else if(mSet[i] > set[i])
			return 1;
	if(mLength == isLen)
		return 0;
	return mLength < isLen ? -1 : 1;
}

bool Uint16ItemSet::Equals(ItemSet *is) const {
	if(mLength != is->GetLength())
		return false;

	uint16 *set = ((Uint16ItemSet*) is)->GetSet();
	for(uint32 i=0; i<mLength; i++)
		if(mSet[i] != set[i])
			return false;

	return true;
}

bool Uint16ItemSet::Intersects(ItemSet *is) const {
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	uint16 *set = ((Uint16ItemSet*)is)->GetSet();

	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB]) {
			curB++;
		} else if(mSet[curA] < set[curB]) {
			curA++;
		} else { 						// gelijk, dus intersects
			return true;
		}
	} return false;
}

/* returns the answer to the question whether 'b' is a subset of 'a'.
	 a->IsSubset(b), a = {1,2}, b = {1} -> true
	 a->IsSubset(b), a = {1}, b = {1,2} -> false
*/
bool Uint16ItemSet::IsSubset(ItemSet *is) const {
	uint32 num = 0;
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	if(numB > numA) return false;

	uint16 *bSet = ((Uint16ItemSet*)is)->GetSet();
	while(curA < numA && curB < numB) {
		if(mSet[curA] > bSet[curB]) {
			curB++;
		} else if(mSet[curA] < bSet[curB]) {
			curA++;
		} else {
			num++; curA++; curB++;
		}
	}
	return (num == numB);
}

/* Removes elements in 'is' from our own set */
void Uint16ItemSet::Remove(ItemSet *is) {
	uint32 num = 0;
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	uint16 *bSet = ((Uint16ItemSet*)is)->GetSet();

	// bepaal overlap
	while(curA < numA && curB < numB) {
		if(mSet[curA] > bSet[curB]) {
			curB++;
		} else if(mSet[curA] < bSet[curB]) {
			curA++;
		} else {						// gelijk, dus zap
			num++; curA++; curB++;
		}
	}

	if(num > 0) {			// overlap is groter dan 1, dus we moeten items zappen.
		uint32 cur = 0;
		curA=0; curB=0;
		uint16 *rem = new uint16[mLength - num];

		while(curA < numA && curB < numB) {
			if(mSet[curA] > bSet[curB]) {
				curB++;
			} else if(mSet[curA] < bSet[curB]) {
				rem[cur++] = mSet[curA++];
			} else {	// gelijk
				curA++; curB++;
			}
		}
		if(curA < numA)
			for(; curA<numA; curA++)
				rem[cur++] = mSet[curA];

		delete[] mSet;
		mSet = rem;
		mLength = cur;
	}
}

ItemSet* Uint16ItemSet::Intersection(ItemSet *is) const  {
	uint32 num = 0;
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	uint16 *set = ((Uint16ItemSet*)is)->GetSet();

	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB])
			curB++;
		else if(mSet[curA] < set[curB])
			curA++;
		else {						// gelijk, voeg toe, up a&b
			num++; curA++; curB++;
		}
	}

	uint32 cur = 0;
	curA=0; curB=0;
	uint16 *inter = new uint16[num];
	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB])
			curB++;
		else if(mSet[curA] < set[curB])
			curA++;
		else {	// gelijk
			inter[cur++] = set[curB++];
			curA++;
		}
	}

	Uint16ItemSet *isi = new Uint16ItemSet();
	isi->SetSet(inter,num);
	return isi;
}

ItemSet *Uint16ItemSet::Union(ItemSet *is) const  {
	uint32 num = 0;
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	uint16 *set = ((Uint16ItemSet*)is)->GetSet();

	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB]) {
			num++; curB++;							// voeg B toe
		} else if(mSet[curA] < set[curB]) {
			num++; curA++;							// voeg A toe
		} else {
			num++; curA++; curB++;					// voeg A of B toe
		}
	}
	if(curA < numA)
		num += numA - curA;
	if(curB < numB)
		num += numB - curB;

	uint32 cur = 0; curA=0; curB=0;
	uint16 *uni = new uint16[num];
	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB]) {
			uni[cur] = set[curB];
			cur++;	curB++;
		} else if(mSet[curA] < set[curB]) {
			uni[cur] = mSet[curA];
			cur++;	curA++;
		} else {
			uni[cur] = mSet[curA];
			cur++;	curA++;	curB++;
		}
	}
	if(curA < numA)
		for(uint32 i=curA; i<numA; i++)
			uni[cur++] = mSet[i];
	if(curB < numB)
		for(uint32 i=curB; i<numB; i++)
			uni[cur++] = set[i];

	Uint16ItemSet *isi = new Uint16ItemSet();
	isi->SetSet(uni,num);

	ItemSet::UpdateDBOccurrencesAfterUnion(this, is, isi);
	return isi;
}

void Uint16ItemSet::Unite(ItemSet *is) {
	uint32 num = 0;
	uint32 curA=0, numA = mLength;
	uint32 curB=0, numB = is->GetLength();
	uint16 *set = ((Uint16ItemSet*)is)->GetSet();

	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB]) {
			num++; curB++;							// voeg B toe
		} else if(mSet[curA] < set[curB]) {
			num++; curA++;							// voeg A toe
		} else {
			num++; curA++; curB++;					// voeg 1x toe
		}
	}
	if(curA < numA)
		num += numA - curA;
	if(curB < numB)
		num += numB - curB;

	uint32 cur = 0; curA=0; curB=0;
	uint16 *uni = new uint16[num];
	while(curA < numA && curB < numB) {
		if(mSet[curA] > set[curB]) {
			uni[cur] = set[curB];
			cur++;	curB++;
		} else if(mSet[curA] < set[curB]) {
			uni[cur] = mSet[curA];
			cur++;	curA++;
		} else {
			uni[cur] = mSet[curA];
			cur++;	curA++;	curB++;
		}
	}
	if(curA < numA)
		for(uint32 i=curA; i<numA; i++)
			uni[cur++] = mSet[i];
	if(curB < numB)
		for(uint32 i=curB; i<numB; i++)
			uni[cur++] = set[i];

	delete mSet;

	mSet = uni;
	mLength = num;
}


void Uint16ItemSet::Sort() {
	this->QSortAscending(0, mLength-1);
}

void Uint16ItemSet::TranslateBackward(ItemTranslator *it) {
	uint16 *set = new uint16[mLength];
	for(uint32 i=0; i<mLength; i++)
		set[i] = it->BitToValue(mSet[i]);
	delete[] mSet;
	mSet = set;
}
void Uint16ItemSet::TranslateForward(ItemTranslator *it) {
	uint16 *set = new uint16[mLength];
	for(uint32 i=0; i<mLength; i++)
		set[i] = it->ValueToBit(mSet[i]);
	delete[] mSet;
	mSet = set;
}

void Uint16ItemSet::QSortAscending(int first, int last) {
	if(last > first +1) {
		int pivot = first;
		int left = first + 1;
		int right = last;

		while (left != right-1) {
			if(mSet[left] <= mSet[pivot])
				left++;
			else 
				this->SwapItems(left, right--);
		}
		if(mSet[left] <= mSet[pivot] &&
			mSet[right] <= mSet[pivot])
			left=right+1;
		else if(mSet[left] <= mSet[pivot] && 
			mSet[right] > mSet[pivot]) {
				left++;
				right--;
		} else if (mSet[left] > mSet[pivot] &&
			mSet[right] <= mSet[pivot])
			this->SwapItems(left++, right--);
		else
			right=left-1;

		this->SwapItems(right--, first);
		this->QSortAscending(first, right);
		this->QSortAscending(left, last);
	} else if(last == first+1) {
		if(mSet[last] < mSet[first])
			this->SwapItems(last,first);
	}
}
string Uint16ItemSet::ToString(bool printCount, bool printFreq) {
	string s = "";
	char *buffer = new char[40];
	if(mLength > 0) {
		for(uint32 i=0; i<mLength-1; i++) {
			sprintf_s(buffer, 40, "%d ", mSet[i]);
			s.append(buffer);
		}
		sprintf_s(buffer, 40, "%d", mSet[mLength-1]);
		s.append(buffer);
	}
	if(printCount || printFreq) {
		if(printCount && !printFreq) {
			sprintf_s(buffer, 40, " (%d)", mUsageCount);
		} else if(!printCount && printFreq) {
			sprintf_s(buffer, 40, " (%d)", mSupport);
		} else
			sprintf_s(buffer, 40, " (%d,%d)", mUsageCount, mSupport);
		s.append(buffer);
	}
	delete[] buffer;
	return s;
}
string Uint16ItemSet::ToCodeTableString() {
	string s = "";
	char *buffer = new char[40];
	for(uint32 i=0; i<mLength; i++) {
		sprintf_s(buffer, 40, "%d ", mSet[i]);
		s.append(buffer);
	}
	sprintf_s(buffer, 40, "(%d,%d) ", mUsageCount, mSupport);
	s.append(buffer);
	delete[] buffer;
	return s;
}

uint16* Uint16ItemSet::GetValues() const {
	uint16 *vals = new uint16[mLength];
	memcpy(vals, mSet, mLength * sizeof(uint16));
	return vals;
}

void Uint16ItemSet::GetValuesIn(uint32* vals) const {
	for(uint32 i=0; i<mLength; i++)
		vals[i] = mSet[i];
}
void Uint16ItemSet::GetValuesIn(uint16* vals) const {
	/*for(uint32 i=0; i<mLength; i++)
		vals[i] = mSet[i];*/
	memcpy(vals,mSet,sizeof(uint16) * mLength);
}
void Uint16ItemSet::IncrementValues(uint32* vals, int32 cnt) const {
	for(uint32 i=0; i<mLength; i++)
		vals[mSet[i]] += cnt;
}


void Uint16ItemSet::FillHerUp(uint16 max) {
	if(mLength != max) {
		delete[] mSet;
		mSet = new uint16[max];		
		mLength = max;
	}
	for(uint16 i=0; i<max; i++) {
		mSet[i] = i;
	}
}

void Uint16ItemSet::CleanTheSlate() {
	mLength = 0;
}
