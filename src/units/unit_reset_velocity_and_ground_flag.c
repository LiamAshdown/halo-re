// unit_reset_velocity_and_ground_flag  (Ghidra: FUN_0056a290)
// address 0x56a290, size 119 bytes, name confidence 0.3, rewrite confidence 0.5
// functions.md: "Sets or clears a per-unit boolean flag, resets a cached 3D vector field from a
// shared default, and clears an additional state bit for biped-type units."
// evidence: types/units.h unit_flags._unit_flag_unknown_1000000 (0x1000000, "0x56a290");
//   global 0x00696714 global_origin3d_pointer (per the module's globals notes); types/objects.h
//   object.velocity (0x68); types/units.h biped_data.flags (0x4cc, bit 0 = grounded).
// blam-cc: in_EAX -> unit_index, param_1 -> set_flag.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data;         // 0x008603b0
extern real_point3d *global_origin3d_pointer;    // 0x00696714

void unit_reset_velocity_and_ground_flag(uint32_t unit_index, uint8_t set_flag) // blam-cc: in_EAX, param_1
{
    if (unit_index != 0xffffffff) {
        object *unit_obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
        unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);

        if (!set_flag) {
            unit->flags &= 0xfeffffff;
        } else {
            unit->flags |= 0x1000000;
        }
        unit_obj->velocity.i = global_origin3d_pointer->x;
        unit_obj->velocity.j = global_origin3d_pointer->y;
        unit_obj->velocity.k = global_origin3d_pointer->z;
        if (unit_obj->type == _object_type_biped) {
            biped_data *biped = (biped_data *)((uint8_t *)unit_obj + k_unit_object_size);
            biped->flags &= 0xfffffffe;
        }
    }
    return;
}

#if 0
Original Ghidra decompilation (0x56a290):

void FUN_0056a290(char param_1)

{
  uint *puVar1;
  int iVar2;
  undefined *puVar3;
  int iVar4;
  uint in_EAX;
  int iVar5;
  uint uVar6;

  iVar4 = DAT_008603b0;
  puVar3 = PTR_DAT_00696714;
  if (in_EAX != 0xffffffff) {
    iVar5 = (in_EAX & 0xffff) * 0xc;
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar5);
    if (param_1 == '\0') {
      uVar6 = *(uint *)(iVar2 + 0x204) & 0xfeffffff;
    }
    else {
      uVar6 = *(uint *)(iVar2 + 0x204) | 0x1000000;
    }
    *(uint *)(iVar2 + 0x204) = uVar6;
    *(undefined4 *)(iVar2 + 0x68) = *(undefined4 *)puVar3;
    *(undefined4 *)(iVar2 + 0x6c) = *(undefined4 *)(puVar3 + 4);
    *(undefined4 *)(iVar2 + 0x70) = *(undefined4 *)(puVar3 + 8);
    if (*(short *)(iVar2 + 0xb4) == 0) {
      puVar1 = (uint *)(*(int *)(*(int *)(iVar4 + 0x34) + 8 + iVar5) + 0x4cc);
      *puVar1 = *puVar1 & 0xfffffffe;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
