#include "timSort.h"

#define TIM_MERGE_TEMP_ARRAY_SIZE   256
#define TIM_MAX_PENDING_RUN_CNT     85
#define TIM_MIN_GALLOP              7

/*
 * MIN_MERGE : The minimun size that an array will be merged.
 *             Python's timsort : 64
 *             Java implementation : 32
 *             One should test and determine its value.
 */
#define MIN_MERGE   64


typedef struct timSlice
{
    int32_t  mBaseIndex;
    uint32_t mLen;
} timSlice;

typedef struct timMergeState
{
    MY_TYPE   *mArray;  /* pointer to source array */

    /*
     * MergeMem : Memory necessary for merging, used in timMergeLow(), timMergeHigh()
     *            Depending on its size, mMergeMem points to mMergeArray or
     *            allocated memory.
     *
     * mMergeMemSize
     *          unit : the number of element.
     *          actual size of memory = sizeof(MY_TYPE) * mMergeMemSize
     */
    uint32_t   mMergeMemSize;
    MY_TYPE   *mMergeMem;
    MY_TYPE    mMergeArray[TIM_MERGE_TEMP_ARRAY_SIZE];

    uint32_t   mPendingRunCnt;
    timSlice   mPendingRun[TIM_MAX_PENDING_RUN_CNT];

    uint32_t   mMinGallop;

} timMergeState;

static void timMergeStateInit(timMergeState *aState, MY_TYPE *aArray)
{
    aState->mArray         = aArray;
    aState->mMergeMem      = aState->mMergeArray;
    aState->mMergeMemSize  = TIM_MERGE_TEMP_ARRAY_SIZE;
    aState->mPendingRunCnt = 0;
    aState->mMinGallop     = TIM_MIN_GALLOP;
}

/*
 * if aSize < MIN_MERGE, returns aSize
 * else
 * if aSize is a power of 2 then returns MIN_MERGE / 2
 * else
 * returns an integer k where MIN_MERGE / 2 <= k <= MIN_MERGE, such that
 * aSize / k is close to, but strictly less than, an exact power of 2.
 */
static uint32_t timCalcMinRunLen(uint32_t aSize)
{
    uint32_t sBumper = 0;
    uint32_t sMinRun;

    assert(aSize >= 0);

    sMinRun = aSize;

    while (sMinRun >= MIN_MERGE)
    {
        sBumper |= (sMinRun & 1);
        sMinRun >>= 1;
    }

    return sMinRun + sBumper;
}

static void timReverseSlice(MY_TYPE *aArray, uint32_t aIndexLow, uint32_t aIndexHigh)
{
    MY_TYPE sTemp;

    aIndexHigh--;

    while (aIndexLow < aIndexHigh)
    {
        sTemp              = aArray[aIndexLow];
        aArray[aIndexLow]  = aArray[aIndexHigh];
        aArray[aIndexHigh] = sTemp;

        aIndexLow++;
        aIndexHigh--;
    }
}

static uint32_t timCountRunAndMakeAscending(MY_TYPE   *aArray,
                                            int32_t    aIndexLow,
                                            int32_t    aIndexHigh,
                                            myCmpFunc *aCmpCb)
{
    int32_t sIndexCur;

    assert(aIndexLow < aIndexHigh);

    if (aIndexLow + 1 == aIndexHigh)
    {
        return 1;
    }
    else
    {
    }

    /*
     * Check the values of the first two elements of the aArray
     * and determine if it is assencing or descending.
     * And then start checking how long respective patterns go.
     */
    if ((*aCmpCb)(aArray[aIndexLow], aArray[aIndexLow + 1]) == MY_LESS)
    {
        /*
         * The first two elements are in ASCENDING order
         *
         * "ASCENDING" is defined as
         *
         *      a[0] <= a[1] <= a[2] <= ...
         */
        sIndexCur = aIndexLow + 2;

        while (sIndexCur < aIndexHigh)
        {
            if ((*aCmpCb)(aArray[sIndexCur - 1], aArray[sIndexCur]) != MY_GREATER)
            {
                /* <= */
                sIndexCur++;
            }
            else
            {
                /* > */
                break;
            }
        }
    }
    else
    {
        /*
         * The first two elements are in DESCENDING order
         *
         * "DESCENDING" is STRICTLY defined as
         *
         *      a[0] > a[1] > a[2] > ...
         *
         * strictly defining "descending" enables preserving stableness.
         */
        sIndexCur = aIndexLow + 2;

        while (sIndexCur < aIndexHigh)
        {
            if ((*aCmpCb)(aArray[sIndexCur - 1], aArray[sIndexCur]) == MY_GREATER)
            {
                /* > */
                sIndexCur++;
            }
            else
            {
                /* <= */
                break;
            }
        }

        timReverseSlice(aArray, aIndexLow, sIndexCur);
    }

    return (uint32_t)(sIndexCur - aIndexLow);
}

