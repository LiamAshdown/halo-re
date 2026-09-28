// object_new_from_scenario_placement  (Ghidra: FUN_004f9b70; formerly named object_get_or_build_render_permutation)
// address 0x4f9b70, size 234 bytes
// name confidence: 0.8  rewrite confidence: 0.9
// evidence: scenario_objects_place 0x4f3ba0 and scenario_objects_place_for_structure_bsp 0x4f4880 call it once per
//   scenario placement with EDI = the placement and the type's palette block on the stack.
// REWRITTEN (first-boot track, objdump 0x4f9b70..0x4f9c59): the old version called object_new and the basis helper
//   with no arguments and passed a 16-byte scratch buffer as the placement data. The body:
//   - nothing (result -1) when the placement's palette index (+0) is -1; when object_globals byte 0 is set and the
//     placement's flags (+4) bit 0 is set; or when its name index (+2) is 0..0x1ff and already names an object
//   - the palette entry (0x30 bytes, tag at +0xc) of the placement's type gives the definition; -1 there is nothing
//   - object_placement_data_initialize(&data, tag, -1); position = placement +0x08; forward / up from the Euler
//     angles at +0x14 (euler_angles_to_basis_vectors); data +0x16 = placement +0x06; object_new(&data)
//   - a created object gets object_type_definitions_notify_two_args_0x2c(object, placement) and, when the
//     placement has a name, object_reserve_render_cache_slot(object, name)
// blam-cc: EDI -> placement, stack -> palette
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"

extern object_globals *object_globals_pointer; // 0x006b8cbc
extern datum_index *object_name_list; // 0x006b8cb8

extern void object_placement_data_initialize(object_placement_data *placement, datum_index definition_tag,
    datum_index role); // 0x4f53a0, blam-cc: EAX -> placement, stack -> definition_tag, role
extern void euler_angles_to_basis_vectors(real_euler_angles3d *angles, real_vector3d *up_out,
    real_vector3d *forward_out); // 0x4cdde0, blam-cc: EAX -> angles, EDX -> up_out, ESI -> forward_out
extern datum_index object_new(object_placement_data *placement); // 0x4f5460, blam-cc: ECX -> placement
extern void object_type_definitions_notify_two_args_0x2c(uint32_t object_index, uint32_t event_argument); // 0x4f3f20
extern void object_reserve_render_cache_slot(uint32_t object_index, int16_t slot); // 0x4f9ac0, EDX object, CX slot

datum_index object_new_from_scenario_placement(uint8_t *placement, TagReflexive *palette)
{
    int16_t type = *(int16_t *)placement;
    int16_t name = *(int16_t *)(placement + 2);
    datum_index tag;
    datum_index object;
    object_placement_data data;

    if (type == -1) {
        return k_datum_index_none;
    }
    if (*(uint8_t *)object_globals_pointer != 0 && (placement[4] & 1) != 0) {
        return k_datum_index_none;
    }
    if (name != -1 && name >= 0 && name < 0x200 && object_name_list[name] != k_datum_index_none) {
        return k_datum_index_none;
    }
    tag = *(datum_index *)((uint8_t *)palette->pointer + type * 0x30 + 0xc);
    if (tag == k_datum_index_none) {
        return k_datum_index_none;
    }
    object_placement_data_initialize(&data, tag, k_datum_index_none);
    data.position = *(real_point3d *)&((struct object_placement_data *)placement)->owner_linkage;
    euler_angles_to_basis_vectors((real_euler_angles3d *)(placement + 0x14), &data.up, &data.forward);
    data.permutation_group = *(int16_t *)(placement + 0x06);
    object = object_new(&data);
    if (object != k_datum_index_none) {
        object_type_definitions_notify_two_args_0x2c(object, (uint32_t)placement);
        if (name != -1) {
            object_reserve_render_cache_slot(object, name);
        }
    }
    return object;
}

#if 0
Original Ghidra decompilation (0x4f9b70):

int FUN_004f9b70(int param_1)

{
  short sVar1;
  int iVar2;
  int iVar3;
  short *unaff_EDI;

  iVar3 = -1;
  if ((((*unaff_EDI != -1) && ((*DAT_006b8cbc == '\0' || ((*(byte *)(unaff_EDI + 2) & 1) == 0)))) &&
      ((sVar1 = unaff_EDI[1], sVar1 == -1 ||
       (((sVar1 < 0 || (0x1ff < sVar1)) || (*(int *)(DAT_006b8cb8 + sVar1 * 4) == -1)))))) &&
     (iVar2 = *(int *)(*unaff_EDI * 0x30 + *(int *)(param_1 + 4) + 0xc), iVar2 != -1)) {
    FUN_004f53a0(iVar2,0xffffffff);
    FUN_004cdde0();
    iVar3 = FUN_004f5460();
    if ((iVar3 != -1) && (FUN_004f3f20(iVar3), unaff_EDI[1] != -1)) {
      FUN_004f9ac0();
    }
  }
  return iVar3;
}
#endif
