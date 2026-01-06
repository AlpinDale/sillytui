/*
 * Apple AMX instruction wrappers
 *
 * Vendored from: https://github.com/corsix/amx
 *
 * MIT License
 *
 * Copyright (c) 2022 Peter Cawley
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once
#include <stdint.h>

#define AMX_NOP_OP_IMM5(op, imm5)                                              \
  __asm("nop\nnop\nnop\n.word (0x201000 + (%0 << 5) + %1)"                     \
        :                                                                      \
        : "i"(op), "i"(imm5)                                                   \
        : "memory")

#define AMX_OP_GPR(op, gpr)                                                    \
  __asm(".word (0x201000 + (%0 << 5) + 0%1 - ((0%1 >> 4) * 6))"                \
        :                                                                      \
        : "i"(op), "r"((uint64_t)(gpr))                                        \
        : "memory")

#define AMX_LDX(gpr) AMX_OP_GPR(0, gpr)
#define AMX_LDY(gpr) AMX_OP_GPR(1, gpr)
#define AMX_STX(gpr) AMX_OP_GPR(2, gpr)
#define AMX_STY(gpr) AMX_OP_GPR(3, gpr)
#define AMX_LDZ(gpr) AMX_OP_GPR(4, gpr)
#define AMX_STZ(gpr) AMX_OP_GPR(5, gpr)
#define AMX_LDZI(gpr) AMX_OP_GPR(6, gpr)
#define AMX_STZI(gpr) AMX_OP_GPR(7, gpr)
#define AMX_EXTRX(gpr) AMX_OP_GPR(8, gpr)
#define AMX_EXTRY(gpr) AMX_OP_GPR(9, gpr)
#define AMX_FMA64(gpr) AMX_OP_GPR(10, gpr)
#define AMX_FMS64(gpr) AMX_OP_GPR(11, gpr)
#define AMX_FMA32(gpr) AMX_OP_GPR(12, gpr)
#define AMX_FMS32(gpr) AMX_OP_GPR(13, gpr)
#define AMX_MAC16(gpr) AMX_OP_GPR(14, gpr)
#define AMX_FMA16(gpr) AMX_OP_GPR(15, gpr)
#define AMX_FMS16(gpr) AMX_OP_GPR(16, gpr)
#define AMX_SET() AMX_NOP_OP_IMM5(17, 0)
#define AMX_CLR() AMX_NOP_OP_IMM5(17, 1)
#define AMX_VECINT(gpr) AMX_OP_GPR(18, gpr)
#define AMX_VECFP(gpr) AMX_OP_GPR(19, gpr)
#define AMX_MATINT(gpr) AMX_OP_GPR(20, gpr)
#define AMX_MATFP(gpr) AMX_OP_GPR(21, gpr)
#define AMX_GENLUT(gpr) AMX_OP_GPR(22, gpr)
