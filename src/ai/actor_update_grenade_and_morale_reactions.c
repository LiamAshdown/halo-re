// actor_update_grenade_and_morale_reactions  (Ghidra: actor_update_grenade_and_morale_reactions, renamed)
// address 0x40b920, size 800 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: types/ai.h actor.unknown_3a8/active_unit_index/unknown_36c/target_unit_index/
//   mode/order_committed(unknown_504)/unknown_374/unknown_378/unknown_1ca; prop.engaged
//   (0xa4)/unknown_38; types/tags.h Actor.attacking_evasion_threshold (0x310)/
//   defending_evasion_threshold (0x314)/evasion_seek_cover_chance (0x318), already-named
//   fields matching puVar4[0xc4]/[0xc5]/[0xc6] exactly; phase-4 summary "periodically checks
//   for nearby grenade threats and expiring morale timers, triggering dodge or flee
//   reactions as needed".
//
// Kept close to the Ghidra decompilation given the size; see UNSURE notes below.
// UNSURE: actor+0x354/0x358/0x368/0x3ac/0x3bb fall inside actor.unknown_350's opaque
// 28-byte run (or just past it); read/written here as raw offsets (a cached evasion
// threshold, a "use defending threshold" flag, a flee-timer, a threatening prop index, and a
// "flee committed" flag). ActorVariant+0x184 is not decoded (compared against 2 as some
// grenade-reaction sub-mode selector).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern uint32_t random_seed_global;  // 0x00719cd0