static void timDoBinarySort(MY_TYPE   *aArray,
                            int32_t    aIndexLow,
                            int32_t    aIndexHigh,
                            int32_t    aIndexStart,
                            myCmpFunc *aCmpCb)
{
    MY_TYPE   sPivot;
    int32_t   sLeft;
    int32_t   sRight;
    int32_t   sMiddle;

    int32_t   i;

    assert(aIndexLow <= aIndexStart && aIndexStart <= aIndexHigh);

    if (aIndexLow == aIndexStart) aIndexStart++;

    for (; aIndexStart < aIndexHigh; aIndexStart++)
    {
        sPivot = aArray[aIndexStart];
        sLeft  = aIndexLow;
        sRight = aIndexStart;

        assert(sLeft < sRight);

        /*
         * Invariants :
         *      sPivot >= all in [aIndexLow, sLeft).
         *      sPivot <  all in [sRight, aIndexStart).
         */

        while (sLeft < sRight)
        {
            sMiddle = (sLeft + sRight) >> 1;

            if ((*aCmpCb)(sPivot, aArray[sMiddle]) == MY_LESS)
            {
                sRight = sMiddle;
            }
            else
            {
                sLeft = sMiddle + 1;
            }
        }

        assert(sLeft == sRight);

        /*
         * Slide over to make room
         */
        for (i = aIndexStart;i > sLeft; i--)
        {
            aArray[i] = aArray[i - 1];
        }

        aArray[sLeft] = sPivot;
    }
}

static void timMergeStatePushRun(timMergeState *aState, int32_t aBase, uint32_t aRunLen)
{
    assert(aState->mPendingRunCnt < TIM_MAX_PENDING_RUN_CNT);

    aState->mPendingRun[aState->mPendingRunCnt].mBaseIndex = aBase;
    aState->mPendingRun[aState->mPendingRunCnt].mLen       = aRunLen;
    aState->mPendingRunCnt++;
}

/*
 * Locates the position at which to insert the specified key into the
 * specified sorted range;
 *
 * if the range contains an element equal to key,
 * returns the index of the leftmost equal element.
 *
 * aKey   : the key whose insertion point to search for
 * aArray : the array in which to search
 * aBase  : the index of the first element in the range
 * aLen   : the length of the range
 * aHint  : the index at which to begin the search.
 *          the closer hint is to the result, the faster timSort runs.
 *
 * returns k (0 <= k <= aLen) such that
 *
 *      aArray[aBase + k - 1] < key <= aArray[aBase + k]
 *
 * pretending that aArray[aBase - 1] is minus infinity
 * and aArray[aBase + aLen] is infinity.
 *
 * IOW, key belongs at index aBase + k;
 *
 * or IOW, the first k elements of aArray should precede key,
 * and the last aLen - k should follow it.
 *
 * This is called gallop LEFT because searching direction is from right to LEFT.
 */
