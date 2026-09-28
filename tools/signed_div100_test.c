#include "../src/game/signed_division.h"
_Static_assert(sizeof(long)==4,"target arithmetic probe");
long reference_div100(long value) { return value/100L; }
long native_div100(long value) { return slicks_div100(value); }
