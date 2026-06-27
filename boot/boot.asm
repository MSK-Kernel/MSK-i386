BITS 32

GLOBAL _start
EXTERN kernel_main


SECTION .multiboot
ALIGN 4

MB_MAGIC     EQU 0x1BADB002
MB_FLAGS     EQU 0x00000003
MB_CHECKSUM  EQU -(MB_MAGIC + MB_FLAGS)

dd MB_MAGIC
dd MB_FLAGS
dd MB_CHECKSUM


SECTION .multiboot2
ALIGN 8

MB2_MAGIC      EQU 0xE85250D6
MB2_ARCH       EQU 0
MB2_LENGTH     EQU mb2_end - mb2_start
MB2_CHECKSUM   EQU -(MB2_MAGIC + MB2_ARCH + MB2_LENGTH)

mb2_start:

dd MB2_MAGIC
dd MB2_ARCH
dd MB2_LENGTH
dd MB2_CHECKSUM

; End tag
dw 0
dw 0
dd 8

mb2_end:


SECTION .text
ALIGN 16

_start:
    cli

    ; Setup stack
    mov esp, stack_top
    mov ebp, esp

    ; Optional: clear direction flag
    cld

    ; Boot into C kernel
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang


SECTION .bss
ALIGN 16

stack_bottom:
    resb 16384        ; 16 KiB stack

stack_top:
