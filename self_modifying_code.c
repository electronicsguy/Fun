// Ref: https://www.quora.com/C-programming-language-Can-you-write-a-C-program-to-demonstrate-a-self-modifying-code
// http://stackoverflow.com/questions/1474030/how-can-i-tell-gcc-not-to-inline-a-function
// http://stackoverflow.com/questions/2995251/why-in-c-do-we-use-dword-rather-than-unsigned-int
// https://linux.die.net/man/2/mprotect
// http://stackoverflow.com/questions/14267081/difference-between-je-jne-and-jz-jnz
// http://stackoverflow.com/questions/20381812/mprotect-always-returns-invalid-arguments

// Thanks to Vladislav Zorov for answer on Quora (link above) and
// to Eric Postpischil from stackoverflow for the memory address alignment code

#include <stdio.h>
#include <errno.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stdint.h>

// JZ = JE instruction
#define JZ_OP 0x74
#define NOOP 0x90

// 'volatile' is used to prevent the 'if' check from being removed by
// the compiler optimizing routine
// The 'noinline' attribute is used to prevent inlining of the function
// so that it is actually created and put somewhere in memory
// (which is the purpose of this code :) )
void __attribute__ ((noinline)) hello()
{
    volatile int flag = 0;

    if (flag) {
        printf("Code modified itself!\n");
    }
    else {
        printf("Code unchanged\n");
    }
}

int main(int argc, char* argv[])
{   
    // Get address of code, as byte (char) pointer.
    char* fn = (char *)&hello;
 
    // http://stackoverflow.com/questions/20381812/mprotect-always-returns-invalid-arguments
    // Find page size for this system.
    size_t pagesize = sysconf(_SC_PAGESIZE);
    //printf("Pagesize: %d", pagesize);
    
    // Calculate start and end addresses for the write.
    uintptr_t start = (uintptr_t) &hello;
    uintptr_t end = start + 128;

    // Calculate start of page for mprotect.
    // Aligns pagestart to page boundary
    // How does this work?
    uintptr_t pagestart = start & -pagesize;
    
    // Change memory page protection
    // Note: setting the page to PROT_WRITE alone means you're not allowed to read it anymore
    // This particular page will be in the text segment, so it needs to be PROT_EXEC as well, 
    // or the program will crash the moment it returns from mprotect
    // (because the code on that page is no longer considered executable).
    mprotect((void *) pagestart, end - pagestart, PROT_READ | PROT_WRITE | PROT_EXEC);
    perror("mprotect()");
    
    // Find JZ/JE instruction (opcode 0x74)
    while (*fn != JZ_OP) {
        fn++;
    }

    // Replace by NOOPs.
    // Uncomment these to replace the 'if' statement check in the function hello()
    // with NOOPs
    fn[0] = NOOP;
    fn[1] = NOOP;

    // Make the function call
    hello();

    return 0;
}

/* Code Assembly generated using gcc -S <filename>
See the line "je  .L2" which is replaced
by a NOOP within this program

        .file   "self_modifying_code.c"
        .section        .rodata
.LC0:
        .string "Code modified itself!"
.LC1:
        .string "Code unchanged"
        .text
.globl hello
        .type   hello, @function
hello:
.LFB0:
        .cfi_startproc
        pushq   %rbp
        .cfi_def_cfa_offset 16
        .cfi_offset 6, -16
        movq    %rsp, %rbp
        .cfi_def_cfa_register 6
        subq    $16, %rsp
        movl    $0, -4(%rbp)
        movl    -4(%rbp), %eax
        testl   %eax, %eax
        je      .L2
        movl    $.LC0, %edi
        call    puts
        jmp     .L4
.L2:
        movl    $.LC1, %edi
        call    puts
.L4:
        leave
        .cfi_def_cfa 7, 8
        ret
        .cfi_endproc
.LFE0:
        .size   hello, .-hello
        .section        .rodata
.LC2:
        .string "mprotect()"
        .text
.globl main
*/