static int32_t timGallopLeft(MY_TYPE    aKey,
                             MY_TYPE   *aArray,
                             int32_t    aBase,
                             int32_t    aLen,
                             int32_t    aHint,
                             myCmpFunc *aCmpCb)
{
    int32_t  sOffset;
    int32_t  sLastOffset;
    int32_t  sMaxOffset;
    int32_t  sTemp;
    int32_t  sMiddle;

    assert(aLen > 0 && aHint >= 0 && aHint < aLen);

    sLastOffset = 0;
    sOffset     = 1;

    if ((*aCmpCb)(aKey, aArray[aBase + aHint]) == MY_GREATER)
    {
        /*
         * key > a[b+h]
         * Gallop right until a[b+h+sLastOffset] < key <= a[b+h+sOffset]
         * (a : aArray, b : aBase, h : aHint)
         */
        sMaxOffset = aLen - aHint;

        while (sOffset < sMaxOffset)
        {
            if ((*aCmpCb)(aKey, aArray[aBase + aHint + sOffset]) == MY_GREATER)
            {
                sLastOffset = sOffset;
                sOffset     = (sOffset << 1) + 1;

                /* integer overflow */
                if (sOffset <= 0) sOffset = sMaxOffset;
            }
            else
            {
                break;
            }
        }

        if (sOffset > sMaxOffset) sOffset = sMaxOffset;

        /* Make sOffset relative to aBase */
        sLastOffset += aHint;
        sOffset     += aHint;
    }
    else
    {
        /*
         * key <= a[b+h]
         * Gallop right until a[b+h+sLastOffset] < key <= a[b+h+sOffset]
         * (a : aArray, b : aBase, h : aHint)
         */
        sMaxOffset = aHint + 1;

        while (sOffset < sMaxOffset)
        {
            if ((*aCmpCb)(aKey, aArray[aBase + aHint - sOffset]) == MY_GREATER)
            {
                break;
            }
            else
            {
                sLastOffset = sOffset;
                sOffset     = (sOffset << 1) + 1;

                /* integer overflow */
                if (sOffset <= 0) sOffset = sMaxOffset;
            }
        }

        if (sOffset > sMaxOffset) sOffset = sMaxOffset;

        /* Make sOffset relative to aBase */
        sTemp       = sLastOffset;
        sLastOffset = aHint - sOffset;
        sOffset     = aHint - sTemp;
    }

    assert(-1 <= sLastOffset && sLastOffset < sOffset && sOffset <= aLen);

    /*
     * Now a[b+sLastOffset] < key <= a[b+sOffset].
     * So, key belongs somewhere to the right of sLastOffset,
     * but no farther right than sOffset.
     *
     * Do a binary search with invariant
     *
     *      a[b+sLastOffset-1] < key <= a[b+sOffset].
     */
    sLastOffset++;

    while (sLastOffset < sOffset)
    {
        sMiddle = sLastOffset + ((sOffset - sLastOffset) >> 1);

        if ((*aCmpCb)(aKey, aArray[aBase + sMiddle]) == MY_GREATER)
        {
            /* a[b+m] < key */
            sLastOffset = sMiddle + 1;
        }
        else
        {
            /* key <= a[b+m] */
            sOffset = sMiddle;
        }
    }

    assert(sLastOffset == sOffset);

    return sOffset;
}

/*
 * Like timGallopLeft(), except that if the range contains an element equal to
 * the aKey, timGallopRight() returns the index after the rightmost equal element.
 *
 * aKey   : the key whose insertion point to search for
 * aArray : the array in which to search
 * aBase  : the index of the first element in the range
 * aLen   : the length of the range
 * aHint  : the index at which to begin the search.
 *          the closer hint is to the result, the faster timSort runs.
 *
 * returns k (0 <= k <= aLen) such that
 *
 *      aArray[aBase + k - 1] <= key < aArray[aBase + k]
 *
 * This is called gallop RIGHT because searching direction is from left to RIGHT.
 */
static int32_t timGallopRight(MY_TYPE    aKey,
                              MY_TYPE   *aArray,
                              int32_t    aBase,
                              int32_t    aLen,
                              int32_t    aHint,
                              myCmpFunc *aCmpCb)
{
    int32_t  sOffset;
    int32_t  sLastOffset;
    int32_t  sMaxOffset;
    int32_t  sTemp;
    int32_t  sMiddle;

    assert(aLen > 0 && aHint >= 0 && aHint < aLen);

    sLastOffset = 0;
    sOffset     = 1;

    if ((*aCmpCb)(aKey, aArray[aBase + aHint]) == MY_LESS)
    {
        /*
         * key < a[b+h]
         * Gallop left until a[b+h-sOffset] <= key < a[b+h-sLastOffset]
         * (a : aArray, b : aBase, h : aHint)
         */
        sMaxOffset = aHint + 1;

        while (sOffset < sMaxOffset)
        {
            if ((*aCmpCb)(aKey, aArray[aBase + aHint - sOffset]) == MY_LESS)
            {
                sLastOffset = sOffset;
                sOffset     = (sOffset << 1) + 1;

                /* integer overflow */
                if (sOffset <= 0) sOffset = sMaxOffset;
            }
            else
            {
                break;
            }
        }

        if (sOffset > sMaxOffset) sOffset = sMaxOffset;

        /* Make sOffset relative to aBase */
        sTemp       = sLastOffset;
        sLastOffset = aHint - sOffset;
        sOffset     = aHint - sTemp;
    }
    else
    {
        /*
         * key >= a[b+h]
         * Gallop right until a[b+h+sLastOffset] <= key < a[h+sOffset]
         * (a : aArray, b : aBase, h : aHint)
         */
        sMaxOffset = aLen - aHint;

        while (sOffset < sMaxOffset)
        {
            if ((*aCmpCb)(aKey, aArray[aBase + aHint + sOffset]) == MY_LESS)
            {
                break;
            }
            else
            {
                sLastOffset = sOffset;
                sOffset     = (sOffset << 1) + 1;

                /* integer overflow */
                if (sOffset <= 0) sOffset = sMaxOffset;
            }
        }

        if (sOffset > sMaxOffset) sOffset = sMaxOffset;

        /* Make sOffset relative to aBase */
        sLastOffset += aHint;
        sOffset     += aHint;
    }

    assert(-1 <= sLastOffset && sLastOffset < sOffset && sOffset <= aLen);

    /*
     * Now a[b + sLastOffset] <= key < a[b + sOffset].
     * So, key belongs somewhere to the right of sLastOffset,
     * but no farther right than sOffset.
     *
     * Do a binary search with invariant
     *
     *      a[b+sLastOffset-1] <= key < a[b+sOffset]
     */
    sLastOffset++;

    while (sLastOffset < sOffset)
    {
        sMiddle = sLastOffset + ((sOffset - sLastOffset) >> 1);

        if ((*aCmpCb)(aKey, aArray[aBase + sMiddle]) == MY_LESS)
        {
            /* key < a[b+m] */
            sOffset = sMiddle;
        }
        else
        {
            /* a[b+m] <= key */
            sLastOffset = sMiddle + 1;
        }
    }

    assert(sLastOffset == sOffset);

    return sOffset;
}

