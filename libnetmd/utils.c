#include "utils.h"

inline int min(int a,int b)
{
    if (a < b) {
        return a;
    }

    return b;
}

inline char proper_to_bcd_single(int value)
{
    int high, low;

    low = (value % 10) & 0xf;
    high = ((value / 10) % 10) & 0xf;

    return (high << 4) + low;
}

inline char* proper_to_bcd(int value, char* target, size_t len)
{
    while (value > 0 && len > 0) {
        target[len - 1] = proper_to_bcd_single(value);
        value /= 100;
        len--;
    }

    return target;
}

inline int bcd_to_proper_single(char value)
{
    int high, low;

    high = (value & 0xf0) >> 4;
    low = (value & 0xf);

    return (high * 10) + low;
}

inline int bcd_to_proper(char* value, size_t len)
{
    int result = 0;
    int nibble_value = 1;

    for (; len > 0; len--) {
        result += nibble_value * bcd_to_proper_single(value[len - 1]);

        nibble_value *= 100;
    }

    return result;
}
