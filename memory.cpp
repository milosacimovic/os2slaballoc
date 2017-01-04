//File: memory.cpp
#include <init.h>

bool is_power_of_two(unsigned long x){
  return ((x != 0) && !(x & (x - 1)));
}

unsigned long next_power_of_two(unsigned long n){
    unsigned long p = 1;
    if (is_power_of_two(n))
        return n;
 
    while (p < n) {
        p <<= 1;
    }
    return p;
}