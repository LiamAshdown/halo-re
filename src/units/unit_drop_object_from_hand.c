// unit_drop_object_from_hand  (Ghidra: FUN_0056ed00)
// address 0x56ed00, size 596 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// REWRITTEN from objdump 0x56ed00..0x56ef53. The draft called item_accelerate without the item, the root-velocity
//   helper without operands (reading an uninitialised float as the toss), and the reposition with a bogus point.
//   Stack: (unit, object). An object that is not attached is first put in a cluster, unhidden and placed at the
//   unit's "left hand"; then it is detached (item flags &= ~3), stopped, and tossed along the unit's aim randomised
//   within pi/8 at 0.0267..0.04 per tick plus the unit's root velocity (item +0x200 remembers the unit), swept back
//   from the unit's camera (0x4f7b70; deleted in single player when that fails), and deleted when the unit's
//   dropped items vanish (unit +0x204 bit 0x100000).
// blam-cc: stack -> unit_index, object_index

// FIXED 2026-09-28: global_origin3d_pointer here is the global at its address comment, global_zero_vector3d_pointer (the name belonged to another
// global at a different address, so the link bound it there).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#include "fn_units.h"
#include "fn_objects.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14
extern real_vector3d *global_origin3d_pointer; // 0x00696714
extern random_seed random_seed_global;    // 0x00719cd0
extern game_engine_definition *current_game_engine;
extern char s_left_hand_marker[];    // 0x00671ffc "left hand"
extern char k_empty_string[];   // 0x0065512c ""


extern void object_for_each_light_attachment(uint32_t object_index, int32_t register_in_table,
                                              int32_t invoke_callback); // 0x4f9a20, EAX, stack
extern void object_reorient_relative_to_marker(uint32_t parent_index, char *parent_marker_name,
                                                uint32_t object_index, char *object_marker_name); // 0x4f6180, stack, ESI, EDI
extern void object_snap_to_parent_marker_and_detach(uint32_t object_index); // 0x4f6610
extern real_vector3d *vector3d_randomize_direction(real_point3d *direction, real_vector3d *out,
    random_seed *seed, real lo, real hi); // 0x4cd1b0, EAX, EBX, EDI, stack
extern void object_get_root_object_velocities(uint32_t object_index, real_vector3d *out_velocity,
    real_vector3d *out_angular_velocity); // 0x4f6aa0, EAX, ESI, EDI
extern void item_accelerate(uint32_t item_index, real_vector3d *delta, uint8_t apply_detonation_timer); // 0x4bd080, EAX, stack
extern void unit_get_camera_position(uint32_t unit_index, real_point3d *out); // 0x568f80, ECX, EDI

extern void object_delete(uint32_t object_index);            // 0x4f5bd0, EAX
extern void object_delete_unparented(uint32_t object_index); // 0x4f5aa0, EDI
extern void object_delete_recursive(uint32_t object_index, uint8_t recurse_siblings); // 0x4f59d0

#define OBJECT_DATA(h) ((uint8_t *)((object_header *)object_data->data)[(h) & 0xffff].data)

void unit_drop_object_from_hand(uint32_t unit_index, uint32_t object_index)
{
    uint8_t *unit = OBJECT_DATA(unit_index);         // [esp+0x14]
    uint8_t *dropped = OBJECT_DATA(object_index);    // [esp+0x1c]
    real_vector3d toss;                              // [esp+0x20]
    real_vector3d root_velocity;                     // [esp+0x2c]
    real_point3d camera;                             // [esp+0x38]
    real speed;
    int32_t role;

    if (((struct object *)dropped)->parent_object == k_datum_index_none) {
        uint8_t *object;
        uint8_t *object_tag;

        object_set_cluster_and_parent(object_index, 0);
        object = OBJECT_DATA(object_index);
        object_tag = (uint8_t *)tag_instances[*(datum_index *)object & 0xffff].data;
        if (*(int32_t *)(object_tag + 0x34) != -1) {
            if (object[0x10] & 1) {
                object_for_each_light_attachment(object_index, 0, 1);
            }
            if (*(int32_t *)(object_tag + 0x34) != -1) {
                *(uint32_t *)(object + 0x10) &= ~1u;
                ((object_header *)object_data->data)[object_index & 0xffff].flags |= 2;
            }
        }
        object_reorient_relative_to_marker(unit_index, s_left_hand_marker, object_index, k_empty_string);
    }
    *(uint32_t *)(OBJECT_DATA(object_index) + 0x1f4) &= ~3u;
    object_snap_to_parent_marker_and_detach(object_index);
    *(real_vector3d *)&((struct object *)dropped)->velocity.i = *global_origin3d_pointer;
    *(real_vector3d *)&((struct object *)dropped)->angular_velocity.i = *global_origin3d_pointer;

    // 0x56ee29: toss along the aim
    vector3d_randomize_direction((real_point3d *)(unit + 0x23c), &toss, &random_seed_global, 0.0f, 0.39269909f);
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    speed = (real)(int32_t)((uint32_t)random_seed_global >> 16) * 1.5259022e-05f * 0.013333336f + 0.026666667f;
    toss.i *= speed;
    toss.j *= speed;
    toss.k *= speed;
    object_get_root_object_velocities(unit_index, &root_velocity, 0);
    toss.i += root_velocity.i;
    *(datum_index *)(dropped + 0x200) = unit_index;
    toss.j += root_velocity.j;
    toss.k += root_velocity.k;
    item_accelerate(object_index, &toss, 0);

    unit_get_camera_position(unit_index, &camera);
    if (!object_reposition_to_spawn_location(object_index, &camera, k_datum_index_none) && current_game_engine == 0) {
        object_delete(object_index);
    }
    if (((unit_object *)unit)->unit.flags & 0x100000) {
        role = *(int32_t *)(OBJECT_DATA(object_index) + 0x4);
        if (role == 0) {
            object_delete_unparented(object_index);
            object_delete_recursive(object_index, 0);
        } else if (role == 3) {
            object_delete_recursive(object_index, 0);
        }
    }
}

