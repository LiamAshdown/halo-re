// game_engine_apply_player_grenade_counts  (Ghidra: game_engine_apply_player_grenade_counts,
// already named)
// address 0x4613c0, size 366 bytes
// name confidence: 0.5   rewrite confidence: 0.25
// evidence: out/phase4/game_functions.md ("Computes and applies a player's starting frag and
// plasma grenade counts based on scenario defaults and game option overrides"); types/tags.h
// Globals::grenades (TagReflexive at +0x128, pointer field at +0x12c matching this function's
// own DAT_00746fa0+300 read exactly) and GlobalsGrenade (0x44 bytes: maximum_count, then
// mp_spawn_default -- both int16 -- so grenades[1] starts at short-index 0x22, matching the
// psVar1[0x22]/[0x23] reads here for the second (plasma) grenade type); types/game.h
// game_variant::flags (+0x38), game_variant::starting_equipment (+0x5c, "clamped to 0..0xd"),
// types/objects.h object::network_role (+0x04); this batch's
// game_engine_spawn_player_starting_loadout (0x4611f0) and FUN_00462bd0.
// register convention: a player index in EAX (in_EAX), used only for the one player->unit read;
// everything after that operates on the resulting unit handle.
//   // blam-cc: EAX -> player_index
// VERIFIED against disassembly 0x4613c0..0x46152d (2026-09-30). Fixed: the engine's +0x78 callback takes the player
// index (push esi) and case 0xd of the weapon-set switch only tests AL of 0x462bd0; it does not read a second
// result out of EDX (0x462bd0 never touches EDX), it just zeroes both counts when AL == 0.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern Globals *global_globals;                     // 0x00746fa0
extern int32_t game_engine_unknown_aa00;            // 0x0087aa00
extern game_variant game_engine_variant;            // 0x006f1c88 (flags aliased 0x006f1cc0,
                                                     // starting_equipment aliased 0x006f1ce4)
extern data_array *object_data;                  // 0x008603b0

extern void game_engine_spawn_player_starting_loadout(uint32_t starting_equipment_index,
    int32_t *frag_count, int32_t *plasma_count); // 0x4611f0, this batch
extern uint32_t game_engine_pack_object_flags_or_passthrough(uint32_t input); // 0x462bd0, this batch

// blam-cc: EAX -> player_index
void game_engine_apply_player_grenade_counts(uint32_t player_index)
{
    GlobalsGrenade *grenades;
    int32_t frag_max, plasma_max, frag_count, plasma_count;
    player *p;
    datum_index unit;

    if (current_game_engine == 0) {
        return;
    }
    if (current_game_engine->allow_grenade_counts != 0 &&
        ((char (*)(uint32_t))current_game_engine->allow_grenade_counts)(player_index) == 0) { // 0x4613dd: push esi (the player index, unmasked)
        return;
    }

    grenades = (GlobalsGrenade *)global_globals->grenades.pointer;
    plasma_max = grenades[1].maximum_count;
    frag_count = grenades[0].mp_spawn_default;
    plasma_count = grenades[1].mp_spawn_default;
    frag_max = grenades[0].maximum_count;

    p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    unit = p->unit;

    if ((game_engine_unknown_aa00 & 8) == 0) {
        if ((game_engine_unknown_aa00 & 4) != 0) {
            frag_max = 2;
            plasma_max = 2;
        }
    } else {
        frag_max = 1;
        plasma_max = 1;
    }

    if ((game_engine_variant.flags & 0x20) == 0) {
        object *obj = ((object_header *)object_data->data)[unit & 0xffff].data;
        if (obj->network_role == 0 || obj->network_role == 3) {
            game_engine_spawn_player_starting_loadout(unit, &frag_count, &plasma_count);
        }
    }

    {
        int32_t plasma_result = plasma_count;
        int32_t frag_result = frag_count;

        if ((game_engine_unknown_aa00 & 4) == 0 && ((game_engine_variant.flags >> 2) & 1) != 0) {
            plasma_result = plasma_max;
            frag_result = frag_max;
        }

        if (unit == (datum_index)0xffffffff) {
            return;
        }
        {
            object *obj = ((object_header *)object_data->data)[unit & 0xffff].data;

            if (obj->network_role != 0 && obj->network_role != 3) {
                return;
            }

            switch (game_engine_variant.weapon_set) {
            case 3:
            case 10:
                plasma_result = plasma_result + frag_result;
                frag_result = 0;
                break; // falls to the shared clamp section, skipping the plasma_result=0 below
            case 9:
                frag_result = frag_result + plasma_result;
                plasma_result = 0;
                goto clamp;
            case 0x0d:
                // 0x4614f7: call 0x462bd0 (no args, only AL is tested); zero result means both counts are dropped
                if ((uint8_t)game_engine_pack_object_flags_or_passthrough(0) == 0) {
                    frag_result = 0;
                    plasma_result = 0;
                }
                break;
            default:
                break; // skip the plasma_result=0 below
            }
            goto clamp_after_default; // cases 3/10/0xd(ok)/default all reach the shared section
                                       // without zeroing plasma_result again

        clamp:
            plasma_result = 0;
        clamp_after_default:
            if (frag_max < frag_result) {
                frag_result = frag_max;
            }
            if (plasma_max < plasma_result) {
                plasma_result = plasma_max;
            }
            *(uint8_t *)((uint8_t *)obj + 0x31e) = (uint8_t)frag_result;
            *(uint8_t *)((uint8_t *)obj + 0x31f) = (uint8_t)plasma_result; // UNSURE field names,
                // types/units.h desired_grenade_index-adjacent region not split this finely
        }
    }
}