extern real random_real(void); // 0x4019f0
extern uint8_t actor_should_throw_grenade(uint32_t actor_index, char force); // 0x40b840, this session; returns in AL only
extern uint8_t actor_consider_grenade_throw(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_consider_grenade_throw at 0x40dc30
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_handle_death(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_handle_death at 0x40dd50
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint8_t actor_check_pain_reaction(datum_index actor_index); // 0x40de20, this session (later)
extern uint8_t actor_evaluate_grenade_target_position(); // SIGNATURE-CONFLICT: this call site and the rewrite of actor_evaluate_grenade_target_position at 0x40de70
                 // disagree on the argument list; Ghidra drops the register arguments
                 // here. Left unprototyped so the conflict is visible. See src/ai/README.md.
extern uint32_t actor_get_target_prop_object_index(uint32_t a, uint32_t b, uint32_t c, uint32_t d); // 0x4283d0, not yet rewritten
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data);
// 0x42d340, not yet rewritten (this module). Always seven stack arguments: every call
// site in the binary cleans up 0x1c bytes, so the shorter forms Ghidra recovers at some
// sites are artefacts, not a reduced-arity overload.

char actor_update_grenade_and_morale_reactions(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t *actor_base = (uint8_t *)a;
    Actor *actor_def = (Actor *)tag_instances[a->actor_definition_tag & 0xffff].data;
    ActorVariant *variant = (ActorVariant *)tag_instances[a->actor_variant_tag & 0xffff].data;
    int32_t now = game_time->game_time;
    char result = 0;
    float threshold;

    if (a->unknown_3a8 > 0 && a->active_unit_index == (datum_index)k_datum_index_none) {
        prop *threat = &((prop *)prop_data->data)[*(uint32_t *)(actor_base + 0x3ac) & 0xffff];

        if (threat->engaged != 0 && (threat->unknown_38 == 0 || threat->unknown_38 == 1) &&
            (a->unknown_36c == (datum_index)k_datum_index_none || *(int32_t *)&a->unknown_36c + 0x1e <= now)) {
            *(int32_t *)&a->unknown_36c = now;
            if (actor_should_throw_grenade(actor_index, 1) != 0) {
                if (actor_handle_death() != 0) {
                    return 1;
                }
                if ((((uint8_t *)actor_def)[3] & 0x40) != 0 /* UNSURE: bit 0x400000 of Actor.flags */ &&
                    actor_check_pain_reaction(*(uint32_t *)(actor_base + 0x3ac)) != 0) {
                    return 1;
                }
            }
        }
    }

    if (a->unknown_374 == 0 || a->unknown_378 != 0) {
        threshold = actor_def->attacking_evasion_threshold;
    } else {
        threshold = actor_def->defending_evasion_threshold;
    }
    if (a->unknown_1ca != 0 && actor_def->evasion_seek_cover_chance > 0.0f && threshold > 1.1f) {
        threshold = 1.1f;
    }

    if (threshold < *(float *)(actor_base + 0x354)) {
        char local_5 = 0;

        if (a->unknown_504 == 0) {
            random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        }
        if (*(int16_t *)((uint8_t *)variant + 0x184) == 2 && actor_consider_grenade_throw() != 0) {
            *(uint32_t *)(actor_base + 0x354) = 0;
            local_5 = 1;
        }

        {
            uint8_t allow_a = 1;
            uint8_t allow_b = 1;

            if (actor_base[0x358] != 0 && (((uint8_t *)actor_def)[0] & 0x20) != 0) {
                allow_a = 0;
                if (a->target_unit_index != (datum_index)k_datum_index_none) {
                    prop *target = &((prop *)prop_data->data)[a->target_unit_index & 0xffff];
                    if (target->unknown_122 < 3 && target->unknown_121 < 2) {
                        allow_a = 1;
                    }
                }
            }
            if (a->mode == 10 && (*(int16_t *)(a->mode_data + (0xa0 - 0x9c)) == 2 || *(int16_t *)(a->mode_data + (0xa0 - 0x9c)) == 3)) {
                allow_b = 0;
            }

            if (local_5 == 0) {
                if (allow_a && (a->unknown_36c == (datum_index)k_datum_index_none || *(int32_t *)&a->unknown_36c + 0x1e <= now)) {
                    *(int32_t *)&a->unknown_36c = now;
                    if (actor_should_throw_grenade(actor_index, 0) != 0 &&
                        random_real() < actor_def->evasion_seek_cover_chance &&
                        actor_handle_death() != 0) {
                        uint32_t extra = actor_get_target_prop_object_index(0xffffffff, 0xffffffff, 0xffffffff, 0);
                        // The call site at 0x40bbb0 pushes seven arguments; Ghidra only recovered three.
                        ai_communication_broadcast(0x18, a->unit_index, extra, -1, -1, -1, 0);
                        *(uint32_t *)(actor_base + 0x354) = 0;
                        return 1;
                    }
                }
                if (allow_b && *(int16_t *)(actor_base + 0x368) == 0 && actor_evaluate_grenade_target_position() != 0) {
                    *(uint32_t *)(actor_base + 0x354) = 0;
                    *(int16_t *)(actor_base + 0x368) = (int16_t)now; // UNSURE: __ftol with no visible float operand
                    actor_base[0x3bb] = 1;
                    local_5 = 1;
                }
            }
        }
        result = local_5;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x40b920):

char FUN_0040b920(void)

{
  short sVar1;
  int iVar2;
  int iVar3;
  uint *puVar4;
  bool bVar5;
  bool bVar6;
  char cVar7;
  char extraout_AL;
  undefined2 uVar8;
  uint in_EAX;
  int iVar9;
  undefined4 uVar10;
  int iVar11;
  int iVar12;
  float fVar13;
  char local_5;

  iVar9 = *(int *)(DAT_00880360 + 0x34);
  iVar11 = (in_EAX & 0xffff) * 0x724;
  iVar12 = iVar11 + iVar9;
  iVar2 = *(int *)((*(uint *)(iVar11 + 0x5c + iVar9) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar3 = *(int *)(DAT_006f1d6c + 0xc);
  puVar4 = *(uint **)((*(uint *)(iVar11 + 0x58 + iVar9) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  local_5 = '\0';
  if ((((0 < *(short *)(iVar12 + 0x3a8)) && (*(int *)(iVar12 + 0x158) == -1)) &&
      (iVar9 = (*(uint *)(iVar12 + 0x3ac) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34),
      *(char *)(iVar9 + 0xa4) != '\0')) &&
     (((sVar1 = *(short *)(iVar9 + 0x38), sVar1 == 0 || (sVar1 == 1)) &&
      ((*(int *)(iVar12 + 0x36c) == -1 || (*(int *)(iVar12 + 0x36c) + 0x1e <= iVar3)))))) {
    *(int *)(iVar12 + 0x36c) = iVar3;
    cVar7 = actor_should_throw_grenade(1);
    if (cVar7 != '\0') {
      cVar7 = FUN_0040dd50();
      if (cVar7 != '\0') {
        return '\x01';
      }
      if (((*puVar4 & 0x400000) != 0) &&
         (cVar7 = FUN_0040de20(*(undefined4 *)(iVar12 + 0x3ac)), cVar7 != '\0')) {
        return '\x01';
      }
    }
  }
  if ((*(char *)(iVar12 + 0x374) == '\0') || (*(char *)(iVar12 + 0x378) != '\0')) {
    fVar13 = (float)puVar4[0xc4];
  }
  else {
    fVar13 = (float)puVar4[0xc5];
  }
  if (((*(char *)(iVar12 + 0x1ca) != '\0') && (0.0 < (float)puVar4[0xc6])) && (1.1 < fVar13)) {
    fVar13 = 1.1;
  }
  if (fVar13 < *(float *)(iVar12 + 0x354)) {
    if (*(char *)(iVar12 + 0x504) == '\0') {
      random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    }
    if ((*(short *)(iVar2 + 0x184) == 2) && (FUN_0040dc30(), extraout_AL != '\0')) {
      *(undefined4 *)(iVar12 + 0x354) = 0;
      local_5 = '\x01';
    }
    bVar5 = true;
    if ((*(char *)(iVar12 + 0x358) != '\0') && ((*puVar4 & 0x20) != 0)) {
      bVar5 = false;
      if ((*(uint *)(iVar12 + 0x270) != 0xffffffff) &&
         ((iVar9 = (*(uint *)(iVar12 + 0x270) & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34),
          *(char *)(iVar9 + 0x122) < '\x03' && (*(char *)(iVar9 + 0x121) < '\x02')))) {
        bVar5 = true;
      }
    }
    bVar6 = true;
    if ((*(short *)(iVar12 + 0x6c) == 10) &&
       ((*(short *)(iVar12 + 0xa0) == 2 || (bVar6 = true, *(short *)(iVar12 + 0xa0) == 3)))) {
      bVar6 = false;
    }
    if (local_5 == '\0') {
      if ((bVar5) &&
         ((*(int *)(iVar12 + 0x36c) == -1 || (*(int *)(iVar12 + 0x36c) + 0x1e <= iVar3)))) {
        *(int *)(iVar12 + 0x36c) = iVar3;
        cVar7 = actor_should_throw_grenade(0);
        if ((cVar7 != '\0') &&
           ((fVar13 = random_real(), fVar13 < (float)puVar4[0xc6] &&
            (cVar7 = FUN_0040dd50(), cVar7 != '\0')))) {
          uVar10 = FUN_004283d0(0xffffffff,0xffffffff,0xffffffff,0);
          ai_communication_broadcast(0x18,*(undefined4 *)(iVar12 + 0x18),uVar10);
          *(undefined4 *)(iVar12 + 0x354) = 0;
          return '\x01';
        }
      }
      if (((bVar6) && (*(short *)(iVar12 + 0x368) == 0)) && (cVar7 = FUN_0040de70(), cVar7 != '\0'))
      {
        *(undefined4 *)(iVar12 + 0x354) = 0;
        uVar8 = __ftol();
        *(undefined2 *)(iVar12 + 0x368) = uVar8;
        *(undefined1 *)(iVar12 + 0x3bb) = 1;
        local_5 = '\x01';
      }
    }
  }
  return local_5;
}
#endif