static void timMergeFreeMem(timMergeState *aState)
{
    if (aState->mMergeMem != aState->mMergeArray)
    {
        free(aState->mMergeMem);
    }

    aState->mMergeMem     = aState->mMergeArray;
    aState->mMergeMemSize = TIM_MERGE_TEMP_ARRAY_SIZE;
}

static void timMergeGetMem(timMergeState *aState, uint32_t aNeed)
{
    if (aNeed <= aState->mMergeMemSize) return;

    timMergeFreeMem(aState);

    aState->mMergeMem = (MY_TYPE *)malloc(aNeed * sizeof(MY_TYPE));
    assert(aState->mMergeMem != NULL);

    aState->mMergeMemSize = aNeed;
}

/*
 * Merges two adjacent runs in place, in a stable way.
 * The first element of the first run must be greater than the first
 * element of the second run.
 *
 *      a[aBase1] > a[aBase2]
 *
 * and the last element of the first run must be greater than
 * all elements of the second run.
 *
 *      a[aBase1 + aLen1] > every a[aBase2 .. aBase2 + aLen2]
 *
 *      IOW, a[aBase1 + aLen1] is the maximum.
 *
 *      timGallopRight() and timGallopLeft() called just before
 *      timMergeLow() or timMergeHigh() is called creates this condition.
 *
 * For performance, this should be called only when
 *
 *      aLen1 <= aLen2;
 *
 * its counterpart, timMergeHigh() should be called if aLen1 >= aLen2.
 * Either can be called if aLen1 == aLen2.
 *
 *            sCursor1
 *                |
 *                |- - - >
 *                V
 *                +-------------+
 *           sTmp |  RUN1_copy  |
 *                +-------------+
 *                ^
 *                |copy
 *   sDestIndex  /
 *          |   /       sCursor2
 *          |- - - >      |
 *          | /           |- - - >
 *          V/            V
 *      +---+-------------+---------------------+----------+
 *    a |   |    RUN1     |        RUN2         |          |
 *      +---+-------------+---------------------+----------+
 *          ^             ^                     |
 *          |             |                     |
 *          |<-- aLen1 -->|<------ aLen2 ------>|
 *          |             |
 *        aBase1        aBase2
 *
 * timMergeLow() conducts merge from left to right.
 */
