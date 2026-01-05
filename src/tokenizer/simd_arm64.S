#if defined(__aarch64__) || defined(_M_ARM64)

#ifdef __APPLE__
#define SYMBOL(name) _##name
#else
#define SYMBOL(name) name
.arch_extension crc
#endif

.text
.align 4

// =============================================================================
// uint64_t simd_hash_bytes_arm64(const uint8_t *bytes, size_t len)
// 
// Fast hash using ARM64 CRC32 instructions
// x0 = bytes pointer
// x1 = length
// Returns: 64-bit hash in x0
// =============================================================================
.globl SYMBOL(simd_hash_bytes_arm64)
SYMBOL(simd_hash_bytes_arm64):
    mov     w2, #0xFFFFFFFF         // Initial CRC value
    cbz     x1, .Lhash_done         // If len == 0, return initial value
    
    // Process 8 bytes at a time
.Lhash_loop8:
    cmp     x1, #8
    b.lt    .Lhash_loop1
    ldr     x3, [x0], #8
    crc32x  w2, w2, x3
    sub     x1, x1, #8
    b       .Lhash_loop8

    // Process remaining bytes one at a time
.Lhash_loop1:
    cbz     x1, .Lhash_done
    ldrb    w3, [x0], #1
    crc32b  w2, w2, w3
    sub     x1, x1, #1
    b       .Lhash_loop1

.Lhash_done:
    // Mix the result for better distribution
    eor     x0, x2, x2, lsr #16
    movz    x3, #0xca6b
    movk    x3, #0x85eb, lsl #16
    mul     x0, x0, x3
    eor     x0, x0, x0, lsr #13
    movz    x3, #0xae35
    movk    x3, #0xc2b2, lsl #16
    mul     x0, x0, x3
    eor     x0, x0, x0, lsr #16
    ret

// =============================================================================
// size_t simd_find_non_ascii_arm64(const uint8_t *data, size_t len)
//
// Find first non-ASCII byte (>= 0x80) using NEON
// x0 = data pointer
// x1 = length
// Returns: index of first non-ASCII byte, or len if all ASCII
// =============================================================================
.globl SYMBOL(simd_find_non_ascii_arm64)
SYMBOL(simd_find_non_ascii_arm64):
    mov     x2, x0                  // Save original pointer
    mov     x3, x1                  // Save original length
    
    // Process 16 bytes at a time using NEON
.Lascii_loop16:
    cmp     x1, #16
    b.lt    .Lascii_loop1
    
    ld1     {v0.16b}, [x0], #16
    // Check if any byte has high bit set (>= 0x80)
    cmlt    v1.16b, v0.16b, #0      // v1[i] = 0xFF if v0[i] < 0 (signed, i.e., >= 0x80)
    umaxv   b2, v1.16b              // Get max across all lanes
    umov    w4, v2.b[0]
    cbnz    w4, .Lascii_found16
    
    sub     x1, x1, #16
    b       .Lascii_loop16

.Lascii_found16:
    // Found non-ASCII in this block, scan byte by byte
    sub     x0, x0, #16             // Back up to start of block
    mov     x1, #16
    
.Lascii_loop1:
    cbz     x1, .Lascii_done
    ldrb    w4, [x0]
    tst     w4, #0x80
    b.ne    .Lascii_done
    add     x0, x0, #1
    sub     x1, x1, #1
    b       .Lascii_loop1

.Lascii_done:
    sub     x0, x0, x2              // Calculate offset from start
    ret

// =============================================================================
// bool simd_is_all_ascii_arm64(const uint8_t *data, size_t len)
//
// Check if all bytes are ASCII (< 0x80) using NEON
// x0 = data pointer
// x1 = length
// Returns: 1 if all ASCII, 0 otherwise
// =============================================================================
.globl SYMBOL(simd_is_all_ascii_arm64)
SYMBOL(simd_is_all_ascii_arm64):
    movi    v2.16b, #0              // Accumulator for high bits
    
.Lall_ascii_loop16:
    cmp     x1, #16
    b.lt    .Lall_ascii_tail
    
    ld1     {v0.16b}, [x0], #16
    orr     v2.16b, v2.16b, v0.16b  // OR all bytes into accumulator
    sub     x1, x1, #16
    b       .Lall_ascii_loop16

.Lall_ascii_tail:
    cbz     x1, .Lall_ascii_check
    mov     w4, #0
    
