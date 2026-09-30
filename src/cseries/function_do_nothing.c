// function_do_nothing  (Ghidra: FUN_0044ad80)
// address 0x44ad80, size 1 byte
// name confidence: 0.7   rewrite confidence: 1.0
// evidence: the whole function is a bare `ret` (0xc3). Its address is stored as a no-op
//   callback by render_window and structure_picked_polygon_draw (structure_pass lightmap/
//   material callbacks), rasterizer_select_hardware_codepaths (0x007c0494),
//   network_listen_accept_pending_connection and network_join_request_resolve_host. A bare
//   ret leaves the arguments to the caller, so any __cdecl callback shape can point at it.
// register convention: plain __cdecl, no parameters read.
// blam-cc: (no arguments)

#include "tags.h"
#include "memory.h"
#include "fn_cseries.h"

// Returns immediately; used wherever a callback slot needs a harmless default.
void function_do_nothing(void)
{
}

#if 0
0x44ad80: ret
#endif