static void timMergeLow(timMergeState *aState,
                        int32_t        aBase1,
                        int32_t        aLen1,
                        int32_t        aBase2,
                        int32_t        aLen2,
                        myCmpFunc     *aCmpCb)
{
    MY_TYPE *sArray = aState->mArray;
    MY_TYPE *sTmp;

    uint32_t sMinGallop;

    int32_t  sCursor1;    /* Indexes into tmp array (run1) */
    int32_t  sCursor2;    /* Indexes into original array. run2 */
    int32_t  sDestIndex;  /* Indexes into original array. merge buffer */

    assert(aLen1 > 0 && aLen2 > 0 && aBase1 + aLen1 == aBase2);

    /*
     * Should always prepare temp memory with size s; s = min(len(run1), len(run2))
     *
     * In MergeLow, aLen1 is always less than aLen2
     */
    timMergeGetMem(aState, aLen1);
    memcpy(aState->mMergeMem, sArray + aBase1, sizeof(MY_TYPE) * aLen1);
    sTmp = aState->mMergeMem;

    sCursor1    = 0;
    sCursor2    = aBase2;
    sDestIndex  = aBase1;

    /*
     * Move first element of second run
     */
    sArray[sDestIndex] = sArray[sCursor2];
    sDestIndex++;
    sCursor2++;
    aLen2--;

    /*
     * Dealing with degenerate cases
     */
    if (aLen2 == 0) goto LABEL_SUCCEED;
    if (aLen1 == 1) goto LABEL_COPY_B;

    sMinGallop = aState->mMinGallop;

    while (1)
    {
        /*
         * Do the straightforward merge until (if ever) one run
         * appears to win consistently
         */
        uint32_t sCount1 = 0;   /* number of times first run won in a row */
        uint32_t sCount2 = 0;   /* number of times second run won in a row */

        do  /* Normal merge : left to right */
        {
            assert(aLen1 > 1 && aLen2 > 0);

            if ((*aCmpCb)(sArray[sCursor2], sTmp[sCursor1]) == MY_LESS)
            {
                sArray[sDestIndex] = sArray[sCursor2];
                sDestIndex++;
                sCursor2++;
                aLen2--;

                sCount1 = 0;
                sCount2++;

                if (aLen2 == 0) goto LABEL_SUCCEED;
            }
            else
            {
                sArray[sDestIndex] = sTmp[sCursor1];
                sDestIndex++;
                sCursor1++;
                aLen1--;

                sCount1++;
                sCount2 = 0;

                if (aLen1 == 1) goto LABEL_COPY_B;
            }
        } while ((sCount1 | sCount2) < sMinGallop); /* if Count1 > 0 then Count2 == 0;
                                                     * if Count2 > 0 then Count1 == 0; */

		/*
         * One run is winning so consistently that galloping may
		 * be a huge win.  So try that, and continue galloping until
		 * (if ever) neither run appears to be winning consistently
		 * anymore.
		 */
        sMinGallop++;
        do
        {
            assert(aLen1 > 1 && aLen2 > 0);
            sMinGallop -= sMinGallop > 1;
            aState->mMinGallop = sMinGallop;

            sCount1 = timGallopRight(sArray[sCursor2],  /* key */
                                     sTmp,              /* array */
                                     sCursor1,          /* base */
                                     aLen1,             /* len */
                                     0,                 /* hint */
                                     aCmpCb);

            if (sCount1 != 0)
            {
                memcpy(sArray + sDestIndex, sTmp + sCursor1, sizeof(MY_TYPE) * sCount1);
                sDestIndex += sCount1;
                sCursor1   += sCount1;
                aLen1      -= sCount1;
                if (aLen1 == 1) goto LABEL_COPY_B;
                if (aLen1 == 0) goto LABEL_SUCCEED;
            }

            sArray[sDestIndex] = sArray[sCursor2];
            sDestIndex++;
            sCursor2++;
            aLen2--;

            if (aLen2 == 0) goto LABEL_SUCCEED;

            /* - - - - - C u t  H e r e - - - - - */

            sCount2 = timGallopLeft(sTmp[sCursor1],
                                    sArray,
                                    sCursor2,
                                    aLen2,
                                    0,
                                    aCmpCb);

            if (sCount2 != 0)
            {
                /* src and dst may overlap, so we should call memmove instead of memcpy */
                memmove(sArray + sDestIndex, sArray + sCursor2, sizeof(MY_TYPE) * sCount2);
                sDestIndex += sCount2;
                sCursor2   += sCount2;
                aLen2      -= sCount2;
                if (aLen2 == 0) goto LABEL_SUCCEED;
            }

            sArray[sDestIndex] = sTmp[sCursor1];
            sDestIndex++;
            sCursor1++;
            aLen1--;

            if (aLen1 == 1) goto LABEL_COPY_B;

        } while (sCount1 >= TIM_MIN_GALLOP || sCount2 >= TIM_MIN_GALLOP);

        sMinGallop++;   /* penalize it for leaving galloping mode */
        aState->mMinGallop = sMinGallop;
    }

LABEL_SUCCEED:

    if (aLen1 > 0)
    {
        memcpy(sArray + sDestIndex, sTmp + sCursor1, sizeof(MY_TYPE) * aLen1);
    }

    return;

LABEL_COPY_B:
    assert(aLen1 == 1 && aLen2 > 0);

    /* The last element of the first run belongs at the end of the merge */
    memmove(sArray + sDestIndex, sArray + sCursor2, sizeof(MY_TYPE) * aLen2);
    sArray[sDestIndex + aLen2] = sTmp[sCursor1];

    return;
}

