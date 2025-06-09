#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <bits/stdc++.h>
using namespace std;

#include "../includes/cpu.hpp"
#include "../includes/bus.hpp"
#include "../includes/opcodes.hpp"
// #include "../includes/csr.hpp"

#define ANSI_YELLOW  "\x1b[33m"
#define ANSI_BLUE    "\x1b[31m"
#define ANSI_RESET   "\x1b[0m"

// print operation for DEBUG
void print_op(const char* s) {
    printf("%s%s%s", ANSI_BLUE, s, ANSI_RESET);
}
void cpu_init(CPU* cpu){
    cpu->regs[0] = 0x00;  //zero
    cpu->regs[2] = DRAM_BASE + DRAM_SIZE; //stack pointer
    cpu->regs[3] = 0x80001828;  // Register x3 = gp
    for(int i = 4; i < 32; i++){
        cpu->regs[i] = 0;
    }

    cpu->pc = DRAM_BASE;  // Initializing PC
}

uint32_t cpu_fetch(CPU* cpu){
    uint32_t inst = bus_load(&(cpu->bus), cpu->pc, 32);
    printf("Instruction Fetched \n");
    return inst;
}

uint64_t cpu_load(CPU* cpu, uint64_t addr, uint64_t size) {
    return bus_load(&(cpu->bus), addr, size);
}

void cpu_store(CPU* cpu, uint64_t addr, uint64_t size, uint64_t value) {
    bus_store(&(cpu->bus), addr, size, value);
}

// Instruction Decoding

uint64_t rd(uint32_t inst) {
    return (inst >> 7) & 0x1f;
}

uint64_t rs1(uint32_t inst) {
    return (inst >> 15) & 0x1f;
}

uint64_t rs2(uint32_t inst) {
    return (inst >> 20) & 0x1f;
}

// Instruction Type Decoding

uint64_t imm_I(uint32_t inst) {
    return ((int64_t)(int32_t) (inst & 0xfff00000)) >> 20;
}

uint64_t imm_S(uint32_t inst) {
    // imm[11:5] = inst[31:25], imm[4:0] = inst[11:7]
    return ((int64_t)(int32_t)(inst & 0xfe000000) >> 20)
        | ((inst >> 7) & 0x1f); 
}
uint64_t imm_B(uint32_t inst) {
    // imm[12|10:5|4:1|11] = inst[31|30:25|11:8|7]
    return ((int64_t)(int32_t)(inst & 0x80000000) >> 19)
        | ((inst & 0x80) << 4) // imm[11]
        | ((inst >> 20) & 0x7e0) // imm[10:5]
        | ((inst >> 7) & 0x1e); // imm[4:1]
}
uint64_t imm_U(uint32_t inst) {
    // imm[31:12] = inst[31:12]
    return (int64_t)(int32_t)(inst & 0xfffff000);
}
uint64_t imm_J(uint32_t inst) {
    // imm[20|10:1|11|19:12] = inst[31|30:21|20|19:12]
    return (uint64_t)((int64_t)(int32_t)(inst & 0x80000000) >> 11)
        | (inst & 0xff000) // imm[19:12]
        | ((inst >> 9) & 0x800) // imm[11]
        | ((inst >> 20) & 0x7fe); // imm[10:1]
}
uint32_t shamt(uint32_t inst) {
    // shamt(shift amount) only required for immediate shift instructions
    // shamt[4:5] = imm[5:0]
    return (uint32_t) (imm_I(inst) & 0x1f); // TODO: 0x1f / 0x3f ?
}

// Instruction Execution Functions


// U Type Instruction Execution
void exec_LUI(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (uint64_t)(int64_t)(int32_t)(inst & 0xfffff000);
    print_op("lui\n");
}

void exec_AUIPC(CPU* cpu, uint32_t inst){
     // AUIPC forms a 32-bit offset from the 20 upper bits 
    // of the U-immediate
    // cout<<"AUIPC called"<<endl;
    uint64_t imm = imm_U(inst);
    cpu->regs[rd(inst)] = ((int64_t) cpu->pc + (int64_t) imm) - 4;
    print_op("auipc\n");

}

