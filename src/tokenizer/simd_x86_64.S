#if defined(__x86_64__) || defined(_M_X64)

.section .note.GNU-stack,"",@progbits

.text
.align 16

// =============================================================================
// uint64_t simd_hash_bytes_x86_64(const uint8_t *bytes, size_t len)
//
// Fast hash using CRC32C instructions (SSE4.2)
// rdi = bytes pointer
// rsi = length
// Returns: 64-bit hash in rax
// =============================================================================
.globl simd_hash_bytes_x86_64
simd_hash_bytes_x86_64:
    mov     $0xFFFFFFFF, %eax       // Initial CRC value
    test    %rsi, %rsi
    jz      .Lhash_done

    // Process 8 bytes at a time
.Lhash_loop8:
    cmp     $8, %rsi
    jb      .Lhash_loop1
    crc32q  (%rdi), %rax
    add     $8, %rdi
    sub     $8, %rsi
    jmp     .Lhash_loop8

    // Process remaining bytes one at a time
.Lhash_loop1:
    test    %rsi, %rsi
    jz      .Lhash_mix
    movzbl  (%rdi), %ecx
    crc32b  %cl, %eax
    inc     %rdi
    dec     %rsi
    jmp     .Lhash_loop1

.Lhash_mix:
    // Mix the result for better distribution (MurmurHash3 finalizer)
    mov     %rax, %rcx
    shr     $16, %rcx
    xor     %rcx, %rax
    mov     $0x85ebca6b, %rcx
    imul    %rcx, %rax
    mov     %rax, %rcx
    shr     $13, %rcx
    xor     %rcx, %rax
    mov     $0xc2b2ae35, %rcx
    imul    %rcx, %rax
    mov     %rax, %rcx
    shr     $16, %rcx
    xor     %rcx, %rax

.Lhash_done:
    ret

// =============================================================================
// size_t simd_find_non_ascii_x86_64(const uint8_t *data, size_t len)
//
// Find first non-ASCII byte (>= 0x80) using AVX2
// rdi = data pointer
// rsi = length
// Returns: index of first non-ASCII byte, or len if all ASCII
// =============================================================================
.globl simd_find_non_ascii_x86_64
simd_find_non_ascii_x86_64:
    mov     %rdi, %r8               // Save original pointer
    xor     %rax, %rax              // offset = 0

    // Process 32 bytes at a time using AVX2
.Lfind_loop32:
    cmp     $32, %rsi
    jb      .Lfind_loop1

    vmovdqu (%rdi), %ymm0
    vpmovmskb %ymm0, %ecx           // Extract high bit of each byte
    test    %ecx, %ecx              // Any high bits set?
    jnz     .Lfind_found32

    add     $32, %rdi
    add     $32, %rax
    sub     $32, %rsi
    jmp     .Lfind_loop32

.Lfind_found32:
    // Found non-ASCII in this block, find exact position
    bsf     %ecx, %ecx              // Find first set bit
    add     %rcx, %rax
    vzeroupper
    ret

    // Process remaining bytes one at a time
.Lfind_loop1:
    test    %rsi, %rsi
    jz      .Lfind_done
    movzbl  (%rdi), %ecx
    test    $0x80, %ecx
    jnz     .Lfind_done
    inc     %rdi
    inc     %rax
    dec     %rsi
    jmp     .Lfind_loop1

.Lfind_done:
    vzeroupper
    ret

// =============================================================================
// bool simd_is_all_ascii_x86_64(const uint8_t *data, size_t len)
//
// Check if all bytes are ASCII (< 0x80) using AVX2
// rdi = data pointer
// rsi = length
// Returns: 1 if all ASCII, 0 otherwise
// =============================================================================
.globl simd_is_all_ascii_x86_64
simd_is_all_ascii_x86_64:
    vpxor   %ymm2, %ymm2, %ymm2     // Accumulator for high bits

    // Process 32 bytes at a time
.Lascii_loop32:
    cmp     $32, %rsi
    jb      .Lascii_tail

    vmovdqu (%rdi), %ymm0
    vpor    %ymm0, %ymm2, %ymm2     // OR all bytes into accumulator
    add     $32, %rdi
    sub     $32, %rsi
    jmp     .Lascii_loop32

.Lascii_tail:
    test    %rsi, %rsi
    jz      .Lascii_check
    xor     %ecx, %ecx

.Lascii_tail_loop:
    movzbl  (%rdi), %eax
    or      %eax, %ecx
    inc     %rdi
    dec     %rsi
    jnz     .Lascii_tail_loop

    // Merge tail result into vector
    vmovd   %ecx, %xmm3
    vpbroadcastb %xmm3, %ymm3
    vpor    %ymm3, %ymm2, %ymm2