/*
 * Just same as timMergeLow(), except that this one should be called only if
 *
 *      aLen1 >= aLen2;
 *
 * The first element of the first run must be greater than the first
 * element of the second run.
 *
 *      a[aBase1] > a[aBase2]
 *
 * and the last element of the first run must be greater than
 * all elements of the second run.
 *
 *      a[aBase1 + aLen1] > every a[aBase2 .. aBase2 + aLen2]
 *
 *      IOW, a[aBase1 + aLen1] is the maximum.
 *
 *      timGallopRight() and timGallopLeft() called just before
 *      timMergeLow() or timMergeHigh() is called creates this condition.
 *
 * timMergeHigh() conducts merge from right to left.
 *
 *                                                 +-- sCursor2
 *                                           <- - -|
 *                                                 V
 *                                 +---------------+
 *                            sTmp |   RUN2_copy   |
 *                                 +---------------+
 *                                 ^
 *                                 |
 *                 sCursor1 ----+  |copy
 *                              |  |            +---- sDestIndex
 *                        <- - -| /       <- - -|
 *                              V/              V
 *      +---+-------------------+---------------+----------+
 *    a |   |       RUN1        |     RUN2      |          |
 *      +---+-------------------+---------------+----------+
 *          ^                   ^               |
 *          |                   |               |
 *          |<----- aLen1 ----->|<--- aLen2 --->|
 *        aBase1             aBase2
 */
static void timMergeHigh(timMergeState *aState,
                         int32_t        aBase1,
                         int32_t        aLen1,
                         int32_t        aBase2,
                         int32_t        aLen2,
                         myCmpFunc     *aCmpCb)
{
    MY_TYPE *sArray = aState->mArray;
    MY_TYPE *sTmp;

    uint32_t sMinGallop;

    int32_t  sCursor1;    /* Indexes into original array. (run1) */
    int32_t  sCursor2;    /* Indexes into tmp array (run2) */
    int32_t  sDestIndex;  /* Indexes into original array. merge buffer */

    /*
     * Should always prepare temp memory with size s; s = min(len(run1), len(run2))
     * In MergeHigh, aLen2 is always less than aLen1
     */
    timMergeGetMem(aState, aLen2);

    /* Copy second run into temp memory */
    memcpy(aState->mMergeMem, sArray + aBase2, sizeof(MY_TYPE) * aLen2);
    sTmp = aState->mMergeMem;

    sCursor1   = aBase1 + aLen1 - 1;
    sCursor2   = aLen2 - 1;
    sDestIndex = aBase2 + aLen2 - 1;

    /*
     * Move last element of first run
     */
    sArray[sDestIndex] = sArray[sCursor1];
    sDestIndex--;
    sCursor1--;
    aLen1--;

    if (aLen1 == 0) goto LABEL_SUCCEED;
    if (aLen2 == 1) goto LABEL_COPY_A;

    sMinGallop = aState->mMinGallop;

    while (1)
    {
        /*
         * Do the straightforward merge until (if ever) one run
         * appears to win consistently
         */
        uint32_t sCount1 = 0;   /* number of times first run won in a row */
        uint32_t sCount2 = 0;   /* number of times second run won in a row */

        do  /* Normal merge : right to left */
        {
            assert(aLen1 > 0 && aLen2 > 1);

            if ((*aCmpCb)(sTmp[sCursor2], sArray[sCursor1]) == MY_LESS)
            {
                sArray[sDestIndex] = sArray[sCursor1];
                sDestIndex--;
                sCursor1--;
                aLen1--;

                sCount1++;
                sCount2 = 0;

                if (aLen1 == 0) goto LABEL_SUCCEED;
            }
            else
            {
                sArray[sDestIndex] = sTmp[sCursor2];
                sDestIndex--;
                sCursor2--;
                aLen2--;

                sCount1 = 0;
                sCount2++;

                if (aLen2 == 1) goto LABEL_COPY_A;
            }
        } while ((sCount1 | sCount2) < sMinGallop); /* if Count1 > 0 then Count2 == 0;
                                                     * if Count2 > 0 then Count1 == 0; */

		/*
         * One run is winning so consistently that galloping may
		 * be a huge win.  So try that, and continue galloping until
		 * (if ever) neither run appears to be winning consistently
		 * anymore.
		 */
        sMinGallop++;
        do
        {
            assert(aLen1 > 0 && aLen2 > 1);
            sMinGallop -= sMinGallop > 1;
            aState->mMinGallop = sMinGallop;

            sCount1 = timGallopRight(sTmp[sCursor2],    /* key */
                                     sArray,            /* array */
                                     aBase1,            /* base */
                                     aLen1,             /* len */
                                     aLen1 - 1,         /* hint */
                                     aCmpCb);

            sCount1 = aLen1 - sCount1;

            if (sCount1 != 0)
            {
                sDestIndex -= sCount1;
                sCursor1   -= sCount1;
                aLen1      -= sCount1;
                memmove(sArray + sDestIndex + 1, sArray + sCursor1 + 1, sizeof(MY_TYPE) * sCount1);

                if (aLen1 == 0) goto LABEL_SUCCEED;
            }

            sArray[sDestIndex] = sTmp[sCursor2];
            sDestIndex--;
            sCursor2--;
            aLen2--;

            if (aLen2 == 1) goto LABEL_COPY_A;

            /* - - - - - C u t  H e r e - - - - - */

            sCount2 = timGallopLeft(sArray[sCursor1],   /* key */
                                    sTmp,               /* array */
                                    0,                  /* base */
                                    aLen2,              /* len */
                                    aLen2 - 1,          /* hint */
                                    aCmpCb);

            sCount2 = aLen2 - sCount2;

            if (sCount2 != 0)
            {
                sDestIndex -= sCount2;
                sCursor2   -= sCount2;
                aLen2      -= sCount2;
                memcpy(sArray + sDestIndex + 1, sTmp + sCursor2 + 1, sizeof(MY_TYPE) * sCount2);
                if (aLen2 == 1) goto LABEL_COPY_A;
                if (aLen2 == 0) goto LABEL_SUCCEED;
            }

            sArray[sDestIndex] = sArray[sCursor1];
            sDestIndex--;
            sCursor1--;
            aLen1--;

            if (aLen1 == 0) goto LABEL_SUCCEED;

        } while (sCount1 >= TIM_MIN_GALLOP || sCount2 >= TIM_MIN_GALLOP);

        sMinGallop++;   /* penalize it for leaving galloping mode */
        aState->mMinGallop = sMinGallop;
    }

LABEL_SUCCEED:

    if (aLen2 > 0)
    {
        memcpy(sArray + sDestIndex - (aLen2 - 1), sTmp, aLen2 * sizeof(MY_TYPE));
    }

    return;

LABEL_COPY_A:
    assert(aLen2 == 1 && aLen1 > 0);

    sDestIndex -= aLen1;
    sCursor1   -= aLen1;
    memmove(sArray + sDestIndex + 1, sArray + sCursor1 + 1, aLen1 * sizeof(MY_TYPE));
    sArray[sDestIndex] = sTmp[sCursor2];

    return;
}

