#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <stdint.h>

#define MY_DEFAULT_ELEMENT_CNT  10000000   /* default element count : 10,000,000 */

/*
 * -----------------------------------------------------------------------------
 *  Configure
 * -----------------------------------------------------------------------------
 */
static const char *gPatternName[] = 
{
    "None",
    "random",
    "sinwave1",
    NULL
};

typedef enum
{
    GEN_DATA_PATTERN_NONE,
    GEN_DATA_PATTERN_RANDOM,
    GEN_DATA_PATTERN_SINWAVE1,
    GEN_DATA_PATTERN_MAX,
} genDataPattern;

typedef struct genDataConf
{
    int32_t         mCount;
    genDataPattern  mPattern;
} genDataConf;

static void genDataConfInit(genDataConf *aConf)
{
    aConf->mCount   = -1;
    aConf->mPattern = GEN_DATA_PATTERN_NONE;
}

/*
 * -----------------------------------------------------------------------------
 *  Sine wave type 1
 * -----------------------------------------------------------------------------
 */
static void genDataGenerateSinWave1(uint32_t aSampleCnt, uint32_t aCycleCount)
{
    uint32_t  i;

    double    x = 0;
    double    sValue;
    double    sDelta;

    sDelta = 2 * aCycleCount * M_PI / aSampleCnt;

    for (i = 0; i < aSampleCnt; i++)
    {
        x += sDelta;

        sValue  = (sin(x) + 1) * 100000 * x;

        (void)fprintf(stdout, "%u\n", 0xFFFFFFFF - (uint32_t)sValue);
        // sArray[i] = (uint32_t)sValue;
    }
}

/*
 * -----------------------------------------------------------------------------
 *  Sine wave type 2
 * -----------------------------------------------------------------------------
 */
#if 0
static void genDataGenerateSinWave2(uint32_t aSampleCnt, uint32_t aCycleCount)
{
    uint32_t  i;

    double    x = 0;
    double    sValue;
    double    sDelta;

    sDelta = 2 * aCycleCount * M_PI / aSampleCnt;

    for (i = 0; i < aSampleCnt; i++)
    {
        x += sDelta;

        sValue  = (sin(x) + 1) * 10000000;
        sValue += (cos(M_PI * x) + 1) * 10000000;

        (void)fprintf(stdout, "%u\n", (uint32_t)sValue);
    }
}
#endif

/*
 * -----------------------------------------------------------------------------
 *  Random
 * -----------------------------------------------------------------------------
 */
static void genDataGenerateRandom(uint32_t aSampleCnt)
{
    uint32_t  i;
    char      sRandState[256];

    (void)initstate(time(NULL), sRandState, 256);

    for (i = 0; i < aSampleCnt; i++)
    {
        (void)fprintf(stdout, "%u\n", (uint32_t)random());
    }
}

/*
 * -----------------------------------------------------------------------------
 *  Error handling routine
 * -----------------------------------------------------------------------------
 */
static void genDataPrintUsageAndExit(char *aProgramName)
{
    uint32_t i = 1;

    (void)fprintf(stderr, "Usage : %s [ options ]\n"
                          "  -c NUM      element count\n"
                          "  -p PATETERN\n", aProgramName);

    while (gPatternName[i] != NULL)
    {
        (void)fprintf(stderr, "        %s\n", gPatternName[i]);
        i++;
    }

    exit(1);
}

/*
 * -----------------------------------------------------------------------------
 *  Process command line arguments
 *      -c num : count
 *      -p pattern
 * -----------------------------------------------------------------------------
 */
static void processArg(int32_t aArgc, char *aArgv[], genDataConf *aConf)
{
    int32_t i;

    for (i = 1; i < aArgc; i++)
    {
        if (strcmp(aArgv[i], "-c") == 0)
        {
            /* -c : the number of samples */
            if (aConf->mCount < 0)
            {
                if (i + 1 < aArgc)
                {
                    int32_t  sCount;
                    char    *sEndPtr = NULL;
                    sCount = strtol(aArgv[i + 1], &sEndPtr, 10);
                    if (errno == ERANGE)
                    {
                        (void)fprintf(stderr, "error : the value provided with "
                                              "'%s' is out of range.\n", aArgv[i]);
                    }
                    else
                    {
                        if (*sEndPtr != '\0')
                        {
                            (void)fprintf(stderr, "error : option '%s' only accepts integer.\n", aArgv[i]);
                            exit(1);
                        }
                    }

                    aConf->mCount = sCount;
                    i++;
                }
                else
                {
                    (void)fprintf(stderr, "error : option '%s' needs to be "
                                          "provided with a value.\n", aArgv[i]);
                    exit(1);
                }
            }
            else
            {
                (void)fprintf(stderr, "error : an option cannot be specified more than once.\n");
                exit(1);
            }
        }
        else if (strcmp(aArgv[i], "-p") == 0)
        {
            if (aConf->mPattern == GEN_DATA_PATTERN_NONE)
            {
                if (i + 1 < aArgc)
                {
                    uint32_t j;

                    i++;

                    for (j = 1; j < GEN_DATA_PATTERN_MAX; j++)
                    {
                        if (strcmp(aArgv[i], gPatternName[j]) == 0)
                        {
                            aConf->mPattern = j;
                        }
                        else
                        {
                        }
                    }
                }
                else
                {
                    (void)fprintf(stderr, 
                                  "error : option '%s' needs to be "
                                  "provided with a source data pattern.\n", aArgv[i]);
                    (void)fprintf(stderr,
                                  "        available source data patterns are : "
                                  "random, ascending, descending, saw");
                    exit(1);
                }
            }
            else
            {
                (void)fprintf(stderr, "error : an option cannot be specified more than once.\n");
                exit(1);
            }
        }
        else
        {
            genDataPrintUsageAndExit(aArgv[0]);
        }
    }

    if (aConf->mCount < 0)
    {
        (void)fprintf(stderr, "error : sample count should be provided.\n");
        genDataPrintUsageAndExit(aArgv[0]);
    }
    else
    {
    }

    if (aConf->mPattern == GEN_DATA_PATTERN_NONE)
    {
        (void)fprintf(stderr, "error : data pattern must be provided.\n");
        genDataPrintUsageAndExit(aArgv[0]);
    }
    else
    {
    }
}

int32_t main(int32_t aArgc, char *aArgv[])
{
    genDataConf sConf;

    genDataConfInit(&sConf);
    processArg(aArgc, aArgv, &sConf);

    /*
     * Printing count
     */
    (void)fprintf(stdout, "#%d\n", sConf.mCount);

    switch (sConf.mPattern)
    {
        case GEN_DATA_PATTERN_RANDOM:
            genDataGenerateRandom(sConf.mCount);
            break;

        case GEN_DATA_PATTERN_SINWAVE1:
            genDataGenerateSinWave1(sConf.mCount, 10);
            break;

        case GEN_DATA_PATTERN_NONE:
        case GEN_DATA_PATTERN_MAX:
            abort();
            break;
    }

    return 0;
}

