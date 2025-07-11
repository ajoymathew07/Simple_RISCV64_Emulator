#ifndef BUS_HPP
#define BUS_HPP

#include "dram.hpp"

typedef struct BUS {
    struct DRAM dram;
} BUS;

uint64_t bus_load(BUS* bus, uint64_t addr, uint64_t size);

void bus_store(BUS* bus, uint64_t addr, uint64_t size, uint64_t value);


#endif