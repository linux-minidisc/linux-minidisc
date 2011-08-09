#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>

inline int min(int a,int b);
inline char* proper_to_bcd(int value, char* target, size_t len);
inline int bcd_to_proper(char* value, size_t len);

#endif
