#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <bits/stdc++.h>

#include "includes/cpu.hpp"
using namespace std;

void read_file(CPU* cpu, char* filename){
    FILE *file;
    uint8_t *buffer;
    unsigned long fileLen;

    file = fopen(filename, "rb");
    if(!file){
        cerr<<"Unable to open file "<<filename<<endl;
    }

    fseek(file, 0, SEEK_END);
    fileLen = ftell(file);

    fseek(file, 0, SEEK_SET);
    // Allocating Memory
    
    buffer = (uint8_t *)malloc(fileLen+1);

    if(!buffer) {
        cerr<<"Memory Error"<<endl;
        fclose(file);
    }
    // Reading file contents
    fread(buffer, fileLen, 1, file);
    fclose(file);

    for(int i = 0; i < fileLen;  i+=2){
        if(i%16 == 0) printf("\n%.8x: ", i);
        printf("%02x%02x ", *(buffer +i), *(buffer+i+1));
    }
    cout<<endl;

    // Copying bin executable to dram
    memcpy(cpu->bus.dram.mem, buffer, fileLen*sizeof(uint8_t));
    free(buffer);
}
uint64_t get_symbol_address(const char* elf_file, const char* symbol_name) {
    std::string command = "riscv64-unknown-elf-nm ";
    command += elf_file;
    command += " | grep ' ";
    command += symbol_name;
    command += "'";

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) {
        std::cerr << "Failed to run nm command.\n";
        return 0;
    }

    char buffer[256];
    std::string result;
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result += buffer;
    }
    pclose(pipe);

    if (result.empty()) {
        std::cerr << "Symbol not found: " << symbol_name << std::endl;
        return 0;
    }

    std::istringstream iss(result);
    std::string addr_str;
    iss >> addr_str;

    uint64_t addr = std::stoull(addr_str, nullptr, 16);
    return addr;
}


int main(int argc, char* argv[]){
    if (argc != 2) {
        cout<<"Usage: rvemu <filename>"<<endl;
        exit(1);
    }

    struct CPU cpu;
    cpu_init(&cpu);

    // Read input file
    read_file(&cpu, argv[1]);

    while(1) {
        uint32_t inst = cpu_fetch(&cpu);

        cpu.pc += 4;

        if(!cpu_execute(&cpu, inst)){
            cout<<"Invalid Instruction found" << inst << endl;
            break;
        }

        dump_registers(&cpu);

        // cout<<"YES"<<endl;
        cout<<"The pc is: "<<cpu.pc<<endl;

        if(cpu.pc == 0){
            break;
        }
        // cout<<"First iteration succesfull" <<endl;
    }
    return 0;

}

