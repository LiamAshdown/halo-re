// interface_update_for_resolution_change  (Ghidra: interface_update_for_resolution_change,
// already named)
// address 0x497250, size 105 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: the two fields it stores into (0x00718f84/88) are used as an (x, y) point by
// widget_instance_find_at_point's caller in interface_tick @0x497e80
// (`widget_instance_find_at_point(widget, DAT_00718f84, DAT_00718f88)`), which is only
// consistent with these being the cursor position, matching types/interface.h's own
// ui_cursor_x/ui_cursor_y naming -- despite the name assigned to this function, it clamps a
// point into the 640x480 UI coordinate space, not a screen resolution.
// register convention: new cursor x in EAX, new cursor y in ECX (both unresolved register
// reads in Ghidra's decompile; sole caller ui_cursor_update @0x4972c0 confirms the slots by
// disassembly).
// blam-cc: EAX -> new_cursor_x, ECX -> new_cursor_y

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t ui_cursor_x;      // 0x00718f84, clamped into 0 .. 0x280
extern int32_t ui_cursor_y;      // 0x00718f88, clamped into 0 .. 0x1e0
extern uint8_t ui_cursor_changed; // 0x00718f82

// blam-cc: EAX -> new_cursor_x, ECX -> new_cursor_y
// Stores a new cursor position into ui_cursor_x/ui_cursor_y, clamping each axis into the
// engine's 640x480 UI coordinate space (or to 0 if negative), and raises ui_cursor_changed
// whenever either axis actually moved.
void interface_update_for_resolution_change(int32_t new_cursor_x, int32_t new_cursor_y)
{
    if (ui_cursor_x != new_cursor_x) {
        ui_cursor_changed = 1;
    } else {
        ui_cursor_changed = 0;
        if (ui_cursor_y != new_cursor_y) {
            ui_cursor_changed = 1;
        }
    }

    if (new_cursor_x < 0) {
        ui_cursor_x = 0;
    } else {
        ui_cursor_x = 0x280;
        if (new_cursor_x < 0x281) {
            ui_cursor_x = new_cursor_x;
        }
    }

    if (new_cursor_y < 0) {
        ui_cursor_y = 0;
        return;
    }
    ui_cursor_y = 0x1e0;
    if (new_cursor_y < 0x1e1) {
        ui_cursor_y = new_cursor_y;
    }
}

#if 0
Original Ghidra decompilation (0x497250):

void interface_update_for_resolution_change(void)

{
  int in_EAX;
  int in_ECX;

  if ((DAT_00718f84 != in_EAX) || (DAT_00718f82 = 0, DAT_00718f88 != in_ECX)) {
    DAT_00718f82 = 1;
  }
  if (in_EAX < 0) {
    DAT_00718f84 = 0;
  }
  else {
    DAT_00718f84 = 0x280;
    if (in_EAX < 0x281) {
      DAT_00718f84 = in_EAX;
    }
  }
  if (in_ECX < 0) {
    DAT_00718f88 = 0;
    return;
  }
  DAT_00718f88 = 0x1e0;
  if (in_ECX < 0x1e1) {
    DAT_00718f88 = in_ECX;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
