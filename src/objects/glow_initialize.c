// glow_initialize  (not a Ghidra function; widget type callback)
// address 0x4fcbb0, size 78 bytes
// name confidence: 0.75  rewrite confidence: 0.9
// evidence:
//   It is a widget type callback in widget_type_definitions 0x0069c010 (type 'glw!'); only reachable through that
//   table, so Ghidra never made it a function. First-boot track: widgets_initialize reached it in the
//   standalone exe.
// objdump 0x4fcbb0..0x4fcbfd: when glow_data does not exist yet, game_state_new("glow", 8, 0x25c); when that
//   worked and glow_particle_data does not exist yet, game_state_new("glow particles", 0x200, 0x64).
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
extern data_array *game_state_new(char *name, int16_t maximum_count, int16_t element_size); // 0x5380d0, blam-cc: EBX -> element_size, stack -> name, maximum_count

void glow_initialize(void)
{
    if (glow_data != 0) {
        return;
    }
    glow_data = game_state_new((char *)"glow", 8, 0x25c);
    if (glow_data != 0 && glow_particle_data == 0) {
        glow_particle_data = game_state_new((char *)"glow particles", 0x200, 0x64);
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
