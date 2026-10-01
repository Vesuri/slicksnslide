#ifndef SLICKS_SIGNED_DIVISION_H
#define SLICKS_SIGNED_DIVISION_H

/* Exact signed truncation, including negative coordinates and LONG_MIN.
 * The 68020 instruction avoids the compiler's full-width reciprocal multiply
 * and sign-correction sequence. No cached coordinates or narrowed inputs. */
static inline long slicks_div100(long value)
{
#if defined(__m68k__)
    _Static_assert(sizeof(long)==4,"68020 signed division operand");
    __asm__("divs.l #100,%0" : "+d"(value) : : "cc");
    return value;
#else
    return value/100L;
#endif
}
#endif