.Lascii_check:
    // Check if any byte in accumulator has high bit set
    vpmovmskb %ymm2, %eax
    test    %eax, %eax
    setz    %al                     // Return 1 if all zero (ASCII), 0 otherwise
    movzbl  %al, %eax
    vzeroupper
    ret

// =============================================================================
// size_t simd_count_utf8_chars_x86_64(const uint8_t *data, size_t len)
//
// Count UTF-8 characters (count bytes that are NOT continuation bytes) using AVX2
// Continuation bytes are 10xxxxxx (0x80-0xBF)
// rdi = data pointer
// rsi = length
// Returns: number of UTF-8 characters
// =============================================================================
.globl simd_count_utf8_chars_x86_64
simd_count_utf8_chars_x86_64:
    xor     %rax, %rax              // Character count

    // Process 32 bytes at a time
.Lcount_loop32:
    cmp     $32, %rsi
    jb      .Lcount_loop1

    vmovdqu (%rdi), %ymm0
    // Mask for 0xC0
    vmovdqa .Lmask_c0(%rip), %ymm1
    vpand   %ymm0, %ymm1, %ymm2     // v2 = data & 0xC0
    // Compare with 0x80
    vmovdqa .Lmask_80(%rip), %ymm3
    vpcmpeqb %ymm3, %ymm2, %ymm4    // v4[i] = 0xFF if continuation byte
    vpmovmskb %ymm4, %ecx
    popcnt  %ecx, %ecx              // Count continuation bytes
    add     $32, %rax
    sub     %rcx, %rax              // Subtract continuation bytes

    add     $32, %rdi
    sub     $32, %rsi
    jmp     .Lcount_loop32

    // Process remaining bytes one at a time
.Lcount_loop1:
    test    %rsi, %rsi
    jz      .Lcount_done
    movzbl  (%rdi), %ecx
    and     $0xC0, %ecx
    cmp     $0x80, %ecx
    je      .Lcount_skip
    inc     %rax

.Lcount_skip:
    inc     %rdi
    dec     %rsi
    jmp     .Lcount_loop1

.Lcount_done:
    vzeroupper
    ret

.align 32
.Lmask_c0:
    .byte 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0
    .byte 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0
    .byte 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0
    .byte 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0

.Lmask_80:
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80
    .byte 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80

// =============================================================================
// size_t simd_argmin_u32_x86_64(const uint32_t *values, size_t count, uint32_t *out_min)
//
// Find index of minimum value in array of uint32_t using AVX2
// rdi = values pointer
// rsi = count
// rdx = out_min pointer
// Returns: index of minimum value
// =============================================================================
.globl simd_argmin_u32_x86_64
simd_argmin_u32_x86_64:
    test    %rsi, %rsi
    jz      .Largmin_empty

    xor     %rax, %rax              // min_idx = 0
    mov     (%rdi), %ecx            // min_val = values[0]
    mov     $1, %r8                 // i = 1
    lea     4(%rdi), %r9            // ptr = values + 1

.Largmin_loop:
    cmp     %rsi, %r8
    jge     .Largmin_done

    // Check if we have 8+ elements remaining
    mov     %rsi, %r10
    sub     %r8, %r10
    cmp     $8, %r10
    jb      .Largmin_scalar

    // Load 8 uint32_t values
    vmovdqu (%r9), %ymm0

    // Find minimum using horizontal min
    vextracti128 $1, %ymm0, %xmm1   // Extract high 128 bits
    vpminud %xmm1, %xmm0, %xmm0     // Min of low and high halves
    vpshufd $0x0E, %xmm0, %xmm1     // Shuffle
    vpminud %xmm1, %xmm0, %xmm0
    vpshufd $0x01, %xmm0, %xmm1
    vpminud %xmm1, %xmm0, %xmm0
    vmovd   %xmm0, %r10d            // Chunk minimum

    // If chunk min >= current min, skip chunk
    cmp     %ecx, %r10d
    jae     .Largmin_skip8

    // Chunk has smaller value - scan it to find which one
    mov     $0, %r11
.Largmin_scan8:
    cmp     $8, %r11
    jge     .Largmin_skip8
    mov     (%r9, %r11, 4), %r10d
    cmp     %ecx, %r10d
    jae     .Largmin_scan8_next
    mov     %r10d, %ecx
    lea     (%r8, %r11), %rax
.Largmin_scan8_next:
    inc     %r11
    jmp     .Largmin_scan8

.Largmin_skip8:
    add     $8, %r8
    add     $32, %r9
    jmp     .Largmin_loop

