#ifndef SLICKS_SIGNED_DIVISION_H
#define SLICKS_SIGNED_DIVISION_H

#if defined(__m68k__) && defined(SLICKS_DIV100_CHECK)
extern volatile unsigned long slicks_div100_checks,slicks_div100_mismatches;
extern volatile long slicks_div100_first_input,slicks_div100_first_actual,
                     slicks_div100_first_expected;
#endif

/* Exact signed truncation, including negative coordinates and LONG_MIN.
 * The 68020 instruction avoids the compiler's full-width reciprocal multiply
 * and sign-correction sequence. No cached coordinates or narrowed inputs. */
static inline long slicks_div100(long value)
{
#if defined(__m68k__)
    _Static_assert(sizeof(long)==4,"68020 signed division operand");
#if defined(SLICKS_DIV100_CHECK)
    long original=value;
#endif
    __asm__("divs.l #100,%0" : "+d"(value) : : "cc");
#if defined(SLICKS_DIV100_CHECK)
    long expected=original/100L;
    ++slicks_div100_checks;
    if(value!=expected && !slicks_div100_mismatches++) {
        slicks_div100_first_input=original;
        slicks_div100_first_actual=value;
        slicks_div100_first_expected=expected;
    }
#endif
    return value;
#else
    return value/100L;
#endif
}
#endif
