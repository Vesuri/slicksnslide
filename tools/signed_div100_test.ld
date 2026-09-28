SECTIONS
{
    . = 0;
    .reference : { *(.text.reference_div100) }
    ASSERT(SIZEOF(.reference)<256,"reference overlaps native entry")
    . = 256;
    .native : { *(.text.native_div100) }
    /DISCARD/ : { *(*) }
}
