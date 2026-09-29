// actor_update_grenade_and_morale_reactions  (Ghidra: actor_update_grenade_and_morale_reactions, renamed)
// address 0x40b920, size 800 bytes
// name confidence: 0.4   rewrite confidence: 0.85
// REWRITTEN from objdump 0x40b920..0x40bc3f (the draft called actor_handle_death, actor_check_pain_reaction,
//   actor_consider_grenade_throw, actor_evaluate_grenade_target_position and actor_get_target_prop_object_index
//   without their arguments, tested Actor flag 0x40000000 instead of 0x400000, used < where the binary uses <=
//   and set the grenade cooldown to the game time). EAX: actor. An actor on foot facing a thrown grenade
//   (+0x3a8, prop +0x3ac of kind 0 / 1) may react to it at most every 30 ticks. Once the danger meter (+0x354)
//   reaches the Actor tag's evasion threshold (+0x310, or +0x314 while defending; at most 1.1 when +0x1ca and
//   the cover chance +0x318 is positive) the actor may throw a grenade (variant +0x184 == 2), take cover and
//   tell the others (event 0x18), or pick a grenade target (cooldown Actor +0x31c seconds).
// blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"
#include "objects.h"
#include "units.h"


extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern game_time_globals *game_time; // 0x006f1d6c
extern uint32_t random_seed_global;  // 0x00719cd0

extern real random_real(void); // 0x4019f0
extern uint8_t actor_should_throw_grenade(uint32_t actor_index, char force); // 0x40b840, EAX, stack
extern uint8_t actor_consider_grenade_throw(datum_index actor_index); // 0x40dc30, stack
extern uint8_t actor_handle_death(datum_index actor_index, uint8_t param_2, uint8_t param_3); // 0x40dd50, stack
extern uint8_t actor_check_pain_reaction(uint32_t resolved_target, uint8_t use_alt_base,
    uint16_t order_code, datum_index actor_index); // 0x40de20, stack, DL, CX, ESI
extern uint8_t actor_evaluate_grenade_target_position(datum_index actor_index); // 0x40de70, EBX
extern datum_index actor_get_target_prop_object_index(datum_index actor_index); // 0x4283d0, EAX
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason,
    datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340, seven stack arguments

#define TAG_DATA(t) ((uint8_t *)tag_instances[(t) & 0xffff].data)

char actor_update_grenade_and_morale_reactions(uint32_t actor_index)
{
    uint8_t *act = (uint8_t *)actor_data->data + (actor_index & 0xffff) * 0x724;
    uint8_t *variant = TAG_DATA(((actor *)act)->actor_variant_tag);
    uint8_t *actor_tag = TAG_DATA(((actor *)act)->actor_definition_tag);
    int32_t now = game_time->game_time;
    char result = 0;
    float threshold;
    uint8_t may_evade;
    uint8_t may_target;

    if (((struct actor *)act)->unknown_3a8 > 0 && ((actor *)act)->active_unit_index == k_datum_index_none) {
        uint8_t *threat = (uint8_t *)prop_data->data + (((struct actor *)act)->unknown_3ac & 0xffff) * 0x138;

        if (threat[0xa4] != 0 && (((struct prop *)threat)->engagement_reachability == 0 || ((struct prop *)threat)->engagement_reachability == 1) &&
            (*(int32_t *)&((struct actor *)act)->unknown_36c == -1 || *(int32_t *)&((struct actor *)act)->unknown_36c + 0x1e <= now)) {
            *(int32_t *)&((struct actor *)act)->unknown_36c = now;
            if (actor_should_throw_grenade(actor_index, 1)) {
                if (actor_handle_death(actor_index, 0, 1)) {
                    return 1;
                }
                // 0x40ba13: ECX = 5, DL = 0, ESI = the actor
                if ((*(uint32_t *)actor_tag & 0x400000) != 0 &&
                    actor_check_pain_reaction(((struct actor *)act)->unknown_3ac, 0, 5, actor_index)) {
                    return 1;
                }
            }
        }
    }

    if (act[0x374] != 0 && act[0x378] == 0) {
        threshold = ((Actor *)actor_tag)->defending_evasion_threshold;
    } else {
        threshold = ((Actor *)actor_tag)->attacking_evasion_threshold;
    }
    if (act[0x1ca] != 0 && ((Actor *)actor_tag)->evasion_seek_cover_chance > 0.0f && threshold > 1.1f) {
        threshold = 1.1f;
    }
    if (!(threshold <= *(float *)(act + 0x354))) {
        return 0;
    }
    if (act[0x504] == 0) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    }
    if (*(int16_t *)&((ActorVariant *)variant)->grenade_stimulus == 2 && actor_consider_grenade_throw(actor_index)) {
        *(float *)(act + 0x354) = 0.0f;
        result = 1;
    }
    may_evade = 1;
    may_target = 1;
    if (act[0x358] != 0 && (*(uint32_t *)actor_tag & 0x20) != 0) {
        datum_index target = ((actor *)act)->target_unit_index;

        may_evade = 0;
        if (target != k_datum_index_none) {
            uint8_t *target_prop = (uint8_t *)prop_data->data + (target & 0xffff) * 0x138;

            if ((int8_t)target_prop[0x122] <= 2 && (int8_t)target_prop[0x121] <= 1) {
                may_evade = 1;
            }
        }
    }
    if (((actor *)act)->mode == 10 && (*(int16_t *)(act + 0xa0) == 2 || *(int16_t *)(act + 0xa0) == 3)) {
        may_target = 0;
    }
    if (result) {
        return result;
    }
    if (may_evade && (*(int32_t *)&((struct actor *)act)->unknown_36c == -1 || *(int32_t *)&((struct actor *)act)->unknown_36c + 0x1e <= now)) {
        *(int32_t *)&((struct actor *)act)->unknown_36c = now;
        if (actor_should_throw_grenade(actor_index, 0) && random_real() <= ((Actor *)actor_tag)->evasion_seek_cover_chance &&
            actor_handle_death(actor_index, 0, 1)) {
            ai_communication_broadcast(0x18, ((actor *)act)->unit_index, actor_get_target_prop_object_index(actor_index),
                                       -1, -1, -1, 0);
            *(float *)(act + 0x354) = 0.0f;
            return 1;
        }
    }
    if (may_target && *(int16_t *)(act + 0x368) == 0 && actor_evaluate_grenade_target_position(actor_index)) {
        *(float *)(act + 0x354) = 0.0f;
        *(int16_t *)(act + 0x368) = (int16_t)(int32_t)(((Actor *)actor_tag)->evasion_delay_time * 30.0f);
        act[0x3bb] = 1;
        result = 1;
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
