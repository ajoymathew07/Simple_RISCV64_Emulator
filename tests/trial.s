.section .data
value1:   .word 5          # First number
value2:   .word 7          # Second number
result:   .word 0          # Storage for result

    .section .text
    .globl _start
_start:
    la a0, value1          # Load address of value1
    lw t0, 0(a0)           # Load value1 into t0

    la a1, value2          # Load address of value2
    lw t1, 0(a1)           # Load value2 into t1

    add t2, t0, t1         # Add t0 and t1, store in t2

    la a2, result          # Load address of result
    sw t2, 0(a2)           # Store result (t2) into memory

    li a7, 10              # Exit syscall code
    ecall                  # Exit the program
