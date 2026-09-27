// ai_communication_select_speaker_by_team  (Ghidra: ai_communication_select_speaker_by_team; named for this rewrite)
// address 0x4300d0, size 522 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// VERIFIED 2026-09-27 (static loop) against objdump 0x4300d0..0x4302d9: iterator init, team filter (+0xa4 bits via the
// 0x6b0b84 pointer, game engine inequality), match modes, the 10-argument push order, the shared position slot.
// evidence: phase-4 summary ("iterates all actors matching a team filter, scoring each as a
// candidate speaker/target and returning the best match"). Same scorer
// (ai_communication_rate_speaker, 0x42fb90) and same hostility bitmap
// (0x006b0b84 + 0xa4, the 10x10 team-relationship bit table this module reads in
// ai_recompute_all_relationship_flags.c and ai_propagate_communication_reaction.c) as its
// siblings.
// register convention: DI -> team (unaff_DI, unresolved in Ghidra's own decompile); the ten
// parameters are genuine stack arguments. A team of -1 disables the filter entirely.
// blam-cc: DI -> team, stack -> match_mode, object_a, object_b, radius, allow_unreachable,
//          fade_limit, line_class, line_id, seat_filter, flags
//
// TRANSCRIBED BUG, deliberately preserved: the second marker lookup passes `object_a`
// (param_2), not `object_b` (param_3), as the object whose marker to resolve -- the
// disassembly at 0x430140 is `push ebp` with EBP still holding param_2, loaded once at
// 0x4300d7. The result is that when object_b is present its marker position silently
// overwrites object_a's with the same value. The single shared position block is then
// handed to ai_communication_rate_speaker as BOTH its EAX position_a and its param_3
// position_b (`lea edx,[esp+0x30]` and `lea eax,[esp+0x3c]` at 0x430284/0x43028b resolve to
// the same address), so the scorer's two range tests measure against the same point.
//
// UNSURE: when ai_globals.actors_valid is clear the iterator block is never initialized and
// the original still calls actor_iterator_next on it. Reproduced as-is, matching the same
// pattern already preserved in ai_propagate_communication_reaction.c.
// UNSURE: match_mode is named from behaviour only -- 0 means "same team", 1 means "not
// hostile", anything else means "hostile" (or, in multiplayer, "different team").
// reconciled: R04 0x006f1d20 int32_t use_absolute_team_check -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>

extern data_array *encounter_data; // 0x008802c8
extern ai_globals *ai_globals_ptr; // 0x00880354
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t *team_relationship_flags; // 0x006b0b84, a POINTER to the team-relationship block (+0x94 / +0xa4 10x10 bit matrices); the draft used the pointer's own address as the block
extern char ai_marker_name_a[];    // 0x0066bfa0

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                               object_marker *marker, uint32_t flags); // 0x4f6080
extern float ai_communication_rate_speaker(datum_index actor_index, datum_index object_b,
                                           real_point3d *position_b, float radius,
                                           int16_t allow_unreachable, uint32_t fade_limit,
                                           uint32_t line_class, uint32_t line_id,
                                           int16_t seat_filter, uint8_t flags,
                                           real_point3d *position_a, datum_index object_a); // 0x42fb90

// blam-cc: DI -> team, stack -> match_mode, object_a, object_b, radius, allow_unreachable,
//          fade_limit, line_class, line_id, seat_filter, flags
// Walks every live actor, keeps the ones whose team satisfies the filter, scores each with
// ai_communication_rate_speaker and returns the highest-scoring actor's handle (none when
// nothing qualifies).
datum_index ai_communication_select_speaker_by_team(int16_t match_mode, datum_index object_a,
                                                    datum_index object_b, float radius,
                                                    int16_t allow_unreachable, uint32_t fade_limit,
                                                    uint32_t line_class, uint32_t line_id,
                                                    int16_t seat_filter, uint8_t flags,
                                                    int16_t team /* DI */)
{
    object_marker marker_a;
    object_marker marker_b;
    real_point3d position;
    actor_iterator_state iterator;
    actor *a;
    datum_index best;
    float best_score;
    datum_index actor_index;
    int16_t other_team;
    uint8_t accept;
    int32_t pair;
    float score;

    best = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    if (object_a != (datum_index)k_datum_index_none) {
        object_get_node_local_transform(object_a, ai_marker_name_a, &marker_a, 1);
        position = marker_a.node_transform.position;
    }
    if (object_b != (datum_index)k_datum_index_none) {
        // See the TRANSCRIBED BUG note: object_a, not object_b.
        object_get_node_local_transform(object_a, ai_marker_name_a, &marker_b, 1);
        position = marker_b.node_transform.position;
    }

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.unknown_04 = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
        iterator.unknown_10 = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.unknown_18 = -1;
    }

    a = actor_iterator_next(&iterator);
    if (a == 0) {
        return (datum_index)k_datum_index_none;
    }
    do {
        actor_index = (datum_index)iterator.actor_index;
        accept = 1;
        if (team != -1) {
            other_team = a->team;
            if (current_game_engine != 0) {
                accept = (uint8_t)(team != other_team);
            } else if (team >= 0 && team < 10 && other_team >= 0 && other_team < 10) {
                pair = (int32_t)other_team + (int32_t)team * 10;
                accept = (uint8_t)(((*(uint32_t *)(team_relationship_flags + 0xa4 + (pair >> 5) * 4)) &
                                    (1u << ((uint8_t)pair & 0x1f))) == 0);
            }
            if (match_mode == 0) {
                accept = (uint8_t)(other_team == team);
            } else if (match_mode == 1) {
                accept = (uint8_t)(accept == 0);
            }
        }
        if (accept) {
            score = ai_communication_rate_speaker(actor_index, object_b, &position, radius,
                                                  allow_unreachable, fade_limit, line_class,
                                                  line_id, seat_filter, flags, &position,
                                                  object_a);
            if (best_score < score) {
                best_score = score;
                best = actor_index;
            }
        }
        a = actor_iterator_next(&iterator);
    } while (a != 0);

    return best;
}

