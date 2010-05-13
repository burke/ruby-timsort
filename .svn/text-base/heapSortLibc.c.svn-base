#include <stdio.h>
#include <stdlib.h>

#include "heapSortLibc.h"

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

void heapSortLibc(MY_TYPE aArray[], uint32_t aElementCnt, myCmpFunc *aCmpCb)
{
    aCmpCb = aCmpCb;

    heapsort(aArray, aElementCnt, sizeof(MY_TYPE), qCompCb);
}



