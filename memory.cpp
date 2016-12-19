//File: memory.cpp
#include <slab.h>

int is_power_of_two(int block_num){
  return !(block_num & (block_num - 1));
}
