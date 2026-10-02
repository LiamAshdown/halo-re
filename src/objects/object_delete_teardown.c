// object_delete_teardown
// address 0x4edc80, size 166 bytes
// name confidence: 0.35 (still FUN_004edc80 in Ghidra; named from behaviour per
// out/phase4/objects_functions.md: "Tears down an object's active links: clears stun state,
// notifies any linked object, recurses through its children, and runs type-specific cleanup for
// lifecycle states 0 and 3.")
// rewrite confidence: 0.9 (FIXED role-0 fallthrough; rest VERIFIED against objdump)
// evidence: types/objects.h object.network_role (0x004, "object_delete dispatches on 0 versus
// 3"); types/tags.h Object.collision_model (0x7c).
// register convention: uint32_t object_index in EAX (in_EAX).
// blam-cc: EAX=object_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "effects.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void object_set_health_frozen_flag(uint32_t object_index);   // 0x4eda20
extern void object_children_recurse_prune(uint32_t object_index);   // 0x4edc10
extern datum_index effect_new_on_object(datum_index creator_object_index, datum_index definition_index,
    datum_index object_index, int16_t first_person_weapon_override, real a_scale, real b_scale,
    const ColorRGB *color, const effect_tint_source *tint_source);
    // 0x4507a0, blam-cc: EAX -> creator_object_index, ECX -> definition_index, stack -> the other six
extern void object_delete_unparented(uint32_t object_index); // blam-cc: EDI -> object_index // UNSURE: zero visible args; objects module, 0x4f5aa0 (out of range)
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0, stack -> object_index, recurse_siblings

void object_delete_teardown(uint32_t object_index)
{
    object_header *headers = (object_header *)object_data->data;
    object *obj = headers[object_index & 0xffff].data;
    Object *definition = (Object *)tag_instances[obj->definition_tag & 0xffff].data;

    object_set_health_frozen_flag(object_index);

    if (definition->collision_model.tag_id.index != 0xffff) {
        // 0x4edcbf..0x4edce4: EAX = the object, ECX = the collision model's +0xc8 effect, stack: object, -1, 0..
        effect_new_on_object(object_index,
            *(datum_index *)((uint8_t *)tag_instances[definition->collision_model.tag_id.index].data + 0xc8),
            object_index, -1, 0.0f, 0.0f, 0, 0);
    }

    object_children_recurse_prune(object_index);

    // FIXED (objdump 0x4edcff..0x4edd1f): role 0 runs object_delete_unparented and then falls through into
    //   object_delete_recursive(object, 0); role 3 runs only object_delete_recursive. The draft never deleted
    //   role-0 objects (single player), leaving them alive after the teardown.
    obj = headers[object_index & 0xffff].data;
    if (obj->network_role == 0 || obj->network_role == 3) {
        if (obj->network_role == 0) {
            object_delete_unparented(object_index); // EDI carries the handle
        }
        object_delete_recursive(object_index, 0);
    }
}

#if 0
Original Ghidra decompilation (0x4edc80):

void FUN_004edc80(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;

  iVar2 = (in_EAX & 0xffff) * 0xc;
  iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  FUN_004eda20();
  if (*(int *)(iVar1 + 0x7c) != -1) {
    FUN_004507a0();
  }
  FUN_004edc10();
  iVar1 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar2) + 4);
  if (iVar1 == 0) {
    FUN_004f5aa0();
  }
  else if (iVar1 != 3) {
    return;
  }
  FUN_004f59d0();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
