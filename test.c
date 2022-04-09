/* Some tests for the ARM instruction decoder (disassembler)
 *
 * Copyright 2022, CompuPhase
 */
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "armdisasm.h"

static void printthumb(ARMSTATE *state, uint16_t w, uint16_t w2)
{
  disasm_thumb(state, w, w2);
  if (state->size == 4)
    printf("%04x %04x   ", w, w2);
  else
    printf("%04x        ", w);
  printf("%s\n", state->text);
}

static void testthumb(ARMSTATE *state, uint16_t w, uint16_t w2, int length, const char *match)
{
  disasm_clear_codepool(state);
  printthumb(state, w, w2);
  assert(state->size == length && strcmp(state->text, match) == 0);
}

static void printarm(ARMSTATE *state, uint32_t w)
{
  disasm_arm(state, w);
  printf("%08x    ", w);
  printf("%s\n", state->text);
}

static void testarm(ARMSTATE *state, uint32_t w, const char *match)
{
  disasm_clear_codepool(state);
  printarm(state, w);
  assert(state->size == 4 && strcmp(state->text, match) == 0);
}

void disasm_callback(const char *text)
{
  printf("%s\n", text);
}

int main(void)
{
  ARMSTATE arm;
  disasm_init(&arm, 0);
  testthumb(&arm, 0xe001, 0,      2, "b       0000006");
  testthumb(&arm, 0xd33a, 0,      2, "bcc     000007a");
  testthumb(&arm, 0xd048, 0,      2, "beq     0000098");
  disasm_address(&arm, 0x0800049c);
  testthumb(&arm, 0xe7ea, 0,      2, "b       8000474");
  disasm_address(&arm, 0x0800052e);
  testthumb(&arm, 0xf7ff, 0xfed1, 4, "bl      80002d4");
  disasm_address(&arm, 0x08000424);
  testthumb(&arm, 0xdbfa, 0,      2, "blt     800041c");
  testthumb(&arm, 0x4770, 0,      2, "bx      lr");
  testthumb(&arm, 0xaf00, 0,      2, "add     r7, sp, #0");
  testthumb(&arm, 0x3304, 0,      2, "adds    r3, #4");
  testthumb(&arm, 0xf107, 0x0308, 4, "add     r3, r7, #8");
  testthumb(&arm, 0x4013, 0,      2, "ands    r3, r2");
  disasm_address(&arm, 0x0800158a);
  testthumb(&arm, 0xb12c, 0,      2, "cbz     r4, 8001598");
  testthumb(&arm, 0xfab2, 0xf282, 4, "clz     r2, r2");
  testthumb(&arm, 0x2a00, 0,      2, "cmp     r2, #0");
  testthumb(&arm, 0xf5b4, 0x6faf, 4, "cmp     r4, #1400");
  testthumb(&arm, 0x407c, 0,      2, "eors    r4, r7");
  testthumb(&arm, 0x4b09, 0,      2, "ldr     r3, [pc, #36]");
  testthumb(&arm, 0x687a, 0,      2, "ldr     r2, [r7, #4]");
  testthumb(&arm, 0xf852, 0x3023, 4, "ldr     r3, [r2, r3, lsl #2]");
  testthumb(&arm, 0xf85f, 0x1ef0, 4, "ldr     r1, [pc, #-3824]");
  testthumb(&arm, 0x5cd1, 0,      2, "ldrb    r1, [r2, r3]");
  testthumb(&arm, 0x880b, 0,      2, "ldrh    r3, [r1, #0]");
  testthumb(&arm, 0xf852, 0x1eff, 4, "ldrt    r1, [r2, #255]");
  testthumb(&arm, 0x0783, 0,      2, "lsls    r3, r0, #30");
  testthumb(&arm, 0x079d, 0,      2, "lsls    r5, r3, #30");
  testthumb(&arm, 0xfa01, 0xf202, 4, "lsl     r2, r1, r2");
  testthumb(&arm, 0x2208, 0,      2, "movs    r2, #8");
  testthumb(&arm, 0x46bd, 0,      2, "mov     sp, r7");
  testthumb(&arm, 0xf44f, 0x5200, 4, "mov     r2, #8192");
  testthumb(&arm, 0xf644, 0x631f, 4, "movw    r3, #19999");
  testthumb(&arm, 0xf3ef, 0x8311, 4, "mrs     r3, BASEPRI");
  testthumb(&arm, 0xf381, 0x8811, 4, "msr     BASEPRI, r1");
  testthumb(&arm, 0x4353, 0,      2, "muls    r3, r2");
  testthumb(&arm, 0xbf00, 0,      2, "nop");
  testthumb(&arm, 0xea41, 0x0300, 4, "orr     r3, r1, r0");
  testthumb(&arm, 0xf892, 0x1fab, 4, "ldrb    r1, [r2, #4011]");
  testthumb(&arm, 0xf892, 0xffab, 4, "pld     [r2, #4011]");
  testthumb(&arm, 0xbd30, 0,      2, "pop     {r4, r5, pc}");
  testthumb(&arm, 0xbd80, 0,      2, "pop     {r7, pc}");
  testthumb(&arm, 0xb530, 0,      2, "push    {r4, r5, lr}");
  testthumb(&arm, 0xb580, 0,      2, "push    {r7, lr}");
  testthumb(&arm, 0xb4ff, 0,      2, "push    {r0-r7}");
  testthumb(&arm, 0xb5ff, 0,      2, "push    {r0-r7, lr}");
  testthumb(&arm, 0x6078, 0,      2, "str     r0, [r7, #4]");
  testthumb(&arm, 0x60bb, 0,      2, "str     r3, [r7, #8]");
  testthumb(&arm, 0xf8c3, 0x20f0, 4, "str     r2, [r3, #240]");
  testthumb(&arm, 0x9300, 0,      2, "str     r3, [sp, #0]");
  testthumb(&arm, 0x70fb, 0,      2, "strb    r3, [r7, #3]");
  testthumb(&arm, 0x8003, 0,      2, "strh    r3, [r0, #0]");
  testthumb(&arm, 0xe942, 0x5504, 4, "strd    r5, r5, [r2, #-16]");
  testthumb(&arm, 0xe942, 0x5502, 4, "strd    r5, r5, [r2, #-8]");
  testthumb(&arm, 0xb084, 0,      2, "sub     sp, #16");
  testthumb(&arm, 0x1e54, 0,      2, "subs    r4, r2, #1");
  testthumb(&arm, 0x3c01, 0,      2, "subs    r4, #1");
  testthumb(&arm, 0x1ad3, 0,      2, "subs    r3, r2, r3");
  testthumb(&arm, 0xdf01, 0,      2, "svc     #1");
  testthumb(&arm, 0xb2ca, 0,      2, "uxtb    r2, r1");
  testthumb(&arm, 0xb299, 0,      2, "uxth    r1, r3");

  testthumb(&arm, 0xbf1c, 0     , 2, "itt     ne");
  testthumb(&arm, 0xfa22, 0xf20c, 4, "lsrne   r2, r2, ip");
  testthumb(&arm, 0x4313, 0     , 2, "orrne   r3, r2");
  testthumb(&arm, 0xbf04, 0     , 2, "itt     eq");
  testthumb(&arm, 0xf851, 0x3b04, 4, "ldreq   r3, [r1], #4");
  testthumb(&arm, 0x3004, 0     , 2, "addeq   r0, #4");

  testarm(&arm, 0xe0a13082, "adc     r3, r1, r2, lsl #1");
  testarm(&arm, 0xe59f00f0, "ldr     r0, [pc, #240]");
  testarm(&arm, 0xe2400024, "sub     r0, r0, #36");
  testarm(&arm, 0xe321f0db, "msr     CPSR_c, #219");
  testarm(&arm, 0xe1a0d000, "mov     sp, r0");
  testarm(&arm, 0xe2400004, "sub     r0, r0, #4");
  testarm(&arm, 0xe1a0b001, "mov     fp, r1");
  testarm(&arm, 0xe59f108c, "ldr     r1, [pc, #140]");
  testarm(&arm, 0xe1510003, "cmp     r1, r3");
  testarm(&arm, 0x34910004, "ldrcc   r0, [r1], #4");
  testarm(&arm, 0x30244000, "eorcc   r4, r4, r0");
  disasm_address(&arm, 0x00a8);
  testarm(&arm, 0x3afffffb, "bcc     000009c");
  testarm(&arm, 0xe12fff12, "bx      r2");
  testarm(&arm, 0x0000049c, "muleq   r0, ip, r4");
  testarm(&arm, 0x3fffcfff, "svccc   0x00ffcfff");
  testarm(&arm, 0xe92d0030, "push    {r4, r5}");
  testarm(&arm, 0xe8bd0030, "pop     {r4, r5}");
  testarm(&arm, 0xe1a03083, "lsl     r3, r3, #1");
  testarm(&arm, 0xe7d01003, "ldrb    r1, [r0, r3]");
  testarm(&arm, 0xe1d210b8, "ldrh    r1, [r2, #8]");
  testarm(&arm, 0xe0c20293, "smull   r0, r2, r3, r2");
  testarm(&arm, 0xe10f0000, "mrs     r0, CPSR");
  testarm(&arm, 0x9e6495a3, "cdpls   5, 6, cr9, cr4, cr3, {5}");
  testarm(&arm, 0x0edb8832, "mrceq   8, 6, r8, cr11, cr2, {1}");
  testarm(&arm, 0xbe0b1010, "mcrlt   0, 0, r1, cr11, cr0, {0}");
  testarm(&arm, 0x1db71064, "ldcne   0, cr1, [r7, #400]!");
  testarm(&arm, 0xf5d3f000, "pld     [r3, #0]");
  testarm(&arm, 0xe1413094, "swpb    r3, r4, [r1]");
  testarm(&arm, 0xe6842351, "pkhtb   r2, r4, r1, asr #6");
  testarm(&arm, 0xe6e141d2, "usat    r4, #1, r2, asr #3");

  disasm_cleanup(&arm);
  return 0;
}