// Jump Instructions
void exec_JAL(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_J(inst);
    cpu->regs[rd(inst)] = cpu->pc;
    // print_op("JAL-> rd:%ld, pc:%lx\n", rd(inst), cpu->pc);
    cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("jal\n");
    // if (ADDR_MISALIGNED(cpu->pc)) {
    //     fprintf(stderr, "JAL pc address misalligned");
    //     exit(0);
    // }
}

void exec_JALR(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    uint64_t tmp = cpu->pc;
    cpu->pc = (cpu->regs[rs1(inst)] + (int64_t) imm) & 0xfffffffe;
    cpu->regs[rd(inst)] = tmp;
    /*print_op("NEXT -> %#lx, imm:%#lx\n", cpu->pc, imm);*/
    print_op("jalr\n");
    // if (ADDR_MISALIGNED(cpu->pc)) {
    //     fprintf(stderr, "JAL pc address misalligned");
    //     exit(0);
    // }
}

// Branch Instructions

void exec_BEQ(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] == (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("beq\n");
}
void exec_BNE(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] != (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = (cpu->pc + (int64_t) imm - 4);
    print_op("bne\n");
}
void exec_BLT(CPU* cpu, uint32_t inst) {
    /*print_op("Operation: BLT\n");*/
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] < (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("blt\n");
}
void exec_BGE(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if ((int64_t) cpu->regs[rs1(inst)] >= (int64_t) cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("bge\n");
}
void exec_BLTU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if (cpu->regs[rs1(inst)] < cpu->regs[rs2(inst)])
        cpu->pc = cpu->pc + (int64_t) imm - 4;
    print_op("bltu\n");
}
void exec_BGEU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_B(inst);
    if (cpu->regs[rs1(inst)] >= cpu->regs[rs2(inst)])
        cpu->pc = (int64_t) cpu->pc + (int64_t) imm - 4;
    print_op("jal\n");
}

// Load Instructions
void exec_LB(CPU* cpu, uint32_t inst) {
    // load 1 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t)(int8_t) cpu_load(cpu, addr, 8);
    print_op("lb\n");
}
void exec_LH(CPU* cpu, uint32_t inst) {
    // load 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t)(int16_t) cpu_load(cpu, addr, 16);
    print_op("lh\n");
}
void exec_LW(CPU* cpu, uint32_t inst) {
    // load 4 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    // cout<<"RS1 of inst is: "<<rs1(inst)<<endl;
    // cout<<cpu->regs[rs1(inst)]<<" "<<int64_t(imm)<<endl;
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
 if (addr < DRAM_BASE || addr + 4 > DRAM_BASE + DRAM_SIZE) {
        std::cerr << "Invalid memory access at 0x" << std::hex << addr << std::dec << std::endl;
        exit(1);
    }
    cpu->regs[rd(inst)] = (int64_t)(int32_t) cpu_load(cpu, addr, 32);
    print_op("lw\n");
}
void exec_LD(CPU* cpu, uint32_t inst) {
    // load 8 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = (int64_t) cpu_load(cpu, addr, 64);
    print_op("ld\n");
}
void exec_LBU(CPU* cpu, uint32_t inst) {
    // load unsigned 1 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 8);
    print_op("lbu\n");
}
void exec_LHU(CPU* cpu, uint32_t inst) {
    // load unsigned 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 16);
    print_op("lhu\n");
}
void exec_LWU(CPU* cpu, uint32_t inst) {
    // load unsigned 2 byte to rd from address in rs1
    uint64_t imm = imm_I(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu->regs[rd(inst)] = cpu_load(cpu, addr, 32);
    print_op("lwu\n");
}
void exec_SB(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 8, cpu->regs[rs2(inst)]);
    print_op("sb\n");
}
void exec_SH(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 16, cpu->regs[rs2(inst)]);
    print_op("sh\n");
}
void exec_SW(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 32, cpu->regs[rs2(inst)]);
    print_op("sw\n");
}
void exec_SD(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_S(inst);
    uint64_t addr = cpu->regs[rs1(inst)] + (int64_t) imm;
    cpu_store(cpu, addr, 64, cpu->regs[rs2(inst)]);
    print_op("sd\n");
}

void exec_ADDI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    // cout<<"RS1 is "<<rs1(inst)<<endl;
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] + (int64_t) imm;
    print_op("addi\n");
}

void exec_SLLI(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] << shamt(inst);
    print_op("slli\n");
}

