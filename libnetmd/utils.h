#ifndef UTILS_H
#define UTILS_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
        unsigned char content[255];
        size_t length;
        size_t position;
} netmd_response;

int min(int a,int b);
unsigned char proper_to_bcd_single(unsigned char value);
unsigned char* proper_to_bcd(unsigned int value, unsigned char* target, size_t len);
unsigned char bcd_to_proper_single(unsigned char value);
unsigned int bcd_to_proper(unsigned char* value, size_t len);


#endif