/*
 * Merges the two runs at stack indices i and i + 1.
 * Run i must be the penultimate or antepenultimate run on the stack.
 * IOW, i must be equal to 
 *
 *       i == PendingRunCnt - 2 or 
 *       i == PendingRuncnt - 3.
 */
static void timMergeAt(timMergeState *aState, uint32_t aWhere, myCmpFunc *aCmpCb)
{
    int32_t sBaseA;
    int32_t sLenA;
    int32_t sBaseB;
    int32_t sLenB;
    int32_t k;

    assert(aState->mPendingRunCnt >= 2);
    assert(aWhere == aState->mPendingRunCnt - 2 || aWhere == aState->mPendingRunCnt - 3);

    sBaseA = aState->mPendingRun[aWhere].mBaseIndex;
    sLenA  = aState->mPendingRun[aWhere].mLen;
    sBaseB = aState->mPendingRun[aWhere + 1].mBaseIndex;
    sLenB  = aState->mPendingRun[aWhere + 1].mLen;

    assert(sLenA > 0 && sLenB > 0);
    assert(sBaseA + sLenA == sBaseB);

    /*
     * Record the length of the combined runs;
     */
    aState->mPendingRun[aWhere].mLen = sLenA + sLenB;

    /*
     * if aWhere is the 3rd-last run now, also slide over
     * the last run (which is not involved in this merge).
     * The current run (aWhere + 1) goes away in this case.
     */
    if (aWhere == aState->mPendingRunCnt - 3)
    {
        aState->mPendingRun[aWhere+1] = aState->mPendingRun[aWhere+2];
    }

    aState->mPendingRunCnt--;

    /*
     * Find where the first element of run2 goes in run1.
     * Prior elements in run1 can be ignored (because they are already in place).
     */
    k = timGallopRight(aState->mArray[sBaseB],
                       aState->mArray,
                       sBaseA,
                       sLenA,
                       0,
                       aCmpCb);
    assert(k >= 0);

    sBaseA += k;
    sLenA  -= k;
    if (sLenA == 0) return;

    /*
     * Find where the last element of run1 goes in run2.
     * Subsequent elements in run2 can be ignored
     * (because they are already in place).
     */
    sLenB = timGallopLeft(aState->mArray[sBaseA + sLenA - 1],
                          aState->mArray,
                          sBaseB,
                          sLenB,
                          sLenB - 1,
                          aCmpCb);
    assert(sLenB >= 0);

    if (sLenB == 0) return;

    /*
     * Merge remaining runs, using tmp array with min(sLenA, sLenB) elements
     *
     * At this point, following invariant holds:
     *
     *      Array[sBaseA + sLenA] is the element with biggest value.
     *      Array[sBaseB]         is the element with smallest value.
     *
     *      |<-------- RUN A ---------->|<-------- RUN B ---------->|
     *      |                           |                           |
     *      +---------------------------+---------------------------+
     *      |   |   |   |   |   |   |MAX|MIN|   |   |   |   |   |   |
     *      +---------------------------+---------------------------+
     *                ^                   ^
     *                |<----- sLenA ----->|
     *              sBaseA              sBaseB
     */
    if (sLenA <= sLenB)
    {
        timMergeLow(aState, sBaseA, sLenA, sBaseB, sLenB, aCmpCb);
    }
    else
    {
        timMergeHigh(aState, sBaseA, sLenA, sBaseB, sLenB, aCmpCb);
    }
}

