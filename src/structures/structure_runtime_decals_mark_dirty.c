// structure_runtime_decals_mark_dirty  (not a Ghidra function; structure bsp activate slot 8)
// address 0x553060, size 9 bytes
// name confidence: 0.4  rewrite confidence: 0.95
// evidence: structure_bsp_activate_procedures[8] (0x0069e8fc) holds 0x553060; the byte it sets is the one
//   structure_decals_update_switch_transitions 0x5530d0 clears (runtime_decals_suppressed 0x0072278c).
// objdump 0x553060: mov eax,ds:0x72278c / mov BYTE PTR [eax],0x1 / ret
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint8_t *runtime_decals_suppressed; // 0x0072278c

void structure_runtime_decals_mark_dirty(void)
{
    *runtime_decals_suppressed = 1;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