.Lall_ascii_tail_loop:
    ldrb    w2, [x0], #1
    orr     w4, w4, w2
    sub     x1, x1, #1
    cbnz    x1, .Lall_ascii_tail_loop
    
    // Merge tail result into vector
    dup     v3.16b, w4
    orr     v2.16b, v2.16b, v3.16b

.Lall_ascii_check:
    // Check if any byte in accumulator has high bit set
    cmlt    v1.16b, v2.16b, #0
    umaxv   b0, v1.16b
    umov    w0, v0.b[0]
    // Invert: return 1 if no high bits, 0 if any high bit
    eor     w0, w0, #0xFF
    and     w0, w0, #1
    ret

// =============================================================================
// size_t simd_count_utf8_chars_arm64(const uint8_t *data, size_t len)
//
// Count UTF-8 characters (count bytes that are NOT continuation bytes 10xxxxxx)
// x0 = data pointer
// x1 = length
// Returns: number of UTF-8 characters
// =============================================================================
.globl SYMBOL(simd_count_utf8_chars_arm64)
SYMBOL(simd_count_utf8_chars_arm64):
    mov     x2, #0                  // Character count
    
.Lcount_loop16:
    cmp     x1, #16
    b.lt    .Lcount_loop1
    
    ld1     {v0.16b}, [x0], #16
    // Continuation bytes are 10xxxxxx (0x80-0xBF)
    // NOT continuation = either < 0x80 OR >= 0xC0
    // Count bytes where (byte & 0xC0) != 0x80
    movi    v1.16b, #0xC0
    and     v2.16b, v0.16b, v1.16b
    movi    v3.16b, #0x80
    cmeq    v4.16b, v2.16b, v3.16b  // v4[i] = 0xFF if continuation
    not     v4.16b, v4.16b          // v4[i] = 0xFF if NOT continuation
    // Count set bytes (each 0xFF = -1, so negate and sum)
    cnt     v4.16b, v4.16b          // Count bits in each byte
    uaddlv  h5, v4.16b              // Sum all counts
    umov    w3, v5.h[0]
    lsr     w3, w3, #3              // Divide by 8 (we counted bits, want bytes)
    add     x2, x2, x3
    
    sub     x1, x1, #16
    b       .Lcount_loop16

.Lcount_loop1:
    cbz     x1, .Lcount_done
    ldrb    w3, [x0], #1
    and     w4, w3, #0xC0
    cmp     w4, #0x80
    b.eq    .Lcount_skip            // Skip continuation bytes
    add     x2, x2, #1
.Lcount_skip:
    sub     x1, x1, #1
    b       .Lcount_loop1

.Lcount_done:
    mov     x0, x2
    ret

// =============================================================================
// size_t simd_argmin_u32_arm64(const uint32_t *values, size_t count, uint32_t *out_min)
//
// Find index of minimum value in array of uint32_t
// Uses NEON for parallel comparison, scalar for index tracking
// x0 = values pointer
// x1 = count
// x2 = out_min pointer (to store the minimum value)
// Returns: index of minimum value
// =============================================================================
.globl SYMBOL(simd_argmin_u32_arm64)
SYMBOL(simd_argmin_u32_arm64):
    cbz     x1, .Largmin_empty
    
    mov     x3, #0                  // min_idx
    ldr     w4, [x0]                // min_val
    mov     x5, #1                  // i = 1
    add     x6, x0, #4              // ptr = values + 1
    
.Largmin_loop:
    cmp     x5, x1
    b.ge    .Largmin_done
    
    // Check if we have 4+ elements remaining
    sub     x7, x1, x5
    cmp     x7, #4
    b.lt    .Largmin_scalar
    
    // Load 4 values
    ld1     {v0.4s}, [x6]
    
    // Find minimum among 4 using uminv
    uminv   s1, v0.4s
    umov    w7, v1.s[0]
    
    // If chunk min >= current min, skip chunk
    cmp     w7, w4
    b.hs    .Largmin_skip4
    
    // Chunk has smaller value - scan it
    ldr     w7, [x6]
    cmp     w7, w4
    b.hs    1f
    mov     w4, w7
    mov     x3, x5
