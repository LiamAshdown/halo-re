// main_queue_map_change_by_name_or_clear  (Ghidra: main_queue_map_change_by_name_or_clear,
// already named)
// address 0x4c87a0, size 91 bytes
// name confidence: 0.5   rewrite confidence: 0.75
// evidence: out/phase4/main_types_notes.md "Register arguments confirmed at call sites":
// "main_queue_map_change_by_name_or_clear 0x4c87a0: EDI = name or NULL (0x45fc81, 0x47f55d)".
// Confirmed against objdump -d -M intel bin/halo.exe at 0x4c87a0..0x4c87f6: both branches call
// cache_file_request_map with the SAME two arguments (ESI = &multiplayer_map_name,
// quit_on_fail = 0) -- Ghidra's own decompile shows one branch calling it with no visible
// arguments and the other with one, but the disassembly confirms it is the identical call both
// times, just preceded (when name is non-NULL) by resolving the friendly display name first.
// register convention: EDI -> name.
// phase 4 review (disassembly 0x4c87a0..0x4c87fa: no drift; 0x006b2f68 is progress_screen_subtext (types/interface.h).
// UNSURE: 0x006b2f68 is declared a single `int32_t` in every other file that touches it so far
// (src/networking/network_join_request_resolve_host.c, this module's
// network_game_client_connect_to_resolved_address.c), but this function's call into
// map_list_get_friendly_level_name (destination = 0x6b2f68, capacity 0x40 wchar_t) proves it is
// really at least a 0x40-uint16_t (0x80 byte) wide-string buffer -- it sits immediately after
// src/interface/interface_loading_screen_set_text.c's progress_screen_text[0x20] (0x006b2f28,
// ending at 0x006b2f68), so this is very likely a second, adjacent wide buffer (the loading
// screen's map-name line). Declared here with the wider shape this function's own evidence
// requires; the mismatch with the other files' narrower declaration is a pre-existing
// across-file inconsistency (each function file's externs are independent in this project), not
// something fixed here.

#include "tags.h"
#include "memory.h"
#include "interface.h"
#include "main.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern main_globals main_globals_data; // 0x00719700
extern uint16_t progress_screen_subtext[0x20]; // 0x006b2f68, foreign (types/interface.h); the level name is
    // written with capacity 0x40 (ESI) as the caller passes it

extern void map_list_get_friendly_level_name(wchar_t *destination, char *map_path,
                                              int32_t destination_capacity); // 0x494f50, foreign (interface module)
extern uint8_t cache_file_request_map(char *name, uint8_t quit_on_fail); // 0x442640, foreign (cache module)

// blam-cc: EDI -> name
// Stages name (or clears the staging buffer when name is NULL) as the multiplayer map to load,
// refreshes the loading screen's cached friendly display name for it, and kicks off
// cache_file_request_map on the staged buffer either way.
// UNSURE: strncpy(main_globals_data.multiplayer_map_name, name, 0xff) runs unconditionally,
// BEFORE the name == NULL check below (confirmed in objdump: the call at 0x4c87ac precedes the
// `test edi,edi` at 0x4c87b4) -- a literal NULL name would fault inside strncpy on retail
// hardware. Preserved exactly as disassembled; presumably no real call site at 0x45fc81/
// 0x47f55d exercises the NULL path with strncpy still faulting in practice.
uint8_t main_queue_map_change_by_name_or_clear(char *name)
{
    strncpy(main_globals_data.multiplayer_map_name, name, 0xff);
    main_globals_data.multiplayer_map_name[0xff] = 0;
    if (name == 0) {
        progress_screen_subtext[0] = 0;
    } else {
        map_list_get_friendly_level_name((wchar_t *)progress_screen_subtext, name, 0x40);
    }
    return cache_file_request_map(main_globals_data.multiplayer_map_name, 0);  // the original returns this call's result (EAX) unchanged
}

#if 0
Original Ghidra decompilation (0x4c87a0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void main_queue_map_change_by_name_or_clear(void)

{
  char *unaff_EDI;

  _strncpy(&DAT_00719879,unaff_EDI,0xff);
  DAT_00719978 = 0;
  if (unaff_EDI == (char *)0x0) {
    _DAT_006b2f68 = 0;
    cache_file_request_map();
    return;
  }
  map_list_get_friendly_level_name(&DAT_006b2f68);
  cache_file_request_map(0);
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
