// unit_try_set_animation_state  (Ghidra: unit_try_set_animation_state, already named)
// address 0x565f90, size 942 bytes
// name confidence: 0.5 (already carries this name)   rewrite confidence: 0.85
// REWRITTEN (objdump 0x565f90..0x56633d, jump tables 0x566340 and 0x5663f0/0x5663f8; the draft called
//   animation_choose_random_permutation without the graph or the animation). Stack: unit, new state.
//   The unit's animation graph is its tag's +0x44; the unit block is graph units (+0x10) [unit +0x2a0] (0x64 each,
//   animations +0x40 count / +0x44 int16 indices) and the weapon block that unit block's weapons (+0x5c)
//   [unit +0x2a1] (0xbc each, animations +0x98 / +0x9c).
//   Unless the current state (+0x2a3, -1 = none) already equals the request, a grenade throw in progress (0x21)
//   releases its grenade (stack: unit, 1) and the new state's animation is looked up (state_animations below: the
//   weapon or unit block and the slot). A missing animation fails (returns 0 with nothing changed) for states
//   0x1e..0x21, 0x27 and 0x29 and otherwise plays -1. The base animation (+0xcc graph, +0xd0 animation from
//   animation_choose_random_permutation(EAX graph, DX animation, stack 1), +0xd2 frame 0) is set and the node
//   interpolation count chosen: 1 between the idle/move states 0, 2, 3; 2 into 0x15/0x16; else 6.
//   The seat type of the new state (unit_animation_state_from_seat_type, CX) picks the weapon block's overlay
//   (+0x29a) when it differs from the current state's or there was no state; with no previous state the unit
//   block's slot 9 also goes to +0x29c and the count is 6. The default node transforms are copied (EAX unit,
//   DX count) when anything changed, the state byte is stored and 1 is returned.
// blam-cc: stack -> unit_index, new_state

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern void unit_release_thrown_grenade(uint32_t object_index, uint8_t apply_throw_fraction); // 0x56e440
extern int16_t animation_choose_random_permutation(datum_index animation_graph_tag, int16_t first_animation,
    int32_t stream); // 0x4d6280, blam-cc: EAX, DX, stack
extern int32_t unit_animation_state_from_seat_type(int16_t animation_state); // 0x565da0, blam-cc: CX
extern void object_copy_default_node_transforms(uint32_t object_index, int16_t requested_count); // 0x4f6b70, EAX, DX

// 0x566340: per new state, the block (1 weapon, 2 unit, 0 none) and slot the animation comes from.
static const uint8_t state_animations[0x2c][2] = {
    { 1, 0x00 }, { 1, 0x01 }, { 1, 0x02 }, { 1, 0x03 }, { 1, 0x08 }, { 1, 0x09 }, { 1, 0x0a }, { 1, 0x0b },
    { 1, 0x23 }, { 1, 0x24 }, { 1, 0x25 }, { 1, 0x26 }, { 1, 0x0c }, { 1, 0x0d }, { 1, 0x0e }, { 1, 0x0f },
    { 2, 0x17 }, { 2, 0x18 }, { 2, 0x19 }, { 2, 0x1a }, { 1, 0x10 }, { 1, 0x11 }, { 1, 0x12 }, { 0, 0 },
    { 2, 0x00 }, { 2, 0x01 }, { 0, 0 },    { 0, 0 },    { 0, 0 },    { 0, 0 },    { 1, 0x27 }, { 1, 0x2a },
    { 1, 0x2e }, { 1, 0x14 }, { 1, 0x2c }, { 1, 0x2d }, { 1, 0x2f }, { 2, 0x1b }, { 2, 0x1c }, { 1, 0x30 },
    { 1, 0x31 }, { 1, 0x32 }, { 0, 0 },    { 2, 0x1d },
};

static int16_t block_animation(const uint8_t *block, int32_t count_offset, int32_t slot)
{
    if (slot < *(int32_t *)(block + count_offset)) {
        return (*(int16_t **)(block + count_offset + 4))[slot];
    }
    return -1;
}

uint8_t unit_try_set_animation_state(uint32_t unit_index, int16_t new_state)
{
    uint8_t *unit = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);
    uint8_t *unit_tag = (uint8_t *)tag_instances[*(datum_index *)unit & 0xffff].data;
    datum_index graph = *(datum_index *)(unit_tag + 0x44);
    uint8_t *graph_data = (uint8_t *)tag_instances[graph & 0xffff].data;
    uint8_t *unit_block = *(uint8_t **)(graph_data + 0x10) + (int8_t)unit[0x2a0] * 0x64;
    uint8_t *weapon_block = *(uint8_t **)(unit_block + 0x5c) + (int8_t)unit[0x2a1] * 0xbc;
    uint8_t no_state = (uint8_t)(unit[0x2a3] == 0xff);
    uint8_t changed = 0;
    int16_t current_state = 0;
    int16_t transform_count = 0;
    int16_t seat_type;
    int16_t count;

    if (no_state || (current_state = (int8_t)unit[0x2a3]) != new_state) {
        int16_t animation = -1;

        if (unit[0x2a3] == 0x21) {
            unit_release_thrown_grenade(unit_index, 1);
        }
        if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 1) {
            animation = block_animation(weapon_block, 0x98, state_animations[new_state][1]);
        } else if ((uint16_t)new_state < 0x2c && state_animations[new_state][0] == 2) {
            animation = block_animation(unit_block, 0x40, state_animations[new_state][1]);
        }
        if (animation == -1) {
            switch (new_state) {
            case 0x1e: case 0x1f: case 0x20: case 0x21: case 0x27: case 0x29:
                return 0;
            default:
                break;
            }
        }
        animation = animation_choose_random_permutation(graph, animation, 1);
        {
            uint8_t *reloaded = *(uint8_t **)((uint8_t *)object_data->data + (unit_index & 0xffff) * 0xc + 8);

            *(datum_index *)(reloaded + 0xcc) = graph;
            *(int16_t *)(reloaded + 0xd0) = animation;
            *(int16_t *)(reloaded + 0xd2) = 0;
        }
        current_state = (int8_t)unit[0x2a3];
        transform_count = 6;
        if ((new_state == 0 || new_state == 2 || new_state == 3) &&
            (current_state == 0 || current_state == 2 || current_state == 3)) {
            transform_count = 1;
        }
        if (new_state == 0x16 || new_state == 0x15) {
            transform_count = 2;
        }
        changed = 1;
    }

    seat_type = (int16_t)unit_animation_state_from_seat_type(new_state);
    if (no_state || seat_type != (int16_t)unit_animation_state_from_seat_type(current_state)) {
        int16_t overlay = -1;

        if (seat_type >= 0 && seat_type < *(int32_t *)(weapon_block + 0x98)) {
            overlay = (*(int16_t **)(weapon_block + 0x9c))[seat_type];
        }
        *(int16_t *)(unit + 0x29a) = animation_choose_random_permutation(graph, overlay, 1);
        count = 6;
        if (no_state) {
            int16_t idle = (*(int32_t *)(unit_block + 0x40) > 9) ? (*(int16_t **)(unit_block + 0x44))[9] : -1;

            *(int16_t *)(unit + 0x29c) = animation_choose_random_permutation(graph, idle, 1);
        }
        object_copy_default_node_transforms(unit_index, count);
    } else if (changed) {
        object_copy_default_node_transforms(unit_index, transform_count);
    }
    unit[0x2a3] = (uint8_t)new_state;
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
