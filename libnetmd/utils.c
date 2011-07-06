#include "utils.h"

inline int min(int a,int b)
{
    if (a < b) {
        return a;
    }

    return b;
}

inline int proper_to_bcd(int value)
{
    int high, low;

    high = (value / 10) & 0xf;
    low = (value % 10) & 0xf;

    return (high << 4) + low;
}

inline int bcd_to_proper(int value)
{
    int high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return (high * 10) + low;
}
