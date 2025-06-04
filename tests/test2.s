    .section .text
    .globl _start

_start:
    li a0, 42        # a0 = 42
    li a1, 10        # a1 = 10
    add a2, a0, a1   # a2 = a0 + a1 = 52

loop:
    j loop           # infinite loop to stop execution here
