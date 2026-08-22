; @file bignum_bit_test.asm
; @brief x86-64 YASM implementation for querying one bignum bit.
; @details System V AMD64 ABI entry point. Arguments are rdi = const bignum_t*,
; rsi = zero-based bit index, and rdx = int* output. The record layout is
; words[32] at offset 0 and len at offset 256. The routine performs no calls,
; uses no global state, preserves all callee-saved registers, and leaves the
; output unchanged on every error path.
; @version 1.0.0
; @return rax = bignum_bit_test_status_t: 0 success, -1 null argument,
;         -2 out-of-range bit index, -3 invalid len.

section .text

BIGNUM_CAPACITY equ 32
BIGNUM_WORD_BITS equ 64
BIGNUM_TOTAL_BITS equ BIGNUM_CAPACITY * BIGNUM_WORD_BITS
BIGNUM_LEN_OFFSET equ BIGNUM_CAPACITY * 8

SUCCESS equ 0
ERROR_NULL_ARG equ -1
ERROR_INDEX equ -2
ERROR_LENGTH equ -3

global bignum_bit_test
bignum_bit_test:
    ; Validate pointers before dereferencing either argument.
    test    rdi, rdi
    jz      .error_null
    test    rdx, rdx
    jz      .error_null

    ; Reject an index outside the fixed 2048-bit addressable domain.
    cmp     rsi, BIGNUM_TOTAL_BITS
    jae     .error_index

    ; Reject malformed records without touching the output.
    mov     rcx, [rdi + BIGNUM_LEN_OFFSET]
    cmp     rcx, BIGNUM_CAPACITY
    ja      .error_length

    ; Locate the word. A valid index above len is logically zero.
    mov     rcx, rsi
    shr     rcx, 6                  ; rcx = word index
    cmp     rcx, [rdi + BIGNUM_LEN_OFFSET]
    jae     .store_zero

    ; Extract the selected bit. The shift count is guaranteed to be 0..63.
    mov     r8, rsi
    and     r8d, 63                 ; r8d = intra-word offset
    mov     rax, [rdi + rcx * 8]
    mov     ecx, r8d
    shr     rax, cl
    and     eax, 1
    mov     [rdx], eax
    xor     eax, eax
    ret

.store_zero:
    xor     eax, eax
    mov     [rdx], eax
    ret

.error_null:
    mov     eax, ERROR_NULL_ARG
    ret

.error_index:
    mov     eax, ERROR_INDEX
    ret

.error_length:
    mov     eax, ERROR_LENGTH
    ret

section .note.GNU-stack noalloc noexec nowrite progbits
