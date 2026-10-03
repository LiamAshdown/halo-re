// object_new
// address 0x4f5460, size 67 bytes
// name confidence: 0.35 (still FUN_004f5460 in Ghidra; functions.md: "Thin wrapper entry point
//   that forwards directly to object_new_with_datum_role_control" -- named as the natural public
//   entry point for the internal constructor)
// rewrite confidence: 0.9 (VERIFIED against objdump 0x4f5460..0x4f54a2)
// evidence: callee object_new_with_datum_role_control 0x4f54b0.
// register convention: placement in ECX (mov edx,[ecx] at entry reads placement->definition_tag,
//   and ecx is pushed unchanged as object_new_with_datum_role_control's first stack argument at
//   0x4f5498). Several other modules already declare this function as
//   `object_new(object_placement_data *placement)` returning datum_index (e.g.
//   src/ai/actor_place_new_unit.c), matching what objdump shows.
//   // blam-cc: ECX -> placement
// FIXED (register inputs, objdump): ECX carries placement (read at 0x4f5470, mov edx,[ecx]); it
// was missing, and the whole body (the role selection) had been elided as a zero-argument call.
// The role computation is reconstructed from objdump 0x4f5460..0x4f549e: role defaults to 3, and
// becomes 0 when network_game_mode == 2 and the placement's object type still sends network
// delta updates (network_delta_message_type != -1) -- the identical idiom already rewritten in
// src/ai/actor_place_new_unit.c and src/ai/actor_apply_unit_definition_properties.c.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern int16_t network_game_mode; // 0x00719720
extern tag_instance *tag_instances; // 0x0087bc14
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role); // 0x4f54b0, this module

// blam-cc: ECX -> placement
datum_index object_new(object_placement_data *placement)
{
    uint32_t role = 3;

    if (network_game_mode == 2) {
        Object *definition = (Object *)tag_instances[(uint16_t)placement->definition_tag].data;
        if (object_type_definitions[definition->object_type]->network_delta_message_type != -1) {
            role = 0;
        }
    }

    return object_new_with_datum_role_control(placement, role);
}

#if 0
Original Ghidra decompilation (0x4f5460):

void FUN_004f5460(void)

{
  object_new_with_datum_role_control();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
