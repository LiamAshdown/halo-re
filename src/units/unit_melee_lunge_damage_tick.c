// unit_melee_lunge_damage_tick  (Ghidra: FUN_0056fc80; renamed from the phase2 proposal)
// address 0x56fc80, size 192 bytes
// name confidence: 0.35 (phase2 proposal at 0.35, matches functions.md summary)
// rewrite confidence: 0.2 -- the swept-plane collision test (object_collision_context_test_segment / matrix4x3_transform_plane)
//   is reproduced with Ghidra's own locals; only the object/unit fields and the damage_data
//   record are translated to named struct members.
// evidence: types/units.h unit_data.melee_state (0x289, ==4 the "lunge" sub-state),
//   .melee_damage_countdown (0x28a); types/objects.h object.parent_object (0x11c),
//   .forward (0x074), .name_index (0x0b8); types/tags.h Unit.melee_damage (tag_id at absolute
//   0x294); the damage_data field mapping follows the same offset pattern established in
//   unit_cause_melee_damage.c and unit_melee_attack_scan.c (team_index from name_index,
//   responsible_player from controlling_player, direction from object.forward).
// register convention: unit object index in EAX (param_1).
//   // blam-cc: EAX -> unit_index
// UNSURE: object_apply_damage's final parameter is a plain flags dword everywhere else in this
//   module, but here it is a real_vector3d* (or NULL) at the call site; reproduced as a raw
//   uint32_t cast of the pointer rather than widening the shared prototype.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;      // 0x008603b0
extern tag_instance *tag_instances;  // 0x0087bc14

extern void object_get_position(real_point3d *out, uint32_t object_index); // 0x4f6900
extern int8_t object_collision_context_build(void); // 0x504e10, collision module
extern int8_t object_collision_context_test_segment(real_plane3d *out_plane, int32_t mask, real_point3d *origin,
                            real_vector3d *delta, void *out_record); // 0x504f60, UNSURE signature
extern void matrix4x3_transform_plane(void); // 0x4cbf10, UNSURE: register args  // real signature (matrix4x3_transform_plane.c): void matrix4x3_transform_plane(real_plane3d *out, real_matrix4x3 *m, real_plane3d *plane); Ghidra recovered 0 of 3 args at this call site
extern void object_apply_damage(damage_data *dd, uint32_t object_index, int16_t param_3,
                                 int16_t param_4, int16_t param_5, uint32_t param_6); // 0x4ee5e0

// Applies a periodic melee/lunge damage tick while the unit is in melee sub-state 4 and attached
// to a parent object, using a swept collision-plane test along the unit's own forward direction
// to decide whether to apply a directional impulse alongside the damage.
void unit_melee_lunge_damage_tick(uint32_t unit_index)
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Unit *tag = (Unit *)tag_instances[obj->definition_tag & 0xffff].data;
    uint8_t hit_valid = 0;
    real_vector3d sweep_dir = {0};
    real_point3d hit_point = {0};
    int16_t record_field0 = 0, record_field1 = 0, record_field2 = 0;

    if (unit->melee_state != 4 || obj->parent_object == k_datum_index_none ||
        *(int32_t *)&tag->melee_damage.tag_id == -1) {
        return;
    }

    if (unit->melee_damage_countdown == 0 && object_collision_context_build() != 0) {
        real_point3d start;
        real_plane3d plane;
        uint8_t record[16]; // UNSURE: object_collision_context_test_segment's out_record layout; fStack_418 is its
                             // fraction field at +0x1c relative to this buffer's start
        int16_t plane_sign; // iStack_40c after matrix4x3_transform_plane

        object_get_position(&start, unit_index);
        sweep_dir.i = obj->forward.i * 0.2f;
        sweep_dir.j = obj->forward.j * 0.2f;
        sweep_dir.k = obj->forward.k * 0.2f;
        start.x -= sweep_dir.i * 0.5f;
        start.y -= sweep_dir.j * 0.5f;
        start.z -= sweep_dir.k * 0.5f;

        if (object_collision_context_test_segment(&plane, 3, &start, &sweep_dir, record) != 0) {
            float fraction = *(float *)(record + 0x08); // fStack_418
            hit_point.x = sweep_dir.i * fraction + start.x;
            hit_point.y = sweep_dir.j * fraction + start.y;
            hit_point.z = sweep_dir.k * fraction + start.z;
            record_field0 = *(int16_t *)(record + 0x00); // local_420
            record_field1 = *(int16_t *)(record + 0x02); // uStack_41e
            record_field2 = *(int16_t *)(record + 0x04); // uStack_41c
            matrix4x3_transform_plane(); // UNSURE: writes iStack_40c's sign via register(s)
            plane_sign = *(int16_t *)((uint8_t *)&plane + 0x0c); // UNSURE: raw offset placeholder
            if (plane_sign < 0) {
                sweep_dir.i = -sweep_dir.i;
                sweep_dir.j = -sweep_dir.j;
                sweep_dir.k = -sweep_dir.k;
            }
            hit_valid = 1;
        }
    }

    {
        damage_data dd = {0};
        int16_t p3, p4, p5;
        real_vector3d *impulse;

        dd.damage_effect_tag = *(datum_index *)&tag->melee_damage.tag_id;
        dd.flags = 0;
        dd.team_index = (int16_t)obj->name_index;
        *(int16_t *)&dd.location_cluster_index = -1;
        dd.responsible_player = obj->name_index; // UNSURE, see unit_cause_melee_damage.c note
        dd.responsible_object = unit_index;
        dd.random_blend = 0.033333335f;
        dd.multiplier = 1.0f;
        dd.unknown_4c = -1;

        if (hit_valid) {
            dd.epicentre = hit_point;
            dd.origin = hit_point;
            dd.direction = *(real_vector3d *)&obj->forward;
            dd.flags |= 2;
            unit->melee_damage_countdown = 10;
            // p3/p4/p5 are the three int16 fields object_collision_context_test_segment wrote into its out_record,
            // read back here as the object_apply_damage node/coordinate hint arguments.
            p3 = record_field0; p4 = record_field1; p5 = record_field2;
            impulse = &sweep_dir;
        } else {
            p3 = -1; p4 = -1; p5 = -1;
            impulse = 0;
        }

        object_apply_damage(&dd, obj->parent_object, p3, p4, p5, (uint32_t)impulse);
    }

    unit->melee_damage_countdown -= 1;
}

