#include <errno.h>
#include <stdio.h>
#include <sys/time.h>
#include <math.h>

#include "heapSortLibc.h"

#include "timSort.h"
#include "quickSort.h"
#include "mergeSortLibc.h"

/*
 * -----------------------------------------------------------------------------
 *  Configure Structure
 * -----------------------------------------------------------------------------
 */
typedef struct perfContext
{
    int32_t      mDoVerify;
    char        *mFileName;

    int32_t      mCount;            /* read from input file */

    uint32_t    *mArrayToSort;      /* array to sort */
    uint32_t    *mOriginalArray;    /* original array */

    void       (*mSortFunc)(MY_TYPE [], uint32_t, myCmpFunc *);
} perfContext;

static void perfContextInit(perfContext *aContext)
{
    aContext->mDoVerify      = -1;
    aContext->mFileName      = NULL;
    aContext->mCount         = -1;

    aContext->mArrayToSort   = NULL;
    aContext->mOriginalArray = NULL;
}

/*
 * -----------------------------------------------------------------------------
 *  Verifying Sorted Array
 * -----------------------------------------------------------------------------
 */
static int32_t verifyArrayIsSorted(uint32_t *aArray, int32_t aCount)
{
    int32_t i;

    for (i = 1; i < aCount; i++)
    {
        if (*(aArray + i - 1) > *(aArray + i)) return -1;
    }

    return 0;
}

/*
 * -----------------------------------------------------------------------------
 *  Compare function to deliver to sorting functions
 * -----------------------------------------------------------------------------
 */
static myCmpRc compareFunc(MY_TYPE aElem1, MY_TYPE aElem2)
{
    if (aElem1 > aElem2)
    {
        return MY_GREATER;
    }
    else if (aElem1 < aElem2)
    {
        return MY_LESS;
    }
    else
    {
        return MY_EQUAL;
    }
}

/*
 * -----------------------------------------------------------------------------
 *  Allocating And Filling Array
 * -----------------------------------------------------------------------------
 */
static int32_t getCountFromFile(FILE *aFileHandle)
{
    int32_t  sCount = 0;
    char     sFirstLine[1024] = {0,};

    if (fgets(sFirstLine, sizeof(sFirstLine), aFileHandle) == NULL)
    {
        if (ferror(aFileHandle) != 0)
        {
            /* error */
            (void)fprintf(stderr, "error : reading file.\n");
            clearerr(aFileHandle);
            exit(1);
        }
        else
        {
            if (feof(aFileHandle) != 0)
            {
                /* eof */
                (void)fprintf(stderr, "%s\n", sFirstLine);
                (void)fprintf(stderr, "eof\n");
                exit(1);
            }
            else
            {
                abort();
            }
        }
    }

    if (sFirstLine[0] != '#')
    {
        (void)fprintf(stderr, "error : invalid file format.\n");
        exit(1);
    }
    else
    {
        char    *sEndPtr = NULL;

        sCount = strtol(sFirstLine + 1, &sEndPtr, 10);

        if (errno == ERANGE)
        {
            (void)fprintf(stderr, "error : the count is out of range.\n");
            exit(1);
        }
        else
        {
            if (*sEndPtr != '\n')
            {
                (void)fprintf(stderr, "error : invalid file format.\n");
                (void)fprintf(stderr, "endptr : %c\n", *sEndPtr);
                exit(1);
            }
            else
            {
            }
        }
    }

    return sCount;
}

static void createArray(perfContext *aContext)
{
    /*
     * Note : It is ubsurd to allocate a linear memory of such a big size.
     *        Operating system might swap out the memory if the system has not enough memory,
     *        and it might undermine the credibility of this performance test.
     */
    aContext->mOriginalArray = malloc(aContext->mCount);

    if (aContext->mOriginalArray == NULL)
    {
        (void)fprintf(stderr, "error : malloc fail\n");
        exit(0);
    }
    else
    {
    }

    /*
     * Note : It is ubsurd to allocate a linear memory of such a big size.
     *        Operating system might swap out the memory if the system has not enough memory.
     *        and it might undermine the credibility of this performance test.
     */
    aContext->mArrayToSort = malloc(aContext->mCount);

    if (aContext->mOriginalArray == NULL)
    {
        (void)fprintf(stderr, "error : malloc fail\n");
        exit(0);
    }
    else
    {
    }
}

