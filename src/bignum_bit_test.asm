; @file bignum_bit_test.asm
; @brief x86-64 YASM implementation for querying one bignum bit.
; @details This symbol is the assembly side of the public C contract. Under the
; System V AMD64 ABI, rdi carries const bignum_t*, rsi carries the zero-based
; size_t bit index, and rdx carries the caller-allocated int* output. The
; bignum representation is 32 little-endian uint64_t words at byte offset 0
; followed by size_t len at byte offset 256; BIGNUM_CAPACITY is therefore 32.
; The entry point is a leaf function: it makes no calls, allocates no stack
; frame, uses no global state, and requires the caller-provided ABI stack
; alignment to remain intact. It uses only caller-saved rax, rcx and r8 after
; reading arguments, so no callee-saved register is clobbered. The output is an
; int store of 0 or 1 only on success; every error path returns before the store.
; Logical-zero words above normalized len are not inspected. The C enum values
; are returned as signed two's-complement int in eax/rax: 0 success, -1 NULL,
; -2 out-of-range bit index, -3 invalid len. No condition flags are part of the
; public contract and are clobbered normally.
; @version 1.0.0
; @pre rdi and rdx point to readable/writable live objects when non-NULL, and rsi is an arbitrary size_t accepted for explicit bounds validation.
; @post On success [rdx] is 0 or 1 and the input record is unchanged, while every error path leaves [rdx] and the input record unchanged.
; @thread_safety Safe for independent read-only objects, while concurrent writers require external synchronization.
; @complexity O(1) time and O(1) auxiliary space.
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