#if 0
Original Ghidra decompilation (0x56fc80):

void FUN_0056fc80(uint param_1)

{
  uint *puVar1;
  bool bVar2;
  char cVar3;
  int iVar4;
  uint *puVar5;
  uint uVar6;
  undefined4 uVar7;
  undefined4 uVar8;
  float *pfVar9;
  float local_4b4;
  float fStack_4b0;
  float fStack_4ac;
  float fStack_4a8;
  float local_4a4;
  float fStack_4a0;
  float fStack_49c;
  float fStack_498;
  float fStack_494;
  float fStack_490;
  int local_48c;
  uint auStack_488 [4];
  undefined2 uStack_478;
  undefined2 uStack_470;
  float fStack_46c;
  float fStack_468;
  float fStack_464;
  float fStack_460;
  float fStack_45c;
  float fStack_458;
  uint uStack_454;
  uint uStack_450;
  uint uStack_44c;
  undefined4 uStack_448;
  undefined4 uStack_444;
  undefined2 uStack_43c;
  undefined1 local_430 [16];
  undefined2 local_420;
  undefined2 uStack_41e;
  undefined2 uStack_41c;
  float fStack_418;
  int iStack_40c;
  undefined4 uStack_406;

  puVar1 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (param_1 & 0xffff) * 0xc);
  local_48c = *(int *)((*puVar1 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (((*(char *)((int)puVar1 + 0x289) == '\x04') && (puVar1[0x47] != 0xffffffff)) &&
     (*(int *)(local_48c + 0x294) != -1)) {
    bVar2 = false;
    if (*(char *)((int)puVar1 + 0x28a) == '\0') {
      cVar3 = FUN_00504e10();
      if (cVar3 != '\0') {
        object_get_position();
        local_4b4 = (float)puVar1[0x1d] * 0.2;
        fStack_4b0 = (float)puVar1[0x1e] * 0.2;
        fStack_4ac = (float)puVar1[0x1f] * 0.2;
        local_4a4 = local_4a4 - local_4b4 * 0.5;
        fStack_4a0 = fStack_4a0 - fStack_4b0 * 0.5;
        fStack_49c = fStack_49c - fStack_4ac * 0.5;
        cVar3 = FUN_00504f60(local_430,3,&local_4a4,&local_4b4,&local_420);
        if (cVar3 != '\0') {
          fStack_498 = local_4b4 * fStack_418 + local_4a4;
          fStack_494 = fStack_4b0 * fStack_418 + fStack_4a0;
          fStack_490 = fStack_4ac * fStack_418 + fStack_49c;
          matrix4x3_transform_plane();
          if (iStack_40c < 0) {
            local_4b4 = -local_4b4;
            fStack_4b0 = -fStack_4b0;
            fStack_4ac = -fStack_4ac;
            fStack_4a8 = -fStack_4a8;
          }
          bVar2 = true;
        }
      }
    }
    uStack_478 = (undefined2)puVar1[0x2e];
    puVar5 = auStack_488;
    for (iVar4 = 0x15; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    auStack_488[0] = *(uint *)(local_48c + 0x294);
    auStack_488[2] = puVar1[0x86];
    uStack_43c = 0xffff;
    uStack_470 = 0xffff;
    uStack_444 = 0x3f800000;
    auStack_488[3] = param_1;
    uStack_448 = 0x3d088889;
    if (bVar2) {
      fStack_458 = fStack_490;
      fStack_464 = fStack_490;
      fStack_460 = fStack_498;
      fStack_46c = fStack_498;
      uStack_454 = puVar1[0x1d];
      fStack_45c = fStack_494;
      fStack_468 = fStack_494;
      uStack_450 = puVar1[0x1e];
      uStack_44c = puVar1[0x1f];
      auStack_488[1] = auStack_488[1] | 2;
      pfVar9 = &local_4b4;
      uVar7 = CONCAT22(uStack_41e,local_420);
      uVar6 = puVar1[0x47];
      uVar8 = CONCAT22(uStack_41c,uStack_41e);
      *(undefined1 *)((int)puVar1 + 0x28a) = 10;
    }
    else {
      pfVar9 = (float *)0x0;
      uStack_406 = 0xffffffff;
      uVar8 = 0xffffffff;
      uVar7 = 0xffffffff;
      uVar6 = puVar1[0x47];
    }
    object_apply_damage(auStack_488,uVar6,uVar7,uVar8,uStack_406,pfVar9);
    *(char *)((int)puVar1 + 0x28a) = *(char *)((int)puVar1 + 0x28a) + -1;
  }
  return;
}
#endif
