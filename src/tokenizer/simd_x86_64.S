#if defined(__x86_64__) || defined(_M_X64)

.section .note.GNU-stack,"",@progbits

.text
.align 16

// =============================================================================
// uint64_t simd_hash_bytes_x86_64(const uint8_t *bytes, size_t len)
//
// Stub implementation - returns FNV-1a hash
// rdi = bytes pointer
// rsi = length
// Returns: 64-bit hash in rax
// =============================================================================
.globl simd_hash_bytes_x86_64
simd_hash_bytes_x86_64:
    movabs  $14695981039346656037, %rax  // FNV offset basis
    test    %rsi, %rsi
    jz      .Lhash_done

.Lhash_loop:
    movzbl  (%rdi), %ecx
    xor     %rcx, %rax
    movabs  $1099511628211, %rdx          // FNV prime
    imul    %rdx, %rax
    inc     %rdi
    dec     %rsi
    jnz     .Lhash_loop

.Lhash_done:
    ret

// =============================================================================
// size_t simd_find_non_ascii_x86_64(const uint8_t *data, size_t len)
//
// Stub implementation - finds first non-ASCII byte
// rdi = data pointer
// rsi = length
// Returns: index of first non-ASCII byte, or len if all ASCII
// =============================================================================
.globl simd_find_non_ascii_x86_64
simd_find_non_ascii_x86_64:
    xor     %rax, %rax
    test    %rsi, %rsi
    jz      .Lfind_done

.Lfind_loop:
    movzbl  (%rdi, %rax), %ecx
    test    $0x80, %ecx
    jnz     .Lfind_done
    inc     %rax
    cmp     %rsi, %rax
    jb      .Lfind_loop

.Lfind_done:
    ret

// =============================================================================
// bool simd_is_all_ascii_x86_64(const uint8_t *data, size_t len)
//
// Stub implementation - checks if all bytes are ASCII
// rdi = data pointer
// rsi = length
// Returns: 1 if all ASCII, 0 otherwise
// =============================================================================
.globl simd_is_all_ascii_x86_64
simd_is_all_ascii_x86_64:
    test    %rsi, %rsi
    jz      .Lascii_true

.Lascii_loop:
    movzbl  (%rdi), %eax
    test    $0x80, %eax
    jnz     .Lascii_false
    inc     %rdi
    dec     %rsi
    jnz     .Lascii_loop

.Lascii_true:
    mov     $1, %eax
    ret

.Lascii_false:
    xor     %eax, %eax
    ret

// =============================================================================
// size_t simd_count_utf8_chars_x86_64(const uint8_t *data, size_t len)
//
// Stub implementation - counts UTF-8 characters
// rdi = data pointer
// rsi = length
// Returns: number of UTF-8 characters
// =============================================================================
.globl simd_count_utf8_chars_x86_64
simd_count_utf8_chars_x86_64:
    xor     %rax, %rax
    test    %rsi, %rsi
    jz      .Lcount_done

.Lcount_loop:
    movzbl  (%rdi), %ecx
    and     $0xC0, %ecx
    cmp     $0x80, %ecx
    je      .Lcount_skip
    inc     %rax

.Lcount_skip:
    inc     %rdi
    dec     %rsi
    jnz     .Lcount_loop

.Lcount_done:
    ret

// =============================================================================
// size_t simd_argmin_u32_x86_64(const uint32_t *values, size_t count, uint32_t *out_min)
//
// Stub implementation - finds index of minimum value
// rdi = values pointer
// rsi = count
// rdx = out_min pointer
// Returns: index of minimum value
// =============================================================================
.globl simd_argmin_u32_x86_64
simd_argmin_u32_x86_64:
    test    %rsi, %rsi
    jz      .Largmin_empty

    mov     (%rdi), %ecx            // min_val
    xor     %rax, %rax              // min_idx = 0
    mov     $1, %r8                 // i = 1

.Largmin_loop:
    cmp     %rsi, %r8
    jge     .Largmin_done

    mov     (%rdi, %r8, 4), %r9d
    cmp     %ecx, %r9d
    jae     .Largmin_next

    mov     %r9d, %ecx
    mov     %r8, %rax

.Largmin_next:
    inc     %r8
    jmp     .Largmin_loop

.Largmin_done:
    mov     %ecx, (%rdx)
    ret

.Largmin_empty:
    movl    $0xFFFFFFFF, (%rdx)
    xor     %rax, %rax
    ret

// =============================================================================
// size_t simd_match_ascii_letters_x86_64(const uint8_t *data, size_t len)
//
// Stub implementation - counts consecutive ASCII letters
// rdi = data pointer
// rsi = length
// Returns: number of consecutive ASCII letters
// =============================================================================
.globl simd_match_ascii_letters_x86_64
simd_match_ascii_letters_x86_64:
    xor     %rax, %rax
    test    %rsi, %rsi
    jz      .Lletters_done

.Lletters_loop:
    movzbl  (%rdi, %rax), %ecx

    // Check uppercase: 'A' <= byte <= 'Z'
    sub     $0x41, %ecx
    cmp     $26, %ecx
    jb      .Lletters_match

    // Restore original byte and check lowercase
    movzbl  (%rdi, %rax), %ecx
    sub     $0x61, %ecx
    cmp     $26, %ecx
    jae     .Lletters_done

.Lletters_match:
    inc     %rax
    cmp     %rsi, %rax
    jb      .Lletters_loop

.Lletters_done:
    ret

#endif
