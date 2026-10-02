// interface_loading_screen_set_text  (Ghidra: FUN_004978a0; named by types/interface.h's own
// progress screen note)
// address 0x4978a0, size 34 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: types/interface.h documents this address as interface_loading_screen_set_text in
// its progress-screen section; clears progress_screen_text (0x006b2f28, named by the same
// header) when given a null string, otherwise forwards to an unnamed formatter/copier.
// register convention: text pointer in EAX (in_EAX, unresolved register read).
// blam-cc: EAX -> text
// UNSURE: string_convert_ascii_to_unicode (0x557990) is well outside this module and not otherwise named; its
// signature (presumably taking the same EAX text pointer plus progress_screen_text as an
// implicit destination) is not recovered here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint16_t progress_screen_text[0x20]; // 0x006b2f28

extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dest, int32_t dest_bytes, const char *source); // 0x557990, 8-bit to wide copy
    // blam-cc: EAX -> dest, EDI -> dest_bytes, EBX -> source (objdump 0x4978af: dest 0x006b2f28, 0x40 bytes)

// blam-cc: EAX -> text
// Sets the progress screen's message text: clears it when text is null, otherwise forwards to
// string_convert_ascii_to_unicode (presumably a formatted copy into progress_screen_text).
void interface_loading_screen_set_text(const char *text)
{
    if (text == (const char *)0) {
        progress_screen_text[0] = 0;
    } else {
        string_convert_ascii_to_unicode(progress_screen_text, 0x40, text);
    }
}

#if 0
Original Ghidra decompilation (0x4978a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_004978a0(void)

{
  int in_EAX;

  if (in_EAX == 0) {
    _DAT_006b2f28 = 0;
    return;
  }
  FUN_00557990();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
