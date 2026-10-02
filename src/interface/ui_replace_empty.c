// ui_replace_empty  (not a Ghidra function; ui_replace_function_table[0])
// address 0x4a8750, size 6 bytes
// name confidence: 0.6  rewrite confidence: 1.0
// evidence: ui_replace_function_table 0x00692c08 (the widget text search/replace callbacks 0x4a8750, 0x4a8840,
//   0x4a8760, 0x4a8810); only reachable through that table. Campaign track: menu text rendering.
// objdump 0x4a8750: returns the empty wide string at 0x00660c34.
// blam-cc: stack -> widget (cdecl, not read); returns EAX

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "interface.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint16_t empty_string[]; // 0x00660c34

void *ui_replace_empty(widget_instance *widget)
{
    return empty_string;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
