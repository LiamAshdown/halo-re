// interface_loading_screen_reset  (Ghidra: interface_loading_screen_reset, already named)
// address 0x4978d0, size 46 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// evidence: matches the given name; plain reset of every progress-screen global named in
// types/interface.h's progress-screen section.
// register convention: no register-passed arguments.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t interface_loading_screen_address_b;      // 0x0068e684
extern uint32_t interface_loading_screen_address_a;   // 0x0068e680, UNSURE: header calls this progress_screen_bitmap; see chimera__do_show_loading_screen.c
extern progress_screen_state join_ui_state;            // 0x00718f8c
extern int32_t interface_loading_screen_progress;         // 0x00718f90
extern uint16_t progress_screen_text[0x20];      // 0x006b2f28
extern uint16_t progress_screen_subtext[0x20];   // 0x006b2f68
extern datum_index interface_loading_screen_request_id;          // 0x0068e688

// Resets every progress-screen global to its inactive state.
void interface_loading_screen_reset(void)
{
    interface_loading_screen_address_a = -1;
    interface_loading_screen_address_b = -1;
    join_ui_state = (progress_screen_state)0;
    interface_loading_screen_progress = 0;
    progress_screen_text[0] = 0;
    progress_screen_subtext[0] = 0;
    interface_loading_screen_request_id = (datum_index)-1;
}

#if 0
Original Ghidra decompilation (0x4978d0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void interface_loading_screen_reset(void)

{
  DAT_0068e680 = 0xffffffff;
  DAT_0068e684 = 0xffffffff;
  DAT_00718f8c = 0;
  DAT_00718f90 = 0;
  _DAT_006b2f28 = 0;
  _DAT_006b2f68 = 0;
  DAT_0068e688 = 0xffffffff;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