void exec_SLTI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < (int64_t) imm)?1:0;
    print_op("slti\n");
}

void exec_SLTIU(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < imm)?1:0;
    print_op("sltiu\n");
}

void exec_XORI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] ^ imm;
    print_op("xori\n");
}

void exec_SRLI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] >> imm;
    print_op("srli\n");
}

void exec_SRAI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = (int32_t)cpu->regs[rs1(inst)] >> imm;
    print_op("srai\n");
}

void exec_ORI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] | imm;
    print_op("ori\n");
}

void exec_ANDI(CPU* cpu, uint32_t inst) {
    uint64_t imm = imm_I(inst);
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] & imm;
    print_op("andi\n");
}

void exec_ADD(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] =
        (uint64_t) ((int64_t)cpu->regs[rs1(inst)] + (int64_t)cpu->regs[rs2(inst)]);
    print_op("add\n");
}

void exec_SUB(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] =
        (uint64_t) ((int64_t)cpu->regs[rs1(inst)] - (int64_t)cpu->regs[rs2(inst)]);
    print_op("sub\n");
}

void exec_SLL(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] << (int64_t)cpu->regs[rs2(inst)];
    print_op("sll\n");
}

void exec_SLT(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < (int64_t) cpu->regs[rs2(inst)])?1:0;
    print_op("slt\n");
}

void exec_SLTU(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (cpu->regs[rs1(inst)] < cpu->regs[rs2(inst)])?1:0;
    print_op("slti\n");
}

void exec_XOR(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] ^ cpu->regs[rs2(inst)];
    print_op("xor\n");
}

void exec_SRL(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] >> cpu->regs[rs2(inst)];
    print_op("srl\n");
}

void exec_SRA(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = (int32_t)cpu->regs[rs1(inst)] >> 
        (int64_t) cpu->regs[rs2(inst)];
    print_op("sra\n");
}

void exec_OR(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] | cpu->regs[rs2(inst)];
    print_op("or\n");
}

void exec_AND(CPU* cpu, uint32_t inst) {
    cpu->regs[rd(inst)] = cpu->regs[rs1(inst)] & cpu->regs[rs2(inst)];
    print_op("and\n");
}
// ecall and ebreak

void exec_ECALL(CPU* cpu, uint32_t inst) {
    cout<<"Ecall called"<<endl;
    exit(1);
}
void exec_EBREAK(CPU* cpu, uint32_t inst) {}

void exec_ECALLBREAK(CPU* cpu, uint32_t inst) {
    if (imm_I(inst) == 0x0)
        exec_ECALL(cpu, inst);
    if (imm_I(inst) == 0x1)
        exec_EBREAK(cpu, inst);
    print_op("ecallbreak\n");
}

