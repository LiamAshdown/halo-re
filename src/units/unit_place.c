// unit_place  (not a Ghidra function; the unit (biped and vehicle) type's +0x2c (placed from the scenario) callback)
// address 0x558d40, size 16 bytes
// name confidence: 0.65  rewrite confidence: 0.9
// evidence: object_type_definition unit (biped and vehicle) (0x0069b428) field +0x2c (placed from the scenario); the object type dispatch (object_type_definitions_*)
//   calls it cdecl with the object handle and the placement. Only reachable through that table (never a Ghidra function);
//   first-boot track: needed while placing the UI map's objects.
//   objdump 0x558d40..0x558d4f: a tail call of unit_apply_scale_change 0x562030 with EAX the object and ECX
//   the unit part of the placement (+0x48).
// blam-cc: stack -> object_index, placement (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *object_data; // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

static uint8_t *object_get(datum_index object_index)
{
    return *(uint8_t **)((uint8_t *)object_data->data + (object_index & 0xffff) * 0xc + 8);
}


extern void unit_apply_scale_change(uint32_t unit_index, void *request); // 0x562030, blam-cc: EAX -> unit_index,
    // ECX -> request

void unit_place(datum_index object_index, uint8_t *placement)
{
    unit_apply_scale_change(object_index, placement + 0x48);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
