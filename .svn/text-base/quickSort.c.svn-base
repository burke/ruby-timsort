#include <stdio.h>
#include <stdlib.h>

#include "quickSort.h"

static int32_t qCompCb(const void *aElem1, const void *aElem2)
{
    if (*(MY_TYPE *)aElem1 < *(MY_TYPE *)aElem2)
    {
        return -1;
    }
    else if (*(MY_TYPE *)aElem1 > *(MY_TYPE *)aElem2)
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

void quickSort(MY_TYPE aArray[], uint32_t aElementCnt, myCmpFunc *aCmpCb)
{
    aCmpCb = aCmpCb;

    qsort(aArray, aElementCnt, sizeof(MY_TYPE), qCompCb);
}

