#ifndef SLICKS_AMIGA_KEY_SCAN_H
#define SLICKS_AMIGA_KEY_SCAN_H

/* Physical positions, not character entry: preserve the DOS set-1 identities
 * used by saved bindings. Name entry separately uses keymap.library.
 * Classic raw positions: Amiga Hardware Reference Manual, Keyboard chapter;
 * https://wiki.amigaos.net/wiki/Keymap_Library (raw-key tables).
 * Cursor/keypad aliases and both Alt keys follow the DOS callback's single
 * scan-byte model. Amiga-only keys and unassigned positions remain unmapped.
 * Caller handles the release bit, as with the previous menu mapping. */
static inline unsigned short amiga_raw_to_dos_scan(unsigned short raw)
{
    static const unsigned char scan[128]={
        0x29,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0a,0x0b,0x0c,0x0d,0x2b,0,0x52,
        0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1a,0x1b,0,0x4f,0x50,0x51,
        0x1e,0x1f,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x2b,0,0x4b,0x4c,0x4d,
        0x56,0x2c,0x2d,0x2e,0x2f,0x30,0x31,0x32,0x33,0x34,0x35,0,0x53,0x47,0x48,0x49,
        0x39,0x0e,0x0f,0x1c,0x1c,0x01,0x53,0,0,0,0x4a,0,0x48,0x50,0x4d,0x4b,
        0x3b,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44,0,0,0x35,0x37,0x4e,0,
        0x2a,0x36,0x3a,0x1d,0x38,0x38,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
    };
    return scan[raw&127];
}

/* Menu-only aliases, by physical position: keypad 9/3 and the two keys
 * immediately right of P ([/]). Keep text entry and saved driving bindings
 * on the unmodified keymap/physical scan paths above. Caller handles release. */
static inline unsigned short amiga_raw_to_menu_scan(unsigned short raw)
{
    unsigned key=raw&127;
    if(key==0x1a) return 0x49;
    if(key==0x1b) return 0x51;
    return amiga_raw_to_dos_scan(raw);
}

struct SlicksAmigaHelpKey { unsigned char ascii,scan; };
/* Page aliases take precedence over their printable keymap characters in
 * Help, which has no text entry. Other characters stay keymapped. */
static inline struct SlicksAmigaHelpKey slicks_amiga_help_key(unsigned char raw,unsigned char character)
{
    struct SlicksAmigaHelpKey key={0,0};
    if(raw&128) return key;
    if(raw==0x3f || raw==0x1f || raw==0x1a || raw==0x1b)
        key.scan=(unsigned char)amiga_raw_to_menu_scan(raw);
    else if(character) key.ascii=character;
    else if(raw<0x60) key.scan=(unsigned char)amiga_raw_to_dos_scan(raw);
    return key;
}
#endif