/*
 * Examines the stack of runs waiting to be merged and merges adjacent runs
 * until the stack invariants are reestablished:
 *
 *     1. PendingRun[i - 3].mLen > PendingRun[i - 2].mLen + PendingRun[i - 1].mLen
 *     2. PendingRun[i - 2].mLen > PendingRun[i - 1].mLen
 *
 * This method is called each time a new run is pushed onto the stack,
 * so the invariants are guaranteed to hold for i < PendingRunCnt upon
 * entry to the method.
 */
static void timMergeCollapse(timMergeState *aState, myCmpFunc *aCmpCb)
{
    uint32_t  n;
    timSlice *sSlice = aState->mPendingRun;

    while (aState->mPendingRunCnt > 1)
    {
        n = aState->mPendingRunCnt - 2;

        if (n > 0 && sSlice[n-1].mLen <= sSlice[n].mLen + sSlice[n+1].mLen)
        {
            if (sSlice[n-1].mLen < sSlice[n+1].mLen)
            {
                n--;
            }
            else
            {
            }

            timMergeAt(aState, n, aCmpCb);
        }
        else if (sSlice[n].mLen <= sSlice[n+1].mLen)
        {
            timMergeAt(aState, n, aCmpCb);
        }
        else
        {
            break;
        }
    }
}

/*
 * Merges all runs on the stack until only one remains.
 */
static void timMergeForceCollapse(timMergeState *aState, myCmpFunc *aCmpCb)
{
    int32_t  n;
    timSlice *sSlice = aState->mPendingRun;

    while (aState->mPendingRunCnt > 1)
    {
        n = aState->mPendingRunCnt - 2;

        if (n > 0 && sSlice[n - 1].mLen < sSlice[n + 1].mLen)
        {
            n--;
        }
        else
        {
        }

        timMergeAt(aState, (uint32_t)n, aCmpCb);
    }
}

void timSort(MY_TYPE *aArray, uint32_t aElementCnt, myCmpFunc *aCmpCb)
{
    timMergeState sState;

    int32_t       sIndexLow  = 0;
    int32_t       sIndexHigh = aElementCnt;

    int32_t       sRemaining = aElementCnt;
    int32_t       sMinRunLen;
    int32_t       sRunLen;

    int32_t       sForcedRunLen;

    assert(aElementCnt <= 0x7fffffff);

    if (sRemaining < 2)
    {
        /* Arrays of size 1 are always sorted. */
        return;
    }
    else
    {
    }

    timMergeStateInit(&sState, aArray);

    sMinRunLen = timCalcMinRunLen(aElementCnt);
    sRemaining = aElementCnt;

    do
    {
        sRunLen = timCountRunAndMakeAscending(aArray, sIndexLow, sIndexHigh, aCmpCb);

        if (sRunLen < sMinRunLen)
        {
            sForcedRunLen = sRemaining <= sMinRunLen ? sRemaining : sMinRunLen;

            /*
             * From sIndexLow to sIndexLow + sRunLen - 1 is already sorted.
             * So we need to start the binary sort from sIndexLow + sRunLen
             */
            timDoBinarySort(aArray,
                            sIndexLow,
                            sIndexLow + sForcedRunLen,
                            sIndexLow + sRunLen,
                            aCmpCb);

            sRunLen = sForcedRunLen;
        }
        else
        {
        }

        /*
         * Push this run onto pending-runs stack, and maybe merge
         */
        timMergeStatePushRun(&sState, sIndexLow, sRunLen);
        timMergeCollapse(&sState, aCmpCb);

        /*
         * Advance to find next run
         */
        sIndexLow  += sRunLen;
        sRemaining -= sRunLen;

    } while (sRemaining != 0);

    assert(sIndexLow == sIndexHigh);

    /*
     * Merge all remaining runs to complete sort
     */
    timMergeForceCollapse(&sState, aCmpCb);

    assert(sState.mPendingRunCnt == 1);
}

