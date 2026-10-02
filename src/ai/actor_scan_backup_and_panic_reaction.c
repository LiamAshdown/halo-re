// actor_scan_backup_and_panic_reaction  (Ghidra: actor_scan_backup_and_panic_reaction, renamed)
// address 0x423220, size 423 bytes
// name confidence: 0.35   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x423220..0x4233c6 (+0x122 compared as a signed byte).)
// evidence: types/ai.h actor.unknown_8d, actor.unknown_308/unknown_30c (danger-slot pair,
//   shared with the whole 0x423220..0x423670 family), actor.unknown_39c; prop.is_unit (0x60),
//   prop.actor_type (0x10), prop.distance (0x11c), prop.unknown_32/unknown_122,
//   prop.engaged/unknown_a6/unknown_a8 (0xa4/0xa6/0xa8); types/tags.h Actor.leader_type
//   (0x2a4), Actor.leader_killed_panic_chance (0x2a8), Actor.friend_killed_panic_chance
//   (0x2a0), Actor.more_flags bit 0x20 "panic_in_groups" (same bitfield convention as
//   actor_find_best_firing_position.c and actor_update_firing_state.c). Calls
//   actor_get_relevant_squad_member_target (0x41f550), actor_scale_value_by_ally_exposure
//   (0x420c90), both already established/rewritten in this module, and random_real (math
//   module).
// register convention: EAX -> target_prop_index, stack -> actor_index.
//   // blam-cc: EAX -> target_prop_index, stack -> actor_index
// UNSURE: actor.unknown_39c (a datum_index per types/ai.h) is compared here as a plain tick
//   value, the same "handle field reused as a timestamp" pattern seen at actor.unknown_3a0 in
//   actor_start_search_timer @0x422130.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern data_array *prop_data;       // 0x008802c0
extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t random_seed_global; // 0x00719cd0
extern game_time_globals *game_time;   // 0x006f1d6c

extern real random_real(void); // 0x4019f0
extern datum_index actor_get_relevant_squad_member_target(uint32_t unused_param, datum_index member_prop_index, char require_is_unit); // 0x41f550
extern uint8_t actor_scale_value_by_ally_exposure(datum_index actor_index, float *value); // 0x420c90, EAX, stack

