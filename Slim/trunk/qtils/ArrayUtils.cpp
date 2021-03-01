#include "qlobal.h"

#include "ArrayUtils.h"

#if defined (__GNUC__)
#include "Primitives.h"
#endif

void* ArrayUtils::sMergeSortAuxArr = NULL;
void* ArrayUtils::sMergeSortIdxAuxArr = NULL;
size_t ArrayUtils::sMergeSortAuxArrNumBytes = 0;
size_t ArrayUtils::sMergeSortIdxAuxArrNumBytes = 0;

