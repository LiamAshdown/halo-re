// glow_dispose  (not a Ghidra function; widget type callback)
// address 0x4fcc00, size 41 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'glw!'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fcc00..0x4fcc28: each existing pool (glow_data, then glow_particle_data) gets its disposing byte
//   (+0x24) set and data_delete_all.
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
extern void data_delete_all(data_array *array); // 0x4d0580, blam-cc: ESI -> array

void glow_dispose(void)
{
    if (glow_data != 0) {
        glow_data->valid = 1;
        data_delete_all(glow_data);
    }
    if (glow_particle_data != 0) {
        glow_particle_data->valid = 1;
        data_delete_all(glow_particle_data);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
