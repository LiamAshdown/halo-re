// ui_controls_sensitivity_row_refresh  (Ghidra: FUN_004a2270, renamed)
// renamed from FUN_004a2270 in the naming pass
// address 0x4a2270, size 99 bytes, callers=0 in this build
// name confidence: 0.3   rewrite confidence: 0.4
// evidence: functions.md: "List-widget build/refresh callback that populates values then
// refreshes the widget." Trivial wrapper around ui_controls_populate_sensitivity_row (this session), forwarding its own
// EAX/ESI unchanged (Ghidra shows zero visible arguments at the call site).
// register convention: same as ui_controls_populate_sensitivity_row: EAX -> widget, ESI -> profile_record.
// blam-cc: EAX -> widget, ESI -> profile_record

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "fn_interface.h"


extern void widget_play_sound_effect(int16_t effect_id); // 0x498e90

uint32_t ui_controls_sensitivity_row_refresh(widget_instance *widget, const uint8_t *profile_record)
{
    ui_controls_populate_sensitivity_row(widget, profile_record);
    widget_play_sound_effect(0); // UNSURE: effect id read from an unresolved register here
    return 1;
}

#if 0
Original Ghidra decompilation (0x4a2270):

/* WARNING: Function: __chkstk replaced with injection: alloca_probe */

undefined4 FUN_004a2270(void)

{
  FUN_004a20f0();
  widget_play_sound_effect();
  return 1;
}
#endif
