#ifndef __MY_TYPE_H__
#define __MY_TYPE_H__

#include <stdint.h>

#define MY_TYPE uint32_t

typedef enum
{
    MY_GREATER,
    MY_LESS,
    MY_EQUAL
} myCmpRc;

typedef myCmpRc myCmpFunc(MY_TYPE aElem1, MY_TYPE aElem2);

#endif