#if 0
Original Ghidra decompilation (0x4613c0), from tools/pack.py 0x4613c0:

void game_engine_apply_player_grenade_counts(void)

{
  short *psVar1;
  uint uVar2;
  int iVar3;
  char cVar4;
  uint in_EAX;
  int extraout_EDX;
  int iVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int local_8;
  int local_4;

  if (DAT_006f1d20 == 0) {
    return;
  }
  if ((*(code **)(DAT_006f1d20 + 0x78) != (code *)0x0) &&
     (cVar4 = (**(code **)(DAT_006f1d20 + 0x78))(), cVar4 == '\0')) {
    return;
  }
  psVar1 = *(short **)(DAT_00746fa0 + 300);
  iVar7 = (int)psVar1[0x22];
  local_8 = (int)psVar1[1];
  local_4 = (int)psVar1[0x23];
  iVar8 = (int)*psVar1;
  uVar2 = *(uint *)((in_EAX & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  if ((DAT_0087aa00 & 8) == 0) {
    if ((DAT_0087aa00 & 4) != 0) {
      iVar8 = 2;
      iVar7 = 2;
    }
  }
  else {
    iVar8 = 1;
    iVar7 = 1;
  }
  if (((DAT_006f1cc0 & 0x20) == 0) &&
     ((iVar5 = *(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc) + 4),
      iVar5 == 0 || (iVar5 == 3)))) {
    game_engine_spawn_player_starting_loadout(uVar2,&local_8,&local_4);
  }
  iVar5 = local_4;
  iVar6 = local_8;
  if (((DAT_0087aa00 & 4) == 0) && ((DAT_006f1cc0 >> 2 & 1) != 0)) {
    iVar5 = iVar7;
    iVar6 = iVar8;
  }
  if (uVar2 == 0xffffffff) {
    return;
  }
  iVar3 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar2 & 0xffff) * 0xc);
  if ((*(int *)(iVar3 + 4) != 0) && (*(int *)(iVar3 + 4) != 3)) {
    return;
  }
  switch(DAT_006f1ce4) {
  case 3:
  case 10:
    iVar5 = iVar5 + iVar6;
    iVar6 = 0;
  default:
    goto switchD_004614f0_caseD_4;
  case 9:
    iVar6 = iVar6 + iVar5;
    break;
  case 0xd:
    cVar4 = FUN_00462bd0();
    iVar5 = extraout_EDX;
    if (cVar4 != '\0') goto switchD_004614f0_caseD_4;
    iVar6 = 0;
  }
  iVar5 = 0;
switchD_004614f0_caseD_4:
  if (iVar8 < iVar6) {
    iVar6 = iVar8;
  }
  if (iVar7 < iVar5) {
    iVar5 = iVar7;
  }
  *(char *)(iVar3 + 0x31e) = (char)iVar6;
  *(char *)(iVar3 + 799) = (char)iVar5;
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
