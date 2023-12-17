#ifndef _UGLYKIDJOE_MATH_H_
#define _UGLYKIDJOE_MATH_H_

#include "uglykidjoe.h"
typedef struct v2
{
    real32 x;
    real32 y;
}v2;

extern inline v2 V2(real32 A, real32 B)
{
    v2 result;
    result.x = A;
    result.y = B;
    return(result);
}

extern inline v2 v2_add(v2 A, v2 B)
{
    v2 result;
    result.x = A.x + B.x;
    result.y = A.y + B.y;
    return(result);
}

extern inline v2 v2_sub(v2 A, v2 B)
{
    v2 result;
    result.x = A.x - B.x;
    result.y = A.y - B.y;
    return(result);
}

extern inline v2 v2_neg(v2 A)
{
    v2 result;
    result.x = -A.x;
    result.y = -A.y;
    return(result);
}

extern inline v2 v2_scalar_mul(v2 A, real32 S)
{
    v2 result;
    result.x = A.x * S;
    result.y = A.y * S;

    return(result);
}

extern inline v2 v2_scalar_add(v2 A, real32 S)
{
    v2 result;
    result.x = A.x + S;
    result.y = A.y + S;

    return(result);
}

extern inline real32 square(real32 A)
{
    real32 result = {};
    result = A*A;
    return(result);
}

extern inline real32 inner(v2 A, v2 B)
{
    real32 result = {};
    result = (A.x * B.x) + (A.y * B.y);
    return(result);
}

extern inline real32 length_sq(v2 A)
{
    real32 result = inner(A, A);
    return(result);
}

#endif