1:  ldr     w7, [x6, #4]
    add     x8, x5, #1
    cmp     w7, w4
    b.hs    2f
    mov     w4, w7
    mov     x3, x8
2:  ldr     w7, [x6, #8]
    add     x8, x5, #2
    cmp     w7, w4
    b.hs    3f
    mov     w4, w7
    mov     x3, x8
3:  ldr     w7, [x6, #12]
    add     x8, x5, #3
    cmp     w7, w4
    b.hs    .Largmin_skip4
    mov     w4, w7
    mov     x3, x8
    
.Largmin_skip4:
    add     x5, x5, #4
    add     x6, x6, #16
    b       .Largmin_loop

.Largmin_scalar:
    ldr     w7, [x6], #4
    cmp     w7, w4
    b.hs    .Largmin_next
    mov     w4, w7
    mov     x3, x5
.Largmin_next:
    add     x5, x5, #1
    b       .Largmin_loop

.Largmin_done:
    str     w4, [x2]
    mov     x0, x3
    ret

.Largmin_empty:
    mov     w7, #0xFFFFFFFF
    str     w7, [x2]
    mov     x0, #0
    ret

// =============================================================================
// size_t simd_match_ascii_letters_arm64(const uint8_t *data, size_t len)
//
// Find length of ASCII letter run (A-Z, a-z)
// x0 = data pointer
// x1 = length
// Returns: number of consecutive ASCII letters
// =============================================================================
.globl SYMBOL(simd_match_ascii_letters_arm64)
SYMBOL(simd_match_ascii_letters_arm64):
    cbz     x1, .Lletters_done_zero
    
    mov     x2, #0                  // matched count
    
.Lletters_loop16:
    cmp     x1, #16
    b.lt    .Lletters_scalar
    
    // Load 16 bytes
    ld1     {v0.16b}, [x0]
    
    // Check for uppercase: 'A' <= byte <= 'Z'
    movi    v1.16b, #0x40           // 'A' - 1
    movi    v2.16b, #0x5B           // 'Z' + 1
    cmhi    v3.16b, v0.16b, v1.16b  // byte > 0x40
    cmhi    v4.16b, v2.16b, v0.16b  // byte < 0x5B
    and     v3.16b, v3.16b, v4.16b  // uppercase mask
    
    // Check for lowercase: 'a' <= byte <= 'z'
    movi    v1.16b, #0x60           // 'a' - 1
    movi    v2.16b, #0x7B           // 'z' + 1
    cmhi    v4.16b, v0.16b, v1.16b  // byte > 0x60
    cmhi    v5.16b, v2.16b, v0.16b  // byte < 0x7B
    and     v4.16b, v4.16b, v5.16b  // lowercase mask
    
    // Combine: is_letter = uppercase | lowercase
    orr     v3.16b, v3.16b, v4.16b
    
    // Check if all 16 bytes are letters
    uminv   b4, v3.16b              // min across lanes
    umov    w3, v4.b[0]
    cbz     w3, .Lletters_find_end  // if min is 0, not all letters
    
    // All 16 are letters, continue
    add     x0, x0, #16
    add     x2, x2, #16
    sub     x1, x1, #16
    b       .Lletters_loop16

.Lletters_find_end:
    // Find first non-letter in v3 (0xFF = letter, 0x00 = non-letter)
    mov     w4, #0
.Lletters_scan:
    umov    w5, v3.b[0]
    cbz     w5, .Lletters_done
    ext     v3.16b, v3.16b, v3.16b, #1
    add     w4, w4, #1
    cmp     w4, #16
    b.lt    .Lletters_scan
    b       .Lletters_done

.Lletters_scalar:
    mov     w4, #0                  // extra count from scalar loop
    cbz     x1, .Lletters_done
    
.Lletters_scalar_loop:
    ldrb    w3, [x0]
    
    // Check uppercase: 'A' <= byte <= 'Z'
    sub     w5, w3, #0x41           // w5 = byte - 'A'
    cmp     w5, #26
    b.lo    .Lletters_scalar_match
    
    // Check lowercase: 'a' <= byte <= 'z'
    sub     w5, w3, #0x61           // w5 = byte - 'a'
    cmp     w5, #26
    b.hs    .Lletters_done
    
.Lletters_scalar_match:
    add     x0, x0, #1
    add     w4, w4, #1
    sub     x1, x1, #1
    cbnz    x1, .Lletters_scalar_loop

.Lletters_done:
    add     x0, x2, x4
    ret

.Lletters_done_zero:
    mov     x0, #0
    ret

#endif