int cpu_execute(CPU* cpu, uint32_t inst){
    int opcode = inst & 0x7f;
    int funct3 = (inst >> 12) & 0x7;
    int funct7 = (inst >> 25) & 0x7f;

    cpu->regs[0] = 0;    // Hardwiring regs[0] to zero in every cycle

    //  printf("%s\n%#.8lx -> %s", ANSI_YELLOW, cpu->pc-4, ANSI_RESET); // DEBUG
   printf("The opcode is %#x\n", opcode);
    switch(opcode) {
        //U type instructions
        case LUI: exec_LUI(cpu, inst); break;
        case AUIPC: exec_AUIPC(cpu, inst); break;

        // J-type Instructions
        case JAL: exec_JAL(cpu, inst); break;
        case JALR: exec_JALR(cpu, inst); break;


        // B type Instructions
        case B_TYPE:
            // Branch type is decided based on funct3
            switch(funct3) {
                case BEQ: exec_BEQ(cpu, inst); break;
                case BNE: exec_BNE(cpu, inst); break;
                case BLT: exec_BLT(cpu, inst); break;
                case BGE: exec_BGE(cpu, inst); break;
                case BLTU: exec_BLTU(cpu, inst); break;
                case BGEU: exec_BGEU(cpu, inst); break;

                default: ;

            }
            break;
        // L-type Instructions
        case LOAD:
        //   printf("Load type Instruction found \n");
            switch(funct3) {
                case LB : exec_LB(cpu, inst); break;
                case LH: exec_LH(cpu, inst); break;
                case LW : exec_LW(cpu, inst); break;
                case LD  :  exec_LD(cpu, inst); break;  
                case LBU :  exec_LBU(cpu, inst); break; 
                case LHU :  exec_LHU(cpu, inst); break; 
                case LWU :  exec_LWU(cpu, inst); break; 
                default: ;
            }
            break;
        
        case S_TYPE:
            switch(funct3) {
                case SB : exec_SB(cpu, inst); break;
                case SH : exec_SH(cpu, inst); break;
                case SW : exec_SW(cpu, inst); break;
                case SD : exec_SD(cpu, inst); break;
            }
            break;

        case I_TYPE:  
        //    printf("I_type Instruction found \n");
            switch (funct3) {
                case ADDI:  exec_ADDI(cpu, inst); break;
                case SLLI:  exec_SLLI(cpu, inst); break;
                case SLTI:  exec_SLTI(cpu, inst); break;
                case SLTIU: exec_SLTIU(cpu, inst); break;
                case XORI:  exec_XORI(cpu, inst); break;
                case SRI:   
                    switch (funct7) {
                        case SRLI:  exec_SRLI(cpu, inst); break;
                        case SRAI:  exec_SRAI(cpu, inst); break;
                        default: ;
                    } break;
                case ORI:   exec_ORI(cpu, inst); break;
                case ANDI:  exec_ANDI(cpu, inst); break;
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;
        
        case R_TYPE:  
            switch (funct3) {
                case ADDSUB:
                    switch (funct7) {
                        case ADD: exec_ADD(cpu, inst);
                        case SUB: exec_ADD(cpu, inst);
                        default: ;
                    } break;
                case SLL:  exec_SLL(cpu, inst); break;
                case SLT:  exec_SLT(cpu, inst); break;
                case SLTU: exec_SLTU(cpu, inst); break;
                case XOR:  exec_XOR(cpu, inst); break;
                case SR:   
                    switch (funct7) {
                        case SRL:  exec_SRL(cpu, inst); break;
                        case SRA:  exec_SRA(cpu, inst); break;
                        default: ;
                    }
                case OR:   exec_OR(cpu, inst); break;
                case AND:  exec_AND(cpu, inst); break;
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;
        case CSR:
            switch (funct3) {
                case ECALLBREAK:    exec_ECALLBREAK(cpu, inst); break;
                // case CSRRW  :  exec_CSRRW(cpu, inst); break;  
                // case CSRRS  :  exec_CSRRS(cpu, inst); break;  
                // case CSRRC  :  exec_CSRRC(cpu, inst); break;  
                // case CSRRWI :  exec_CSRRWI(cpu, inst); break; 
                // case CSRRSI :  exec_CSRRSI(cpu, inst); break; 
                // case CSRRCI :  exec_CSRRCI(cpu, inst); break; 
                default:
                    fprintf(stderr, 
                            "[-] ERROR-> opcode:0x%x, funct3:0x%x, funct7:0x%x\n"
                            , opcode, funct3, funct7);
                    return 0;
            } break;


    }
    printf("Instruction Executed");
    return 1;
}

void dump_registers(CPU* cpu) {
    if (!cpu) {
        printf("Error: cpu pointer is NULL\n");
        return;
    }

    const char* abi[32] = {
        "zero", "ra",  "sp",  "gp",
        "tp", "t0",  "t1",  "t2",
        "s0", "s1",  "a0",  "a1",
        "a2", "a3",  "a4",  "a5",
        "a6", "a7",  "s2",  "s3",
        "s4", "s5",  "s6",  "s7",
        "s8", "s9", "s10", "s11",
        "t3", "t4",  "t5",  "t6",
    };

    for (int i = 0; i < 8; i++) {
        printf("   %4s: 0x%016" PRIx64 "  ", abi[i], cpu->regs[i]);
        printf("   %4s: 0x%016" PRIx64 "  ", abi[i + 8], cpu->regs[i + 8]);
        printf("   %4s: 0x%016" PRIx64 "  ", abi[i + 16], cpu->regs[i + 16]);
        printf("   %4s: 0x%016" PRIx64 "\n", abi[i + 24], cpu->regs[i + 24]);
    }
    // printf("Dump registers done succesfully");
}