#if 0
Original Ghidra decompilation (0x4300d0):

undefined4
FUN_004300d0(short param_1,int param_2,int param_3,undefined4 param_4,undefined4 param_5,
            undefined4 param_6,undefined4 param_7,undefined4 param_8,undefined4 param_9,
            undefined4 param_10)

{
  short sVar1;
  undefined4 uVar2;
  int iVar3;
  short unaff_DI;
  char cVar4;
  float10 fVar5;
  float local_108;
  undefined4 local_104;
  undefined4 local_100;
  undefined4 local_fc;
  undefined4 local_f8;
  uint local_f4;
  undefined2 local_f0;
  undefined4 local_ec;
  uint local_e8;
  undefined1 local_e4;
  undefined1 local_e3;
  undefined4 local_e0;
  undefined4 local_dc;
  undefined1 local_d8 [96];
  undefined4 local_78;
  undefined4 local_74;
  undefined4 local_70;
  undefined1 local_6c [96];
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  local_104 = 0xffffffff;
  local_108 = 0.0;
  if (param_2 != -1) {
    object_get_node_local_transform(param_2,&DAT_0066bfa0,local_d8,1);
    local_100 = local_78;
    local_fc = local_74;
    local_f8 = local_70;
  }
  if (param_3 != -1) {
    object_get_node_local_transform(param_2,&DAT_0066bfa0,local_6c,1);
    local_100 = local_c;
    local_fc = local_8;
    local_f8 = local_4;
  }
  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_f4 = DAT_008802c8;
    local_e8 = DAT_008802c8 ^ 0x69746572;
    local_f0 = 0;
    local_ec = 0xffffffff;
    local_e4 = 0;
    local_dc = 0xffffffff;
    local_e0 = 0xffffffff;
    local_e3 = 1;
  }
  iVar3 = actor_iterator_next();
  if (iVar3 == 0) {
    return 0xffffffff;
  }
  do {
    uVar2 = local_e0;
    if (unaff_DI == -1) {
LAB_00430248:
      fVar5 = (float10)FUN_0042fb90(local_e0,param_3,&local_100,param_4,param_5,param_6,param_7,
                                    param_8,param_9,param_10);
      if ((float10)local_108 < fVar5) {
        local_108 = (float)fVar5;
        local_104 = uVar2;
      }
    }
    else {
      sVar1 = *(short *)(iVar3 + 0x3e);
      cVar4 = '\x01';
      if (DAT_006f1d20 == 0) {
        if ((((-1 < unaff_DI) && (unaff_DI < 10)) && (-1 < sVar1)) && (sVar1 < 10)) {
          iVar3 = (int)sVar1 + unaff_DI * 10;
          cVar4 = '\x01' - ((1 << ((byte)iVar3 & 0x1f) &
                            *(uint *)(DAT_006b0b84 + 0xa4 + (iVar3 >> 5) * 4)) != 0);
        }
      }
      else {
        cVar4 = unaff_DI != sVar1;
      }
      if (param_1 == 0) {
        cVar4 = sVar1 == unaff_DI;
      }
      else if (param_1 == 1) {
        cVar4 = cVar4 == '\0';
      }
      if (cVar4 != '\0') goto LAB_00430248;
    }
    iVar3 = actor_iterator_next();
    if (iVar3 == 0) {
      return local_104;
    }
  } while( true );
}
#endif