.Largmin_scalar:
    mov     (%r9), %r10d
    cmp     %ecx, %r10d
    jae     .Largmin_next
    mov     %r10d, %ecx
    mov     %r8, %rax

.Largmin_next:
    inc     %r8
    add     $4, %r9
    jmp     .Largmin_loop

.Largmin_done:
    mov     %ecx, (%rdx)
    vzeroupper
    ret

.Largmin_empty:
    movl    $0xFFFFFFFF, (%rdx)
    xor     %rax, %rax
    ret

// =============================================================================
// size_t simd_match_ascii_letters_x86_64(const uint8_t *data, size_t len)
//
// Find length of ASCII letter run (A-Z, a-z) using AVX2
// rdi = data pointer
// rsi = length
// Returns: number of consecutive ASCII letters
// =============================================================================
.globl simd_match_ascii_letters_x86_64
simd_match_ascii_letters_x86_64:
    test    %rsi, %rsi
    jz      .Lletters_zero

    xor     %rax, %rax              // matched count

    // Process 32 bytes at a time
.Lletters_loop32:
    cmp     $32, %rsi
    jb      .Lletters_scalar

    // Load 32 bytes
    vmovdqu (%rdi), %ymm0

    // Check for uppercase: 'A' <= byte <= 'Z'
    vmovdqa .Lmask_40(%rip), %ymm1  // 0x40 ('A' - 1)
    vmovdqa .Lmask_5b(%rip), %ymm2  // 0x5B ('Z' + 1)
    vpcmpgtb %ymm1, %ymm0, %ymm3    // byte > 0x40
    vpcmpgtb %ymm0, %ymm2, %ymm4    // byte < 0x5B
    vpand   %ymm3, %ymm4, %ymm3     // uppercase mask

    // Check for lowercase: 'a' <= byte <= 'z'
    vmovdqa .Lmask_60(%rip), %ymm1  // 0x60 ('a' - 1)
    vmovdqa .Lmask_7b(%rip), %ymm2  // 0x7B ('z' + 1)
    vpcmpgtb %ymm1, %ymm0, %ymm4    // byte > 0x60
    vpcmpgtb %ymm0, %ymm2, %ymm5    // byte < 0x7B
    vpand   %ymm4, %ymm5, %ymm4     // lowercase mask

    // Combine: is_letter = uppercase | lowercase
    vpor    %ymm3, %ymm4, %ymm3

    // Check if all 32 bytes are letters
    vpcmpeqb .Lmask_ff(%rip), %ymm3, %ymm4
    vpmovmskb %ymm4, %ecx
    cmp     $0xFFFFFFFF, %ecx
    jne     .Lletters_find_end

    // All 32 are letters, continue
    add     $32, %rdi
    add     $32, %rax
    sub     $32, %rsi
    jmp     .Lletters_loop32

.Lletters_find_end:
    // Find first non-letter in ymm3 (0xFF = letter, 0x00 = non-letter)
    vpcmpeqb .Lmask_00(%rip), %ymm3, %ymm4  // 0xFF where non-letter
    vpmovmskb %ymm4, %ecx
    bsf     %ecx, %ecx              // Find first non-letter
    add     %rcx, %rax
    vzeroupper
    ret

.Lletters_scalar:
    cmp     $0, %rsi
    jz      .Lletters_done

.Lletters_scalar_loop:
    movzbl  (%rdi), %ecx

    // Check uppercase: 'A' <= byte <= 'Z'
    lea     -0x41(%rcx), %r8d
    cmp     $26, %r8d
    jb      .Lletters_scalar_match

    // Check lowercase: 'a' <= byte <= 'z'
    lea     -0x61(%rcx), %r8d
    cmp     $26, %r8d
    jae     .Lletters_done

.Lletters_scalar_match:
    inc     %rdi
    inc     %rax
    dec     %rsi
    jnz     .Lletters_scalar_loop

.Lletters_done:
    vzeroupper
    ret

.Lletters_zero:
    xor     %rax, %rax
    ret

.align 32
.Lmask_40:
    .byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
    .byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
    .byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40
    .byte 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40, 0x40

.Lmask_5b:
    .byte 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B
    .byte 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B
    .byte 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B
    .byte 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B, 0x5B

.Lmask_60:
    .byte 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60
    .byte 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60
    .byte 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60
    .byte 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60

.Lmask_7b:
    .byte 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B
    .byte 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B
    .byte 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B
    .byte 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B, 0x7B

.Lmask_ff:
    .byte 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    .byte 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    .byte 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    .byte 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF

.Lmask_00:
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    .byte 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00

#endif
