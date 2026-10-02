// virtual_keyboard_initialize  (Ghidra: virtual_keyboard_initialize, already named)
// address 0x4a88f0, size 175 bytes
// name confidence: 0.9 (existing Ghidra name, matches types/interface.h)   rewrite confidence: 0.85
// evidence: types/interface.h virtual_keyboard_globals (0x007193a8) -- every DAT_ offset here
// lines up exactly with a named field (active, strings_tag_data, caret, unknown_0a,
// maximum_length, selection_start, selection_end, unknown_12, destination, destination_end,
// open_time, white_bitmap); cache.h tag_instance for the tag_lookup idiom.
// register convention: __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern virtual_keyboard_globals virtual_keyboard; // 0x007193a8
extern tag_instance *tag_instances;               // 0x0087bc14
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group

// Resets the virtual-keyboard globals to inactive and looks up the ui\english strings tag and
// the shared white bitmap tag; returns whether the strings tag was found.
int32_t virtual_keyboard_initialize(void)
{
    datum_index strings_tag;

    virtual_keyboard.active = 0;
    virtual_keyboard.unknown_01 = 0;
    virtual_keyboard.unknown_02 = 0;
    virtual_keyboard.unknown_03 = 0;

    strings_tag = tag_lookup(0x76636b79 /* vcky */, (char *)"ui\\english");
    if (strings_tag != 0xffffffff) {
        virtual_keyboard.strings_tag_data = tag_instances[strings_tag & 0xffff].data;
        virtual_keyboard.caret = 0;
        virtual_keyboard.unknown_0a = 0;
        virtual_keyboard.maximum_length = 0;
        virtual_keyboard.selection_start = -1;
        virtual_keyboard.selection_end = -1;
        virtual_keyboard.unknown_12 = 0;
        virtual_keyboard.destination = 0;
        virtual_keyboard.destination_end = 0;
        virtual_keyboard.open_time = 0;
    }

    virtual_keyboard.white_bitmap = tag_lookup(0x6269746d /* bitm */, (char *)"ui\\shell\\bitmaps\\white");
    return virtual_keyboard.strings_tag_data != 0;
}

#if 0
Original Ghidra decompilation (0x4a88f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

int __cdecl virtual_keyboard_initialize(void)

{
  uint uVar1;

  DAT_007193a8 = 0;
  DAT_007193a9 = 0;
  DAT_007193aa = 0;
  DAT_007193ab = 0;
  uVar1 = tag_lookup("ui\\english");
  if (uVar1 != 0xffffffff) {
    DAT_007193ac = *(int *)((uVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    _DAT_007193b0 = 0;
    _DAT_007193b2 = 0;
    DAT_007193b4 = 0;
    _DAT_007193b6 = 0xffff;
    _DAT_007193b8 = 0xffff;
    _DAT_007193ba = 0;
    DAT_007193c0 = 0;
    DAT_007193c4 = 0;
    _DAT_007193c8 = 0;
  }
  DAT_007193cc = tag_lookup("ui\\shell\\bitmaps\\white");
  return (uint)(DAT_007193ac != 0);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
