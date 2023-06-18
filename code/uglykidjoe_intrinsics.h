#ifndef _UGLYKIDJOE_INTRINSICS_H_
#define _UGLYKIDJOE_INTRINSICS_H_

#include<math.h>
extern inline int32 floor_real32_to_int32(real32 real_32)
{
    int32 result = (int32)floorf(real_32);
    return(result);
}

extern inline uint32 round_real32_to_uint32(real32 real_32)
{
    int32 result = (uint32)roundf(real_32);
    return(result);
}

extern inline int32 round_real32_to_int32(real32 real_32)
{
    int32 result = (int32)roundf(real_32);
    return(result);
}

typedef struct BitScanResult
{
    bool found;
    uint32 index;
}BitScanResult;

internal inline BitScanResult find_least_significant_set_bit(uint32 value)
{
    Assert(value > 0);
    BitScanResult result = {};
#if COMPILER_GCC
    result.index = __builtin_ctz(value);
    result.found = true;
#else
    for(uint32 test = 0;
               test < 32;
               ++test)
    {
        if(value & (1 << test))
        {
            result.index = test;
            result.found = true;
            break;
        }
    }
#endif

    return(result);
}
#endif
