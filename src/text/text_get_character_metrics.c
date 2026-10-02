// text_get_character_metrics  (Ghidra: text_get_character_metrics, already named)
// address 0x557650, size 76 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: identical character_tables/characters lookup used inline by every draw and
//   measure routine in this module (text_draw_character_range_narrow/_wide,
//   text_measure_string_fit_width): character_tables.pointer[character>>8] must have
//   count == k_text_font_character_table_page_size for its page to be used, then
//   character_tables[...].pointer[character&0xff] gives the hardware/character index
//   into font->characters (stride sizeof(FontCharacter)); -1 means unmapped.
// register convention (objdump 0x557650..0x5576a0): EDX = character (movzx edx,dx at
//   entry -- only the low 16 bits matter), EDI = font (Font*, unaff_EDI). No stack args.

#include "tags.h"
#include "memory.h"
#include "text.h"

// blam-cc: EDX=character, EDI=font
// Resolves the glyph-metrics record for character within font, or (void *)0 if font has no
// glyph mapped to it (either its page is unused, or the page has no entry -- or an
// entry of -1 -- for that character's low byte).
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
FontCharacter *text_get_character_metrics(uint16_t character, Font *font)
{
    FontCharacterTables *page;

    page = (FontCharacterTables *)font->character_tables.pointer + (character >> 8);
    if (0 < (int32_t)page->character_table.count) {
        // a page whose count is not 0x100 reads through NULL, as in the binary (0x557680)
        int16_t *character_index = (page->character_table.count ==
            k_text_font_character_table_page_size)
                ? (int16_t *)page->character_table.pointer + (character & 0xff)
                : (int16_t *)0;
        int16_t hardware_index = *character_index;
        if (hardware_index != -1) {
            return (FontCharacter *)font->characters.pointer + hardware_index;
        }
    }
    return (FontCharacter *)((void *)0);
}

#if 0
Original Ghidra decompilation (0x557650):

int text_get_character_metrics(void)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  short *psVar4;
  uint in_EDX;
  int unaff_EDI;

  piVar1 = (int *)(*(int *)(unaff_EDI + 0x34) + ((in_EDX & 0xffff) >> 8) * 0xc);
  iVar2 = *piVar1;
  iVar3 = 0;
  if (0 < iVar2) {
    if (iVar2 == 0x100) {
      psVar4 = (short *)(piVar1[1] + (in_EDX & 0xff) * 2);
    }
    else {
      psVar4 = (short *)0x0;
    }
    if (*psVar4 != -1) {
      iVar3 = *(int *)(unaff_EDI + 0x80) + *psVar4 * 0x14;
    }
  }
  return iVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
