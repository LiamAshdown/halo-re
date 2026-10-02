// glow_clear_disposing_flag  (not a Ghidra function; widget type callback)
// address 0x4fcc30, size 27 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'glw!'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fcc30..0x4fcc4a: clears the disposing byte (+0x24) of each existing glow pool.
// blam-cc: none

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *glow_data; // 0x008603a0
extern data_array *glow_particle_data; // 0x008603a4

void glow_clear_disposing_flag(void)
{
    if (glow_data != 0) {
        glow_data->valid = 0;
    }
    if (glow_particle_data != 0) {
        glow_particle_data->valid = 0;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
