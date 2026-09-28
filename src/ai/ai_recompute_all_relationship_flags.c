// ai_recompute_all_relationship_flags  (Ghidra: ai_recompute_all_relationship_flags; named for this rewrite)
// address 0x42bbb0, size 439 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: phase-4 summary ("recomputes the perceived-relationship (friend/foe) flags for
// every recognized object of every actor in the level, likely run after a team or difficulty
// change"). team_pair_data+0xa4 / +0x94 match the two 100-bit team-pair bitmaps
// out/phase4/game_types_notes.md documents at 0x45bc30's allocation (the 0xa4 map is the one
// teams_are_enemies inverts, 0x94 is "a second per-pair flag"); reproduced here as the raw
// bit test rather than calling teams_are_enemies (0x45bd50), since this function inlines
// that logic itself with slightly different surrounding control flow (a not-multiplayer
// gate) that a black-box call could not be proven to replicate exactly.
// register convention: no parameters; reuses the actor_iterator_next iterator-state pattern
// already recovered in ai_notify_actors_of_encounter_state_change.c.
// blam-cc: (no arguments)
//
// 0x006f1d20 is game.h current_game_engine (R04): NULL means no multiplayer engine. UNSURE: the
// 10x10 team-pair bitmap layout at team_pair_data+0x94/+0xa4 are not independently
// confirmed here; see out/phase4/game_types_notes.md's own "Unresolved: entry 0x08, 0x09,
// 0x0c, and what distinguishes the two bitmaps" note. actor_target_update_active_flag's argument is UNSURE, as
// in the other two functions of this shape.
// reconciled: R04 0x006f1d20 int32_t use_absolute_team_check -> game.h game_engine_definition *current_game_engine (all accesses are DWORD; non-NULL = multiplayer engine loaded)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0
extern data_array *object_data;    // 0x008603b0
extern game_engine_definition *current_game_engine; // 0x006f1d20, game.h; non-NULL = multiplayer engine loaded (R04)
extern uint8_t *team_pair_data; // 0x006b0b84, a POINTER to the team-relationship block (+0x94 / +0xa4 10x10 bit matrices); the draft used the pointer's own address as the block

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, EAX, EDI
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50

// blam-cc: (no arguments)
// For every actor in the level, walks its prop list and, for every recognized prop,
// refreshes its cached object team (object_type), recomputes whether that team is hostile to
// the actor's own team (is_unit) and whether the pair is specially marked (unknown_61), then
// restamps its engaged flag and desirability score.
void ai_recompute_all_relationship_flags(void)
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;
    object *tracked_object;
    int16_t object_team;
    int16_t actor_team;
    uint8_t hostile;
    uint8_t marked;

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
    while (a != 0) {
        actor_index = iterator.actor_index /* the full handle, salt included */;
        prop_cursor = a->first_prop;
        while (prop_cursor != (datum_index)k_datum_index_none) {
            current_prop_index = prop_cursor;
            p = &((prop *)prop_data->data)[current_prop_index & 0xffff];
            prop_cursor = p->next_in_actor;

            tracked_object = ((object_header *)object_data->data)[p->object_index & 0xffff].data;
            object_team = *(int16_t *)((uint8_t *)tracked_object + 0xb8);
            p->object_type = object_team;
            actor_team = a->team;

            hostile = 1;
            if (current_game_engine == 0) {
                if (-1 < actor_team && actor_team < 10 && -1 < object_team && object_team < 10) {
                    int32_t pair = (int32_t)object_team + actor_team * 10;
                    uint32_t bit = *(uint32_t *)(team_pair_data + 0xa4 + (pair >> 5) * 4);
                    hostile = 1 - ((bit & (1u << (pair & 0x1f))) != 0);
                }
            } else {
                hostile = (actor_team != object_team);
            }
            p->is_unit = hostile;

            marked = 0;
            if (-1 < actor_team && actor_team < 10 && -1 < object_team && object_team < 10) {
                int32_t pair = (int32_t)object_team + actor_team * 10;
                uint32_t bit = *(uint32_t *)(team_pair_data + 0x94 + (pair >> 5) * 4);
                marked = (bit & (1u << (pair & 0x1f))) != 0;
            }
            p->unknown_61 = marked;

            p->engaged = actor_target_update_active_flag(actor_index, current_prop_index); // FIXED: EDI = the prop (0x42bc33)
            p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
        }
        a = actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x42bbb0):

void FUN_0042bbb0(void)

{
  short sVar1;
  short sVar2;
  int iVar3;
  uint uVar4;
  undefined1 uVar5;
  int iVar6;
  uint uVar7;
  int iVar8;
  int iVar9;
  bool bVar10;
  char cVar11;
  float10 fVar12;
  uint local_8;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_8 = 0xffffffff;
  }
  iVar6 = actor_iterator_next();
  while (iVar6 != 0) {
    uVar7 = *(uint *)((local_8 & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
    while (uVar7 != 0xffffffff) {
      iVar3 = *(int *)(DAT_008802c0 + 0x34);
      iVar8 = (uVar7 & 0xffff) * 0x138;
      uVar4 = *(uint *)(iVar8 + 8 + iVar3);
      iVar9 = iVar8 + iVar3;
      bVar10 = DAT_006f1d20 == 0;
      sVar1 = *(short *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (*(uint *)(iVar8 + 0x18 + iVar3) & 0xffff) * 0xc) + 0xb8);
      *(short *)(iVar9 + 0x12) = sVar1;
      sVar2 = *(short *)(iVar6 + 0x3e);
      cVar11 = '\x01';
      if (bVar10) {
        if ((((-1 < sVar2) && (sVar2 < 10)) && (-1 < sVar1)) && (sVar1 < 10)) {
          iVar3 = (int)sVar1 + sVar2 * 10;
          cVar11 = '\x01' - ((1 << ((byte)iVar3 & 0x1f) &
                             *(uint *)(DAT_006b0b84 + 0xa4 + (iVar3 >> 5) * 4)) != 0);
        }
      }
      else {
        cVar11 = sVar2 != sVar1;
      }
      sVar1 = *(short *)(iVar9 + 0x12);
      *(char *)(iVar9 + 0x60) = cVar11;
      sVar2 = *(short *)(iVar6 + 0x3e);
      bVar10 = false;
      if (((-1 < sVar2) && (sVar2 < 10)) && ((-1 < sVar1 && (sVar1 < 10)))) {
        iVar3 = (int)sVar1 + sVar2 * 10;
        bVar10 = (*(uint *)(DAT_006b0b84 + 0x94 + (iVar3 >> 5) * 4) & 1 << ((byte)iVar3 & 0x1f)) !=
                 0;
      }
      *(bool *)(iVar9 + 0x61) = bVar10;
      uVar5 = FUN_0041fc60();
      *(undefined1 *)(iVar9 + 0xa4) = uVar5;
      fVar12 = (float10)actor_rate_potential_target(local_8,uVar7);
      *(float *)(iVar9 + 0x50) = (float)fVar12;
      uVar7 = uVar4;
    }
    iVar6 = actor_iterator_next();
  }
  return;
}
#endif
