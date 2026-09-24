// ui_variant_name_is_available  (Ghidra: FUN_004a8b50, renamed)
// address 0x4a8b50, size 37 bytes
// name confidence: 0.45 (chosen)   rewrite confidence: 0.85
// evidence: phase-4 summary "Checks whether the text currently being edited does not collide
// with an existing game-variant name." Rewritten in the phase-4 review from objdump -d
// 0x4a8b50..0x4a8b74: the wide name arrives in EDI (virtual_keyboard_process_input loads the
// keyboard destination there), 0x557950 narrows it into a 0x20 byte stack buffer (ESI out, EDI
// wide source, stack length), and game_engine_get_variant_by_name @0x4622d0 (ECX name, stack out
// NULL) reports whether a variant of that name exists; the result is the negation, in AL.
// register convention: name in EDI.
//   // blam-cc: name -> EDI

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

extern void string_convert_unicode_to_ascii(char *out_name, const uint16_t *wide_name, uint32_t max_length); // 0x557950, blam-cc: ESI out, EDI wide source
extern uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out); // 0x4622d0, blam-cc: ECX name

// blam-cc: name -> EDI
// Returns 1 when no saved game variant is called name.
uint8_t ui_variant_name_is_available(const uint16_t *name)
{
    char narrow[0x24];

    string_convert_unicode_to_ascii(narrow, name, 0x20);
    return game_engine_get_variant_by_name(narrow, (game_variant *)0) == 0;
}

#if 0
Original Ghidra decompilation (0x4a8b50):

bool FUN_004a8b50(void)

{
  char cVar1;

  FUN_00557950(0x20);
  cVar1 = game_engine_get_variant_by_name(0);
  return cVar1 == '\0';
}
#endif
