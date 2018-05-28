.global _start
_start:
.rept 8192
    nop
.endr
    movq $0,%rdi
    movq $60,%rax
    syscall
