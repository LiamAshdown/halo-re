// game_engine_resolve_netgame_flag_role  (Ghidra: FUN_00462df0; renamed per its summary)
// address 0x462df0, size 475 bytes
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: out/phase4/game_functions.md ("Resolves a scenario netgame_flag-style placement
// index into the role/category appropriate for the currently active multiplayer variant type");
// shares its opening weapon-list index scan verbatim with this batch's
// game_engine_resolve_multiplayer_placement (0x462c30); types/game.h game_variant::flags
// (+0x38, aliased 0x006f1cc0), game_variant::starting_equipment (+0x5c, aliased 0x006f1ce4);
// types/tags.h Globals::weapon_list -> GlobalsWeapon (one TagDependency each, tag_id at +0x0c).
// register convention: a weapon tag id in EAX (in_EAX); the return value is the tag id of the
// weapon the variant substitutes for it, or the original handle back out for the early-outs.
//   // blam-cc: EAX -> handle
//
// CORRECTED (phase 4 review). The first pass reproduced Ghidra's structuring of this function --
// twelve outer cases, several of them falling through into the next -- and said so explicitly.
// That structuring is wrong. `objdump -d -M intel --start-address=0x462df0 --stop-address=0x462fcb`
// shows the outer dispatch is a real jump table (`jmp DWORD PTR [eax*4+0x462fcc]`, 12 entries for
// starting_equipment 1..12) and EVERY arm ends in `jmp 0x462fb2`, the shared tail. There is no
// fallthrough anywhere, so the first pass' `default: index = 4; break;` inside case 1 leaked into
// case 2 and remapped 4 -> 6, and the same happened for case 2 -> 3 and case 8 -> 9 -> 10.
//
// Six of the arms are themselves two-level jump tables (a byte index table selecting one of a
// handful of pointers). Decoded from `objdump -s -j .text --start-address=0x462fc8
// --stop-address=0x4630b0`, they are:
//   equip 1  (0x462e85): eax = index - 3, valid 0..0xc, bytes at 0x463004 over
//            {0x462ece -> 5, 0x462f4a -> 4}; out of range -> 4.
//              index 3 4 6 7 14 15 -> 5;  index 5 8 9 10 11 12 13 -> 4
//   equip 2  (0x462e9f): same eax, bytes at 0x46301c over
//            {0x462eb5 -> 6, 0x462f0c -> 0}; out of range -> 0.
//              index 3 4 6 7 14 15 -> 6;  index 5 8..13 -> 0
//   equip 8  (0x462f13): eax = index - 1, valid 0..0xd, bytes at 0x46303c over
//            {0x462f2d -> 1, 0x462f02 -> 8, 0x462eb5 -> 6, 0x462fb2 -> unchanged}.
//              index 1 7 14 -> 1;  4 8 -> 8;  6 9 -> 6;  everything else unchanged
//   equip 9  (0x462f34): eax = index - 3, valid 0..0xc, bytes at 0x46305c over
//            {0x462f0c -> 0, 0x462f4a -> 4, 0x462fad -> 7, 0x462fb2 -> unchanged}.
//              index 3 6 15 -> 0;  5 -> 4;  14 -> 7;  everything else unchanged
//   equip 10 (0x462f79): index itself, valid 0..9, bytes at 0x463080 over
//            {0x462eb5 -> 6, 0x462f8c -> 15, 0x462ece -> 5, 0x462f93 -> 14, 0x462fb2 -> unchanged}.
//              index 0 1 9 -> 6;  3 8 -> 15;  4 -> 5;  7 -> 14;  2 5 6 and >9 unchanged
//   equip 12 (0x462f9a): index itself, valid 0..0xf, bytes at 0x46309c over
//            {0x462fad -> 7, 0x462f2d -> 1, 0x462f93 -> 14, 0x462fb2 -> unchanged}.
//              index 0 4 7 9 -> 7;  1 3 8 15 -> 1;  5 6 14 -> 14;  2 10..13 and >15 unchanged
// Every per-index answer the first pass produced inside the arms is confirmed correct against
// those tables; the fallthrough between arms is the whole of the behaviour change.
//
// UNSURE: the meaning of index values 0..15 (they are positions in the globals tag's weapon
// list, so the mapping is data-driven and not recoverable from code), and of
// `unknown_006f1d24`, which gates two of case 11's answers and the whole 0x80-flag early-out.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"

extern Globals *global_globals;         // 0x00746fa0
extern game_variant game_engine_variant; // 0x006f1c88 (flags aliased 0x006f1cc0,
                                          // starting_equipment aliased 0x006f1ce4)
extern uint8_t unknown_006f1d24;        // 0x006f1d24, UNSURE identity

// blam-cc: EAX -> handle
// Finds `handle` in the globals tag's weapon list, remaps that list index according to the
// variant's starting_equipment setting, and returns the tag id of the weapon the remapped index
// names. Returns `handle` unchanged when it is not in the list, when its index is 10 or 11, or
// when the "no duals" style flag (game_variant::flags bit 7) rules the substitution out; returns
// -1 when the remap produces -1.
int32_t game_engine_resolve_netgame_flag_role(uint32_t handle)
{
    int32_t weapon_list_count = (int32_t)global_globals->weapon_list.count;
    TagDependency *weapon_list = (weapon_list_count == 0) ? 0
        : (TagDependency *)global_globals->weapon_list.pointer;
    int32_t index = -1;
    int32_t i;

    for (i = 0; i < weapon_list_count; i++) {
        if (handle == *(uint32_t *)&weapon_list[i].tag_id) {
            index = i;
            break;
        }
    }

    if (unknown_006f1d24 != 0 && (game_engine_variant.flags & 0x80) != 0 &&
        (index == 1 || index == 0x0e)) {
        return -1;
    }
    if (index == 10 || index == 0x0b || index == -1) {
        return (int32_t)handle;
    }

    switch (game_engine_variant.starting_equipment) {
    case 1: /* 0x462e85 */
        switch (index) {
        case 3: case 4: case 6: case 7: case 0x0e: case 0x0f: index = 5; break;
        default:                                   index = 4; break;
        }
        break;

    case 2: /* 0x462e9f */
        switch (index) {
        case 3: case 4: case 6: case 7: case 0x0e: case 0x0f: index = 6; break;
        default:                                   index = 0; break;
        }
        break;

    case 3: /* 0x462ebf */
        if (index < 3 || (5 < index && index != 0x0f)) {
            index = 6;
        } else {
            index = 5;
        }
        break;

    case 4: /* 0x462ed8 */
        if (index != 4 && index != 9) {
            index = 9;
        }
        break;

    case 5: /* 0x462ef4 */
        if (index == 4) {
            index = 0;
        } else if (index == 9) {
            index = 8;
        }
        break;

    case 6: /* 0x462fad */
        index = 7;
        break;

    case 7: /* 0x462f02 */
        index = 8;
        break;

    case 8: /* 0x462f13 */
        switch (index) {
        case 1: case 7: case 0x0e: index = 1; break;
        case 4: case 8:            index = 8; break;
        case 6: case 9:            index = 6; break;
        default:                   break;
        }
        break;

    case 9: /* 0x462f34 */
        switch (index) {
        case 3: case 6: case 0x0f: index = 0; break;
        case 5:                    index = 4; break;
        case 0x0e:                 index = 7; break;
        default:                   break;
        }
        break;

    case 10: /* 0x462f79 */
        switch (index) {
        case 0: case 1: case 9: index = 6; break;
        case 3: case 8:         index = 0x0f; break;
        case 4:                 index = 5; break;
        case 7:                 index = 0x0e; break;
        default:                break;
        }
        break;

    case 0x0b: /* 0x462f51 */
        if (index == 1) {
            // sbb/and/add: dl != 0 -> -1, dl == 0 -> 8
            index = (unknown_006f1d24 != 0) ? -1 : 8;
        } else if (index == 0x0e) {
            // sete/lea: dl == 0 -> 7, dl != 0 -> -1
            index = (unknown_006f1d24 == 0) ? 7 : -1;
        }
        break;

    case 0x0c: /* 0x462f9a */
        switch (index) {
        case 0: case 4: case 7: case 9:            index = 7; break;
        case 1: case 3: case 8: case 0x0f:         index = 1; break;
        case 5: case 6: case 0x0e:                 index = 0x0e; break;
        default:                                   break;
        }
        break;

    default: /* starting_equipment - 1 > 0xb: `ja 0x462fb2`, index untouched */
        break;
    }

    if (index == -1) {
        return -1;
    }
    return *(int32_t *)((uint8_t *)global_globals->weapon_list.pointer + 0xc + index * 0x10);
}

#if 0
Original Ghidra decompilation (0x462df0), from tools/pack.py 0x462df0:

int FUN_00462df0(void)

{
  int in_EAX;
  int iVar1;
  int iVar2;
  int iVar3;
  int iVar4;
  int *piVar5;

  iVar1 = *(int *)(DAT_00746fa0 + 0x14c);
  if (iVar1 == 0) {
    iVar4 = 0;
  }
  else {
    iVar4 = *(int *)(DAT_00746fa0 + 0x150);
  }
  iVar2 = 0;
  iVar3 = -1;
  if (0 < iVar1) {
    piVar5 = (int *)(iVar4 + 0xc);
    do {
      iVar3 = iVar2;
      if (in_EAX == *piVar5) break;
      iVar2 = iVar2 + 1;
      piVar5 = piVar5 + 4;
      iVar3 = -1;
    } while (iVar2 < iVar1);
  }
  if (((DAT_006f1d24 != '\0') && (((byte)DAT_006f1cc0 & 0x80) != 0)) &&
     ((iVar3 == 1 || (iVar3 == 0xe)))) {
    return -1;
  }
  if (iVar3 == 10) {
    return in_EAX;
  }
  if (iVar3 == 0xb) {
    return in_EAX;
  }
  if (iVar3 == -1) {
    return in_EAX;
  }
  switch(DAT_006f1ce4) {
  case 1:
    switch(iVar3) {
    case 3:
    case 4:
    case 6:
    case 7:
    case 0xe:
    case 0xf:
      goto switchD_00462f85_caseD_4;
    default:
switchD_00462f43_caseD_5:
      iVar3 = 4;
      break;
    }
  case 2:
    switch(iVar3) {
    case 3:
    case 4:
    case 6:
    case 7:
    case 0xe:
    case 0xf:
      goto switchD_00462f85_caseD_0;
    default:
switchD_00462f43_caseD_3:
      iVar3 = 0;
      break;
    }
  case 3:
    if ((iVar3 < 3) || ((5 < iVar3 && (iVar3 != 0xf)))) goto switchD_00462f85_caseD_0;
switchD_00462f85_caseD_4:
    iVar3 = 5;
    break;
  case 4:
    if ((iVar3 != 4) && (iVar3 != 9)) {
      iVar3 = 9;
    }
    break;
  case 5:
    if (iVar3 == 4) goto switchD_00462f43_caseD_3;
    if (iVar3 != 9) break;
  case 7:
switchD_00462e7e_caseD_7:
    iVar3 = 8;
    break;
  case 6:
switchD_00462e7e_caseD_6:
    iVar3 = 7;
    break;
  case 8:
    switch(iVar3) {
    case 1:
    case 7:
    case 0xe:
switchD_00462fa6_caseD_1:
      iVar3 = 1;
    default:
      break;
    case 4:
    case 8:
      goto switchD_00462e7e_caseD_7;
    case 6:
    case 9:
      goto switchD_00462f85_caseD_0;
    }
  case 9:
    switch(iVar3) {
    case 3:
    case 6:
    case 0xf:
      goto switchD_00462f43_caseD_3;
    default:
      break;
    case 5:
      goto switchD_00462f43_caseD_5;
    case 0xe:
      goto switchD_00462e7e_caseD_6;
    }
  case 10:
    switch(iVar3) {
    case 0:
    case 1:
    case 9:
      goto switchD_00462f85_caseD_0;
    case 3:
    case 8:
      iVar3 = 0xf;
      break;
    case 4:
      goto switchD_00462f85_caseD_4;
    case 7:
switchD_00462fa6_caseD_5:
      iVar3 = 0xe;
    }
    break;
  case 0xb:
    if (iVar3 == 1) {
      iVar3 = (-(uint)(DAT_006f1d24 != '\0') & 0xfffffff7) + 8;
    }
    else if (iVar3 == 0xe) {
      iVar3 = (uint)(DAT_006f1d24 == '\0') * 8 + -1;
    }
    break;
  case 0xc:
    switch(iVar3) {
    case 0:
    case 4:
    case 7:
    case 9:
      goto switchD_00462e7e_caseD_6;
    case 1:
    case 3:
    case 8:
    case 0xf:
      goto switchD_00462fa6_caseD_1;
    default:
      break;
    case 5:
    case 6:
    case 0xe:
      goto switchD_00462fa6_caseD_5;
    }
  }
switchD_00462e7e_default:
  iVar1 = -1;
  if (iVar3 != -1) {
    iVar1 = *(int *)(*(int *)(DAT_00746fa0 + 0x150) + 0xc + iVar3 * 0x10);
  }
  return iVar1;
switchD_00462f85_caseD_0:
  iVar3 = 6;
  goto switchD_00462e7e_default;
}
#endif
