// lightnings_clear_disposing_flag  (not a Ghidra function; widget type callback)
// address 0x4feec0, size 14 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'elec'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4feec0..0x4feecd: clears the pool's disposing byte (+0x24) when it exists.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *lightning_instances; // 0x006b8d74

void lightnings_clear_disposing_flag(void)
{
    if (lightning_instances != 0) {
        lightning_instances->valid = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
