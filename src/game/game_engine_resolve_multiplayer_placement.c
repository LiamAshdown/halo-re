// game_engine_resolve_multiplayer_placement  (Ghidra: FUN_00462c30; renamed per its summary)
// address 0x462c30, size 405 bytes
// name confidence: 0.3   rewrite confidence: 0.15
// evidence: out/phase4/game_functions.md ("Resolves a scenario multiplayer object placement...
// to the actual object/index that should be used for the active game variant, hiding it (-1)
// when the relevant vehicle set option/probability check fails"); types/tags.h Globals::
// weapon_list (TagReflexive at +0x14c, matching the header's own "weapon_list" field position
// counted from interface_bitmaps at +0x140); types/cache.h tag_instance (stride 0x20, data at
// +0x14); types/game.h game_variant::flags (+0x38, aliased 0x006f1cc0), game_variant::
// starting_equipment (+0x5c, aliased 0x006f1ce4), types/math.h random_seed.
// register convention: a tag handle in EAX (in_EAX), tested against -1 and looked up both in
// tag_instances and in the globals weapon list.
//   // blam-cc: EAX -> handle
// UNSURE: almost every offset below is transcribed raw rather than named -- this function's own
// evidence does not establish what tag field lives at +0x308 for the object this handle names
// (read here as a 16-bit value, compared to 2/3, which does not match Weapon::weapon_flags being
// a 32-bit bitfield), nor what the two magic weapon-list indices 0xc/0xd represent, nor the exact
// meaning of the two random-probability branches (0.55 vs 0.3, gated on game_engine_unknown_aa00
// bits 2 and 3).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern tag_instance *tag_instances; // 0x0087bc14
extern Globals *global_globals;     // 0x00746fa0
extern game_variant game_engine_variant; // 0x006f1c88 (flags aliased 0x006f1cc0,
                                          // starting_equipment aliased 0x006f1ce4)
extern int32_t game_engine_unknown_aa00; // 0x0087aa00
extern random_seed random_seed_global;   // 0x00719cd0

// blam-cc: EAX -> handle
uint32_t game_engine_resolve_multiplayer_placement(uint32_t handle)
{
    uint8_t *tag_data;
    int32_t weapon_list_count;
    TagDependency *weapon_list;
    int32_t index;
    int32_t i;

    tag_data = (handle == 0xffffffff) ? 0 : (uint8_t *)tag_instances[handle & 0xffff].data;

    weapon_list_count = (int32_t)global_globals->weapon_list.count;
    weapon_list = (weapon_list_count == 0) ? 0
        : (TagDependency *)global_globals->weapon_list.pointer;

    index = -1;
    for (i = 0; i < weapon_list_count; i++) {
        index = i;
        if (handle == *(uint32_t *)((uint8_t *)weapon_list + i * 0x10 + 0xc)) {
            break;
        }
        index = -1;
    }

    if (index != 0x0c && index != 0x0d) {
        if (tag_data == 0) {
            return handle;
        }
        if (*(int16_t *)(tag_data + 0x308) == 2) {
            return ((game_engine_variant.flags & 8) == 0) ? handle : 0xffffffff;
        }
        if (*(int16_t *)(tag_data + 0x308) != 3) {
            return handle;
        }
        return ((game_engine_variant.flags & 0x10) == 0) ? handle : 0xffffffff;
    }

    switch (game_engine_variant.weapon_set) {
    case 3:
    case 10:
        index = 0x0d;
        break;
    case 9:
        index = 0x0c;
        break;
    case 0x0d:
        index = -1;
        break;
    default:
        break;
    }

    if ((game_engine_unknown_aa00 & 4) == 0 && ((game_engine_variant.flags >> 2) & 1) != 0) {
        index = -1;
    }

    if ((game_engine_unknown_aa00 & 8) == 0) {
        if ((game_engine_unknown_aa00 & 4) != 0) {
            float roll;
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
            roll = (float)(random_seed_global >> 0x10) * 1.5259022e-05f;
            if (!(roll < 0.55f) && roll != 0.55f) {
                index = -1;
            }
        }
    } else {
        float roll;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        roll = (float)(random_seed_global >> 0x10) * 1.5259022e-05f;
        if (!(roll < 0.3f) && roll != 0.3f) {
            index = -1;
        }
    }

    if (index == -1) {
        return 0xffffffff;
    }
    return *(uint32_t *)((uint8_t *)global_globals->weapon_list.pointer + 0xc + index * 0x10);
}

#if 0
Original Ghidra decompilation (0x462c30), from tools/pack.py 0x462c30:

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

uint FUN_00462c30(void)

{
  int iVar1;
  float fVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  int iVar7;
  uint *puVar8;
  ushort uVar9;

  if (in_EAX == 0xffffffff) {
    iVar6 = 0;
  }
  else {
    iVar6 = *(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  iVar1 = *(int *)(DAT_00746fa0 + 0x14c);
  if (iVar1 == 0) {
    iVar7 = 0;
  }
  else {
    iVar7 = *(int *)(DAT_00746fa0 + 0x150);
  }
  iVar5 = 0;
  iVar4 = -1;
  if (0 < iVar1) {
    puVar8 = (uint *)(iVar7 + 0xc);
    do {
      iVar4 = iVar5;
      if (in_EAX == *puVar8) break;
      iVar5 = iVar5 + 1;
      puVar8 = puVar8 + 4;
      iVar4 = -1;
    } while (iVar5 < iVar1);
  }
  if ((iVar4 != 0xc) && (iVar4 != 0xd)) {
    if (iVar6 == 0) {
      return in_EAX;
    }
    if (*(short *)(iVar6 + 0x308) == 2) {
      if ((DAT_006f1cc0 & 8) == 0) {
        return in_EAX;
      }
      return 0xffffffff;
    }
    if (*(short *)(iVar6 + 0x308) != 3) {
      return in_EAX;
    }
    if ((DAT_006f1cc0 & 0x10) == 0) {
      return in_EAX;
    }
    return 0xffffffff;
  }
  switch(DAT_006f1ce4) {
  case 3:
  case 10:
    iVar4 = 0xd;
    break;
  case 9:
    iVar4 = 0xc;
    break;
  case 0xd:
    iVar4 = -1;
  }
  if (((_DAT_0087aa00 & 4) == 0) && ((DAT_006f1cc0 >> 2 & 1) != 0)) {
    iVar4 = -1;
  }
  if ((_DAT_0087aa00 & 8) == 0) {
    if ((_DAT_0087aa00 & 4) == 0) goto LAB_00462dac;
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar2 = (float)(random_seed_global >> 0x10) * 1.5259022e-05;
    uVar9 = (ushort)(fVar2 < 0.55) << 8 | (ushort)(fVar2 == 0.55) << 0xe;
  }
  else {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar2 = (float)(random_seed_global >> 0x10) * 1.5259022e-05;
    uVar9 = (ushort)(fVar2 < 0.3) << 8 | (ushort)(fVar2 == 0.3) << 0xe;
  }
  if (uVar9 == 0) {
    iVar4 = -1;
  }
LAB_00462dac:
  uVar3 = 0xffffffff;
  if (iVar4 != -1) {
    uVar3 = *(uint *)(*(int *)(DAT_00746fa0 + 0x150) + 0xc + iVar4 * 0x10);
  }
  return uVar3;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
