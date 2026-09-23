#include <stdio.h>
#include <stdlib.h>

/* Keep the arithmetic oracle in this translation unit so it can exercise the
 * internal Q15 helper without widening the production ABI. */
#include "../src/game/race_runtime.c"

static void expect(long actual, long expected, const char *message)
{
    if (actual == expected)
        return;
    fprintf(stderr, "drive physics: %s: got %ld, expected %ld\n",
            message, actual, expected);
    exit(1);
}

int main(void)
{
    /* Factor seven is 7ffch at zero bias. */
    expect(multiply_q15_unsigned(-860, 0x7ffc), -860,
           "negative special-state velocity");
    expect(multiply_q15_unsigned(960, 0x7ffc), 959,
           "positive special-state velocity");

    /* A -30 driver bias produces 801ah. It remains an unsigned factor, as
     * proved by the forced DOS fixture, rather than becoming -32742. */
    expect(multiply_q15_unsigned(-860, 0x801a), -861,
           "unsigned factor above 7fffh");
    expect(multiply_q15_unsigned(960, 0x801a), 960,
           "unsigned positive factor");

    /* The DOS helper keeps the low product dword before SAR 15. */
    expect(multiply_q15_unsigned(-123456, 0x7ffc), 7631,
           "wrapped signed 32-bit product");
    expect(multiply_q15_unsigned(78901, 0x7ffc), -52181,
           "wrapped positive 32-bit product");

    puts("drive physics: special-state Q15 oracle matched");
    return 0;
}
