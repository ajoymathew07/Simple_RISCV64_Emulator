#ifndef CPU_HPP
#define CPU_HPP

#include <stdint.h>
#include "bus.hpp"

typedef struct CPU {
    uint64_t regs[32]; // 64 bit register (x0 - x31)
    uint64_t pc;       // Program counter
    uint64_t csr[4069];  // Control and Status Registers

    struct BUS bus;

} CPU;

void cpu_init(struct CPU* cpu);
uint32_t cpu_fetch(struct CPU* cpu);
int cpu_execute(struct CPU* cpu, uint32_t inst);
void dump_registers(struct CPU* cpu);



#endif