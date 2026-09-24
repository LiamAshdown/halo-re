// font_glyph_cache_clear_all  (Ghidra: font_glyph_cache_clear_all, already named)
// address 0x514cb0, size 44 bytes
// name confidence: 0.55  rewrite confidence: 0.75
// evidence: walks g_font_glyph_cache.entries[0..0x200) (types/rasterizer.h); for every occupied
//   slot, clears the owning FontCharacter's hardware_character_index (+0xc, per the header's
//   FontCharacter field notes) back to "not cached" before clearing the slot itself.
// register convention: none -- __cdecl, no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

extern font_glyph_cache g_font_glyph_cache; // 0x006d8828

// Invalidates every occupied font glyph cache slot (only while the cache is initialized).
void __cdecl font_glyph_cache_clear_all(void)
{
    int i;

    if (g_font_glyph_cache.initialized == 0) {
        return;
    }
    for (i = 0; i < 0x200; i++) {
        if (g_font_glyph_cache.entries[i].character != 0) {
            *(int16_t *)(g_font_glyph_cache.entries[i].character + 0xc) = -1;
        }
        g_font_glyph_cache.entries[i].character = 0;
    }
}

#if 0
Original Ghidra decompilation (0x514cb0):

void __cdecl font_glyph_cache_clear_all(void)

{
  int *piVar1;
  int iVar2;

  if (DAT_006d8828 != '\0') {
    piVar1 = &DAT_006d8838;
    iVar2 = 0x200;
    do {
      if (*piVar1 != 0) {
        *(undefined2 *)(*piVar1 + 0xc) = 0xffff;
      }
      *piVar1 = 0;
      piVar1 = piVar1 + 2;
      iVar2 = iVar2 + -1;
    } while (iVar2 != 0);
  }
  return;
}
#endif