// blam-cc: EAX -> target_prop_index, stack -> actor_index
// Per-perception-tick reaction for a non-unit target prop: marks the actor "has scanned a
// prop this cycle" (unknown_8d), and if the target's owning actor type matches this actor's
// Actor.leader_type, randomly rolls (a global PRNG advance) against
// Actor.leader_killed_panic_chance to raise danger code 8 for it. Then, if the target is
// close (< 8 world units), resolves the most relevant nearby ally target via
// actor_get_relevant_squad_member_target and, if that target is itself a unit under attack
// and not fully engaged/shielded, either rolls a group-panic chance (scaled down by nearby
// ally exposure when the tag allows group panic and the actor's own panic cooldown has
// elapsed) or a plain random chance against Actor.friend_killed_panic_chance to raise danger
// code 3 for it; either way bumps that ally's exposure counter and resets its cooldown timer.
void actor_scan_backup_and_panic_reaction(datum_index target_prop_index, datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    datum_index relevant;

    self->witnessed_death = 1;

    if (target->enemy != 0) {
        return;
    }

    relevant = actor_get_relevant_squad_member_target(actor_index, target_prop_index, 1);

    if (target->actor_type == actor_tag->leader_type && self->pending_panic_type < 8) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        if ((float)(random_seed_global >> 0x10) * 1.5259022e-05f < actor_tag->leader_killed_panic_chance) {
            self->pending_panic_type = 8;
            self->pending_panic_prop_index = relevant;
        }
    }

    if (target->distance >= 8.0f) {
        return;
    }
    if (relevant == (datum_index)k_datum_index_none) {
        return;
    }

    {
        prop *ally = &((prop *)prop_data->data)[relevant & 0xffff];

        if (ally->enemy != 0) {
            if (ally->visual_perception > 0 && (int8_t)ally->aiming_at_actor_class <= 2) { // 0x42333d: signed byte compare
                float chance = actor_tag->friend_killed_panic_chance;
                int roll_ok;

                // 0x423346..0x42338a: with panic_in_groups and the cooldown over, the chance is scaled by ally
                //   exposure and the roll skipped when the scaler returns 1; otherwise random_real() < chance.
                if ((actor_tag->more_flags & 0x20) != 0 /* panic_in_groups */ &&
                    self->panic_cooldown_time < (int32_t)game_time->game_time &&
                    actor_scale_value_by_ally_exposure(actor_index, &chance)) {
                    roll_ok = 1;
                } else {
                    roll_ok = random_real() < chance;
                }

                if (roll_ok && self->pending_panic_type < 3) {
                    self->pending_panic_type = 3;
                    self->pending_panic_prop_index = relevant;
                }
            }

            if (ally->engaged != 0) {
                ally->friends_killed = ally->friends_killed + 1;
                ally->friends_killed_timer = 0x2ee;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x423220):

void FUN_00423220(uint param_1)

{
  int iVar1;
  char cVar2;
  uint in_EAX;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  float fVar7;
  float local_4;

  iVar1 = DAT_00880360;
  iVar4 = (param_1 & 0xffff) * 0x724;
  iVar5 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  *(undefined1 *)(iVar4 + 0x8d + *(int *)(DAT_00880360 + 0x34)) = 1;
  if (*(char *)(iVar5 + 0x60) != '\0') {
    return;
  }
  iVar6 = *(int *)(iVar1 + 0x34) + iVar4;
  iVar1 = *(int *)((*(uint *)(*(int *)(iVar1 + 0x34) + 0x58 + iVar4) & 0xffff) * 0x20 + 0x14 +
                  DAT_0087bc14);
  uVar3 = FUN_0041f550(param_1,1);
  if (((*(short *)(iVar5 + 0x10) == *(short *)(iVar1 + 0x2a4)) && (*(short *)(iVar6 + 0x308) < 8))
     && (random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f,
        (float)(random_seed_global >> 0x10) * 1.5259022e-05 < *(float *)(iVar1 + 0x2a8))) {
    *(undefined2 *)(iVar6 + 0x308) = 8;
    *(uint *)(iVar6 + 0x30c) = uVar3;
  }
  if (8.0 <= *(float *)(iVar5 + 0x11c)) {
    return;
  }
  if (uVar3 == 0xffffffff) {
    return;
  }
  iVar4 = (uVar3 & 0xffff) * 0x138;
  iVar5 = iVar4 + *(int *)(DAT_008802c0 + 0x34);
  if (*(char *)(iVar4 + 0x60 + *(int *)(DAT_008802c0 + 0x34)) != '\0') {
    if ((((0 < *(short *)(iVar5 + 0x32)) && (*(char *)(iVar5 + 0x122) < '\x03')) &&
        (((local_4 = *(float *)(iVar1 + 0x2a0), (*(byte *)(iVar1 + 4) & 0x20) != 0 &&
          ((*(int *)(iVar6 + 0x39c) < *(int *)(DAT_006f1d6c + 0xc) &&
           (cVar2 = FUN_00420c90(&local_4), cVar2 != '\0')))) ||
         (fVar7 = random_real(), fVar7 < local_4)))) && (*(short *)(iVar6 + 0x308) < 3)) {
      *(undefined2 *)(iVar6 + 0x308) = 3;
      *(uint *)(iVar6 + 0x30c) = uVar3;
    }
    if (*(char *)(iVar5 + 0xa4) != '\0') {
      *(short *)(iVar5 + 0xa6) = *(short *)(iVar5 + 0xa6) + 1;
      *(undefined2 *)(iVar5 + 0xa8) = 0x2ee;
    }
    return;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
