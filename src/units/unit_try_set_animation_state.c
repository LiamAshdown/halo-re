// unit_try_set_animation_state  (Ghidra: unit_try_set_animation_state, already named)
// address 0x565f90, size 942 bytes
// name confidence: 0.5 (already carries this name)   rewrite confidence: 0.3
// evidence: types/units.h unit_data.animation_state (0x2a3), .animation_instance (0x29a),
//   .unknown_29c (0x29c), .animation_definition_index (0x2a0), .animation_weapon_index (0x2a1);
//   types/objects.h object.animation_graph/animation_index/animation_frame (0xcc/0xd0/0xd2),
//   object.definition_tag (0x000); types/tags.h Object.animation_graph (TagDependency at 0x38,
//   tag_id at 0x44), ModelAnimations.units (TagReflexive at 0xc),
//   ModelAnimationsAnimationGraphUnitSeat (0x64, weapons TagReflexive at 0x58, own animations
//   TagReflexive at 0x40), ModelAnimationsAnimationGraphWeapon (0xbc, animations TagReflexive
//   at 0x98), ModelAnimationsAnimationWeaponClassAnimation (a bare uint16 animation index);
//   unit_animation_state_from_seat_type (0x565da0), unit_release_thrown_grenade (0x56e440).
// register convention: unit index in EAX, requested state in DX.
//   // blam-cc: param_1 (EAX) -> unit_index, param_2 (DX) -> new_state
// UNSURE: the big switch maps new_state to a raw animation index either through the weapon's
//   own animation table (the default path) or, for the values that `goto LAB_00566193` in the
//   original, through the unit-seat's own animation table -- both tables are
//   ModelAnimationsAnimationWeaponClassAnimation (bare uint16 arrays), reproduced as int16_t*
//   casts of the TagReflexive pointer field since no per-element struct is named for either.
// UNSURE: at LAB_00566284 the original calls unit_animation_state_from_seat_type() twice with no visible arguments,
//   gated first on `extraout_DL == 0` -- a register Ghidra could not trace back to any write in
//   this function, most likely internal jump-table dispatch noise rather than a designed input.
//   The two calls are modelled here as unit_animation_state_from_seat_type(old_state) and
//   (new_state), which is what the surrounding compare needs, but the extraout_DL gate itself
//   is dropped (always taken) since nothing in this decompilation explains what it tests; this
//   only changes whether the animation_instance/unknown_29c reallocation and the object_copy_default_node_transforms
//   notify run on a same-category transition, not the state write or the return value.
// UNSURE: the upper 24 bits of the return value are undefined register garbage in the original
//   (CONCAT31); only the low "success" byte is meaningful, matching the house simplification
//   used throughout this codebase (see src/memory/bit_stream_write_bit.c).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern int16_t animation_choose_random_permutation(uint32_t flag);   // 0x4d6280, UNSURE: allocates some kind of instance/token
extern void object_copy_default_node_transforms(void);               // 0x4f6b70, UNSURE: no traced arguments  // real signature (object_copy_default_node_transforms.c): void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); Ghidra recovered 0 of 2 args at this call site
extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction); // 0x56e440
extern int32_t unit_animation_state_from_seat_type(int16_t animation_state); // 0x565da0

uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state) // blam-cc: see file header
{
    object *obj = ((object_header *)object_data->data)[unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    Object *obj_tag = (Object *)tag_instances[obj->definition_tag & 0xffff].data;
    void *graph = tag_instances[obj_tag->animation_graph.tag_id.index].data;
    uint8_t *unit_block = *(uint8_t **)((uint8_t *)graph + 0x10); // ModelAnimations.units.pointer
    ModelAnimationsAnimationGraphUnitSeat *unit_seat =
        (ModelAnimationsAnimationGraphUnitSeat *)(unit_block + unit->animation_definition_index * 100);
    ModelAnimationsAnimationGraphWeapon *weapon_anim =
        (ModelAnimationsAnimationGraphWeapon *)((uint8_t *)unit_seat->weapons.pointer +
                                                 unit->animation_weapon_index * 0xbc);

    int8_t old_state = unit->animation_state;
    uint8_t allocated = 0;
    int16_t raw_index;

    if (old_state != -1 && new_state == old_state) {
        goto category_check;
    }
    if (old_state == _unit_animation_state_throwing_grenade) {
        unit_release_thrown_grenade(unit_index, 1);
    }

    switch (new_state) {
    case 0: raw_index = 0; break;
    case 1: raw_index = 1; break;
    case 2: raw_index = 2; break;
    case 3: raw_index = 3; break;
    case 4: raw_index = 8; break;
    case 5: raw_index = 9; break;
    case 6: raw_index = 10; break;
    case 7: raw_index = 0xb; break;
    case 8: raw_index = 0x23; break;
    case 9: raw_index = 0x24; break;
    case 10: raw_index = 0x25; break;
    case 0xb: raw_index = 0x26; break;
    case 0xc: raw_index = 0xc; break;
    case 0xd: raw_index = 0xd; break;
    case 0xe: raw_index = 0xe; break;
    case 0xf: raw_index = 0xf; break;
    case 0x10: raw_index = 0x17; goto via_unit_seat_table;
    case 0x11: raw_index = 0x18; goto via_unit_seat_table;
    case 0x12: raw_index = 0x19; goto via_unit_seat_table;
    case 0x13: raw_index = 0x1a; goto via_unit_seat_table;
    case 0x14: raw_index = 0x10; break;
    case 0x15: raw_index = 0x11; break;
    case 0x16: raw_index = 0x12; break;
    case 0x18: raw_index = 0; goto via_unit_seat_table;
    case 0x19: raw_index = 1; goto via_unit_seat_table;
    case 0x1e: raw_index = 0x27; break;
    case 0x1f: raw_index = 0x2a; break;
    case 0x20: raw_index = 0x2e; break;
    case 0x21: raw_index = 0x14; break;
    case 0x22: raw_index = 0x2c; break;
    case 0x23: raw_index = 0x2d; break;
    case 0x24: raw_index = 0x2f; break;
    case 0x25: raw_index = 0x1b; goto via_unit_seat_table;
    case 0x26: raw_index = 0x1c; goto via_unit_seat_table;
    case 0x27: raw_index = 0x30; break;
    case 0x28: raw_index = 0x31; break;
    case 0x29: raw_index = 0x32; break;
    case 0x2b: raw_index = 0x1d; goto via_unit_seat_table;
    default:
        goto invalid_animation;
    }

    if (raw_index < (int32_t)weapon_anim->animations.count &&
        *(int16_t *)((uint8_t *)weapon_anim->animations.pointer + raw_index * 2) != -1) {
        goto allocate;
    }
    goto invalid_animation;

via_unit_seat_table:
    if (raw_index < (int32_t)unit_seat->animations.count &&
        *(int16_t *)((uint8_t *)unit_seat->animations.pointer + raw_index * 2) != -1) {
        goto allocate;
    }

invalid_animation:
    switch (new_state) {
    case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x27: case 0x29:
        return 0; // (new_state - 0x1e) & 0xffffff00 is always 0 in this range; see file header
    default:
        break; // every other new_state falls through and allocates anyway, per the original
    }

allocate:
    {
        int16_t instance = animation_choose_random_permutation(1);
        obj->animation_graph = *(datum_index *)&obj_tag->animation_graph.tag_id;
        obj->animation_index = instance;
        obj->animation_frame = 0;
        allocated = 1;
    }

category_check:
    {
        int32_t old_category = unit_animation_state_from_seat_type(old_state);
        int32_t new_category = unit_animation_state_from_seat_type(new_state); // UNSURE: extraout_DL gate dropped, see file header

        if (old_category == new_category) {
            if (!allocated) {
                goto done;
            }
        } else {
            unit->animation_instance = animation_choose_random_permutation(1);
            if (old_state == -1) {
                unit->unknown_29c = animation_choose_random_permutation(1);
            }
        }
        object_copy_default_node_transforms();
    }

done:
    unit->animation_state = (int8_t)new_state;
    return 1;
}

#if 0
Original Ghidra decompilation (0x565f90):

uint unit_try_set_animation_state(uint param_1,short param_2)

{
  char cVar1;
  uint *puVar2;
  int iVar3;
  bool bVar4;
  undefined2 uVar5;
  short sVar6;
  int iVar7;
  undefined4 uVar8;
  undefined3 uVar9;
  undefined3 extraout_var;
  char extraout_DL;
  int iVar10;
  int iVar11;
  int iVar12;
  int iVar13;

  iVar11 = (param_1 & 0xffff) * 0xc;
  puVar2 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
  iVar3 = *(int *)((*puVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar12 = (char)puVar2[0xa8] * 100;
  iVar10 = *(int *)(*(int *)((*(uint *)(iVar3 + 0x44) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10
                   );
  iVar13 = iVar12 + iVar10;
  iVar10 = *(char *)((int)puVar2 + 0x2a1) * 0xbc + *(int *)(iVar12 + 0x5c + iVar10);
  cVar1 = *(char *)((int)puVar2 + 0x2a3);
  bVar4 = false;
  if ((cVar1 != -1) && (param_2 == cVar1)) goto LAB_00566284;
  if (cVar1 == '!') {
    unit_release_thrown_grenade(param_1,1);
  }
  iVar12 = (int)param_2;
  switch(iVar12) {
  case 0:
    sVar6 = 0;
    break;
  case 1:
    sVar6 = 1;
    break;
  case 2:
    sVar6 = 2;
    break;
  case 3:
    sVar6 = 3;
    break;
  case 4:
    sVar6 = 8;
    break;
  case 5:
    sVar6 = 9;
    break;
  case 6:
    sVar6 = 10;
    break;
  case 7:
    sVar6 = 0xb;
    break;
  case 8:
    sVar6 = 0x23;
    break;
  case 9:
    sVar6 = 0x24;
    break;
  case 10:
    sVar6 = 0x25;
    break;
  case 0xb:
    sVar6 = 0x26;
    break;
  case 0xc:
    sVar6 = 0xc;
    break;
  case 0xd:
    sVar6 = 0xd;
    break;
  case 0xe:
    sVar6 = 0xe;
    break;
  case 0xf:
    sVar6 = 0xf;
    break;
  case 0x10:
    sVar6 = 0x17;
    goto LAB_00566193;
  case 0x11:
    sVar6 = 0x18;
    goto LAB_00566193;
  case 0x12:
    sVar6 = 0x19;
    goto LAB_00566193;
  case 0x13:
    sVar6 = 0x1a;
    goto LAB_00566193;
  case 0x14:
    sVar6 = 0x10;
    break;
  case 0x15:
    sVar6 = 0x11;
    break;
  case 0x16:
    sVar6 = 0x12;
    break;
  default:
    goto switchD_00566049_caseD_17;
  case 0x18:
    sVar6 = 0;
    goto LAB_00566193;
  case 0x19:
    sVar6 = 1;
    goto LAB_00566193;
  case 0x1e:
    sVar6 = 0x27;
    break;
  case 0x1f:
    sVar6 = 0x2a;
    break;
  case 0x20:
    sVar6 = 0x2e;
    break;
  case 0x21:
    sVar6 = 0x14;
    break;
  case 0x22:
    sVar6 = 0x2c;
    break;
  case 0x23:
    sVar6 = 0x2d;
    break;
  case 0x24:
    sVar6 = 0x2f;
    break;
  case 0x25:
    sVar6 = 0x1b;
    goto LAB_00566193;
  case 0x26:
    sVar6 = 0x1c;
    goto LAB_00566193;
  case 0x27:
    sVar6 = 0x30;
    break;
  case 0x28:
    sVar6 = 0x31;
    break;
  case 0x29:
    sVar6 = 0x32;
    break;
  case 0x2b:
    sVar6 = 0x1d;
LAB_00566193:
    iVar7 = (int)sVar6;
    if (*(int *)(iVar13 + 0x40) <= iVar7) goto switchD_00566049_caseD_17;
    iVar10 = *(int *)(iVar13 + 0x44);
    goto LAB_00566069;
  }
  iVar7 = (int)sVar6;
  if (iVar7 < *(int *)(iVar10 + 0x98)) {
    iVar10 = *(int *)(iVar10 + 0x9c);
LAB_00566069:
    if (*(short *)(iVar10 + iVar7 * 2) == -1) goto LAB_00566077;
  }
  else {
switchD_00566049_caseD_17:
LAB_00566077:
    switch(iVar12) {
    case 0x1e:
    case 0x1f:
    case 0x20:
    case 0x21:
    case 0x27:
    case 0x29:
      return iVar12 - 0x1eU & 0xffffff00;
    }
  }
  uVar5 = FUN_004d6280(1);
  iVar10 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + iVar11);
  *(undefined4 *)(iVar10 + 0xcc) = *(undefined4 *)(iVar3 + 0x44);
  *(undefined2 *)(iVar10 + 0xd0) = uVar5;
  *(undefined2 *)(iVar10 + 0xd2) = 0;
  bVar4 = true;
LAB_00566284:
  sVar6 = FUN_00565da0();
  if ((extraout_DL == '\0') && (uVar8 = FUN_00565da0(), sVar6 == (short)uVar8)) {
    uVar9 = (undefined3)((uint)uVar8 >> 8);
    if (!bVar4) goto LAB_0056632a;
  }
  else {
    uVar5 = FUN_004d6280(1);
    *(undefined2 *)((int)puVar2 + 0x29a) = uVar5;
    if (cVar1 == -1) {
      uVar5 = FUN_004d6280(1);
      *(undefined2 *)(puVar2 + 0xa7) = uVar5;
    }
  }
  FUN_004f6b70();
  uVar9 = extraout_var;
LAB_0056632a:
  *(undefined1 *)((int)puVar2 + 0x2a3) = (undefined1)param_2;
  return CONCAT31(uVar9,1);
}
#endif