static void destroyArray(uint32_t *sArray)
{
    free(sArray);
}

static void fillArray(FILE *aFileHandle, perfContext *aContext)
{
    int32_t  i;
    uint32_t sNumber;

    for (i = 0; i < aContext->mCount; i++)
    {
        if (fscanf(aFileHandle, "%u\n", &sNumber) == EOF)
        {
            (void)fprintf(stderr, "error : value count does not match\n");
            exit(1);
        }
        else
        {
        }

        aContext->mOriginalArray[i] = sNumber;
    }
}

static void createAndFillArray(perfContext *aContext)
{
    FILE *sFileHandle      = NULL;

    sFileHandle = fopen(aContext->mFileName, "r");

    if (sFileHandle == NULL)
    {
        if (errno == ENOENT)
        {
            (void)fprintf(stderr, "File not found.\n");
            exit(1);
        }
        else
        {
            (void)fprintf(stderr, "Unexpected error. %s (errno %d)\n", strerror(errno), errno);
            exit(1);
        }
    }
    else
    {
    }

    aContext->mCount = getCountFromFile(sFileHandle);

    createArray(aContext);

    fillArray(sFileHandle, aContext);

    exit(0);
}

/*
 * -----------------------------------------------------------------------------
 *  Processing Command Line Arguments
 * -----------------------------------------------------------------------------
 */
static void printUsageAndExit(char *aProgramName)
{
    (void)fprintf(stderr, "Usage : %s [ -v ] <input_file_name>\n"
                          "    if -v is specified, the program verifies sorted array.\n", 
                          aProgramName);
    exit(1);
}

static void processArg(int32_t aArgc, char *aArgv[], perfContext *aContext)
{
    if (aArgc == 3)
    {
        /*
         * There is an option specified.
         */
        if (strcmp(aArgv[1], "-v") == 0)
        {
            aContext->mDoVerify = 1;
        }
        else
        {
            printUsageAndExit(aArgv[0]);
        }

        aContext->mFileName = aArgv[2];
    }
    else if (aArgc == 2)
    {
        /*
         * No option specified
         */
        aContext->mFileName = aArgv[1];
    }
    else
    {
        /*
         * Invalid number of arguments.
         */
        printUsageAndExit(aArgv[0]);
    }

    if (aContext->mDoVerify < 0) aContext->mDoVerify = 0; /* do not verify unless -v is provided */
}

/*
 * -----------------------------------------------------------------------------
 *  Main
 * -----------------------------------------------------------------------------
 */
int32_t main(int32_t aArgc, char *aArgv[])
{
    perfContext     sContext;

    struct timeval  sStart, sEnd;
    int32_t         sSeconds, sUseconds;

    uint32_t       *sArray;

    perfContextInit(&sContext);

    processArg(aArgc, aArgv, &sContext);

    createAndFillArray(&sContext);

    (void)gettimeofday(&sStart, NULL);

    (*sContext.mSortFunc)((MY_TYPE *)sArray, sContext.mCount, compareFunc);

    (void)gettimeofday(&sEnd, NULL);
    (void)fprintf(stderr, "Completed sorting...\n");

    sSeconds  = sEnd.tv_sec - sStart.tv_sec;
    sUseconds = sEnd.tv_usec - sStart.tv_usec;

    if (sUseconds < 0)
    {
        sSeconds--;
        sUseconds += 1000000;
    }

    (void)fprintf(stderr, "\nIt took %d.%06d seconds to sort %d integers.\n\n",
                  sSeconds, sUseconds, sContext.mCount);

    if (sContext.mDoVerify == 1)
    {
        (void)fprintf(stderr, "Checking if resulting array is correctly sorted...... ");

        if (verifyArrayIsSorted(sArray, sContext.mCount) == 0)
        {
            (void)fprintf(stderr, "OK\n");
        }
        else
        {
            (void)fprintf(stderr, "FAIL\n");
        }
    }

    destroyArray(sArray);

    return 0;
}