#if 0
Original Ghidra decompilation (0x56ed00):

void unit_drop_object_from_hand(uint param_1,uint param_2)

{
  byte *pbVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  int iVar5;
  float fVar6;
  undefined *puVar7;
  char cVar8;
  int iVar9;
  float10 fVar10;
  float local_24;
  float local_20;
  float local_1c;
  float local_18;
  float local_14;
  float local_10;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  iVar9 = (param_2 & 0xffff) * 0xc;
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
  if (*(int *)(iVar3 + 0x11c) == -1) {
    object_set_cluster_and_parent(param_2,0);
    puVar4 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
    iVar5 = *(int *)((*puVar4 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
    if (*(int *)(iVar5 + 0x34) != -1) {
      if ((puVar4[4] & 1) != 0) {
        object_for_each_light_attachment(0,1);
      }
      if (*(int *)(iVar5 + 0x34) != -1) {
        iVar5 = *(int *)(DAT_008603b0 + 0x34);
        puVar4[4] = puVar4[4] & 0xfffffffe;
        pbVar1 = (byte *)(iVar5 + iVar9 + 2);
        *pbVar1 = *pbVar1 | 2;
      }
    }
    object_reorient_relative_to_marker(param_1,"left hand");
  }
  iVar5 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9);
  *(uint *)(iVar5 + 500) = *(uint *)(iVar5 + 500) & 0xfffffffc;
  FUN_004f6610(param_2);
  puVar7 = PTR_DAT_00696714;
  *(undefined4 *)(iVar3 + 0x68) = *(undefined4 *)PTR_DAT_00696714;
  *(undefined4 *)(iVar3 + 0x6c) = *(undefined4 *)(puVar7 + 4);
  *(undefined4 *)(iVar3 + 0x70) = *(undefined4 *)(puVar7 + 8);
  *(undefined4 *)(iVar3 + 0x8c) = *(undefined4 *)puVar7;
  *(undefined4 *)(iVar3 + 0x90) = *(undefined4 *)(puVar7 + 4);
  *(undefined4 *)(iVar3 + 0x94) = *(undefined4 *)(puVar7 + 8);
  vector3d_randomize_direction(0,0x3ec90fdb);
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  fVar6 = (float)(random_seed_global >> 0x10) * 1.5259022e-05 * 0.0133333355 + 0.026666667;
  local_20 = fVar6 * local_20;
  local_1c = local_1c * fVar6;
  fVar10 = (float10)FUN_004f6aa0();
  local_24 = (float)(fVar10 + (float10)local_18);
  *(uint *)(iVar3 + 0x200) = param_1;
  local_20 = local_14 + local_20;
  local_1c = local_10 + local_1c;
  item_accelerate(&local_24,0);
  unit_get_camera_position();
  cVar8 = FUN_004f7b70(param_2,0xffffffff);
  if ((cVar8 == '\0') && (DAT_006f1d20 == 0)) {
    object_delete();
  }
  if ((*(uint *)(iVar2 + 0x204) & 0x100000) != 0) {
    iVar2 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar9) + 4);
    if (iVar2 == 0) {
      object_delete_unparented();
    }
    else if (iVar2 != 3) {
      return;
    }
    object_delete_recursive(param_2,0);
  }
  return;
}
#endif
