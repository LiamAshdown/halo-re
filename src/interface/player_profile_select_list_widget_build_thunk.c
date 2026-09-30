// player_profile_select_list_widget_build_thunk  (not a Ghidra function; an incremental-link style jump thunk)
// address 0x4a62c0, size 5 bytes
// name confidence: 0.8  rewrite confidence: 1.0
// evidence: objdump 0x4a62c0: jmp 0x4a85f0 (player_profile_select_list_widget_build), reached as a stored widget
//   callback pointer; campaign track: widget_instance_render called it while the level's HUD widgets drew.
// blam-cc: stack -> widget (cdecl, forwarded unchanged)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#include "fn_interface.h"


void player_profile_select_list_widget_build_thunk(widget_instance *widget)
{
    player_profile_select_list_widget_build(widget);
}
