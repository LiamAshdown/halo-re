// cheat_spawn_objects_near_camera  (Ghidra: cheat_spawn_objects_near_camera, already named)
// address 0x45a800, size 441 bytes
// name confidence: 0.5   rewrite confidence: 0.95
// evidence: types/objects.h object_placement_data (0x88 bytes: position 0x18, up 0x40); the
//   struct writes at local_70/6c/68 (position) and local_48/44/40 (up) land exactly on those
//   fields relative to a base at local_88.
// register convention: `tag_array` and `count` are Ghidra's own recognized __cdecl parameters.
//
// Verified against the disassembly 0x45a800..0x45a9b8 (see the REWRITTEN note below): the position/orientation calls take EAX/ECX
// (unit) registers, and the yaw stays on the x87 stack across object_placement_data_initialize, so the angle math is
// done in double here (extended precision in the original); the floats are only stored into the placement.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern uint32_t cheat_get_target_object_index(void); // this batch, 0x45a7a0
extern int16_t network_game_mode;   // 0x00719720
extern tag_instance *tag_instances; // 0x0087bc14
extern void *object_type_definitions[12]; // 0x0069bfdc, an ARRAY of the 12 object type definitions (was a pointer variable)
                                       // tag's object-type byte, +0x10 tested against -1

extern void object_get_position(real_point3d *out, datum_index object_index); // 0x4f6900, blam-cc: EAX -> out, ECX -> object_index (matches src/objects/object_get_position.c)
extern void object_get_orientation(real_vector3d *out_forward, datum_index object_index, real_vector3d *out_up); // 0x4f6970, blam-cc: EAX -> out_forward, ECX -> object_index, stack -> out_up (matches src/objects/object_get_orientation.c)
extern double atan2(double y, double x); // x87 FPATAN
extern double sin(double x);             // x87 FSIN
extern double cos(double x);             // x87 FCOS
extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0, canonical form (src/items)
extern datum_index object_new_with_datum_role_control(object_placement_data *placement, uint32_t role); // 0x4f54b0

extern data_array *player_data; // 0x0087a480, records 0x200 bytes, unit at +0x34

// REWRITTEN 2026-09-28 from objdump 0x45a800..0x45a9b8. 0x45a7a0 returns a PLAYER index (the first player with a
//   unit); the position and basis come from that player's unit (player +0x34). The draft passed the player index
//   to object_get_position as if it were an object, so the cheats spawned relative to a garbage object.
//   Object k of `count` is placed 1.5 units out at yaw + (k - count/2) * min(2 pi / count, pi / 8), yaw being
//   atan2(forward.i, forward.j) (the original's fpatan operand order), 0.8 above the unit's origin, with the
//   unit's forward and up; role 3, or 0 for a network client (game mode 2) when the object type definition's
//   +0x10 is not -1.
void cheat_spawn_objects_near_camera(TagDependency *tag_array, int16_t count)
{
    uint32_t player_index = cheat_get_target_object_index();
    datum_index unit;
    real_point3d unit_position;  // esp+0x18
    real_vector3d unit_forward;  // esp+0x0c
    real_vector3d unit_up;       // esp+0x24
    int32_t i;

    if (player_index == 0xffffffff) {
        return;
    }
    if (count <= 0) { // 0x45a853: `test ax,ax; jle` -- the draft looped (uint16_t)count times for a negative count
        return;
    }
    unit = *(datum_index *)((uint8_t *)player_data->data + (player_index & 0xffff) * 0x200 + 0x34);
    object_get_position(&unit_position, unit);
    object_get_orientation(&unit_forward, unit, &unit_up);

    for (i = 0; i < (int32_t)(uint16_t)count; i++) {
        datum_index tag_handle = *(datum_index *)&tag_array[i].tag_id;
        object_placement_data placement;
        double spacing;
        double angle;
        uint32_t role;

        if (tag_handle == k_datum_index_none) {
            continue;
        }
        spacing = (double)6.2831855f / (double)(int32_t)count; // 0x672c20 / count, kept in extended precision
        if (!(spacing <= (double)0.39269909f)) {                // 0x673160 = pi / 8
            spacing = (double)0.39269909f;
        }
        angle = atan2((double)unit_forward.i, (double)unit_forward.j) +
            (double)(i - (int32_t)count / 2) * spacing;
        object_placement_data_initialize(&placement, tag_handle, k_datum_index_none);
        placement.forward = unit_forward;
        placement.up = unit_up;
        role = 3;
        placement.position.x = (float)(cos(angle) * (double)1.5f + (double)unit_position.x);
        placement.position.y = (float)(sin(angle) * (double)1.5f + (double)unit_position.y);
        placement.position.z = unit_position.z + 0.8f;
        if (network_game_mode == 2) {
            int16_t object_type = *(int16_t *)tag_instances[placement.definition_tag & 0xffff].data;

            if (*(int32_t *)((uint8_t *)object_type_definitions[object_type] + 0x10) != -1) {
                role = 0;
            }
        }
        object_new_with_datum_role_control(&placement, role);
    }
}

#if 0
Original Ghidra decompilation (0x45a800), from tools/pack.py 0x45a800:

void __cdecl cheat_spawn_objects_near_camera(int tag_array,ushort count)

{
  int iVar1;
  undefined4 uVar2;
  unkbyte10 Var3;
  float10 fVar4;
  int *local_b4;
  uint local_b0;
  float local_ac;
  float local_a8;
  float local_a0;
  float local_9c;
  float local_98;
  undefined4 local_94;
  undefined4 local_90;
  undefined4 local_8c;
  uint local_88 [6];
  float local_70;
  float local_6c;
  float local_68;
  undefined4 local_48;
  undefined4 local_44;
  undefined4 local_40;

  iVar1 = FUN_0045a7a0();
  if (iVar1 != -1) {
    object_get_position();
    object_get_orientation(&local_94);
    if (0 < (short)count) {
      local_b0 = (uint)count;
      local_b4 = (int *)(tag_array + 0xc);
      do {
        if (*local_b4 != -1) {
          fpatan((float10)local_ac,(float10)local_a8);
          Var3 = object_placement_data_initialize(*local_b4,0xffffffff);
          fVar4 = (float10)fcos(Var3);
          local_48 = local_94;
          local_44 = local_90;
          local_40 = local_8c;
          uVar2 = 3;
          local_70 = (float)(fVar4 * (float10)1.5 + (float10)local_a0);
          fVar4 = (float10)fsin(Var3);
          local_6c = (float)(fVar4 * (float10)1.5 + (float10)local_9c);
          local_68 = local_98 + 0.8;
          if ((DAT_00719720 == 2) &&
             (*(int *)((&PTR_PTR_0069bfdc)
                       [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10)
              != -1)) {
            uVar2 = 0;
          }
          object_new_with_datum_role_control(local_88,uVar2);
        }
        local_b4 = local_b4 + 4;
        local_b0 = local_b0 - 1;
      } while (local_b0 != 0);
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
