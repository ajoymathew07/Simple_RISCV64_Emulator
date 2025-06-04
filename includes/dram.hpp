#ifndef DRAM_HPP
#define DRAM_HPP

#include<stdint.h>

#define DRAM_SIZE 1024*1024*1

#define DRAM_BASE 0x80000000

typedef struct DRAM {
    uint8_t  mem[DRAM_SIZE];
}
DRAM;

// To read data from an address and dram
uint64_t dram_load(DRAM* dram, uint64_t addr, uint64_t size);


// To store a value into a certain address in DRAM
void dram_store(DRAM* dram, uint64_t addr, uint64_t size, uint64_t value);




#endif