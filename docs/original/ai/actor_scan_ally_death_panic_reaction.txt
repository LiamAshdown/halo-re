// actor_scan_ally_death_panic_reaction  (Ghidra: actor_scan_ally_death_panic_reaction, renamed)
// address 0x4233d0, size 275 bytes
// name confidence: 0.35   rewrite confidence: 0.9
// evidence: types/ai.h actor.unknown_39c, actor.target_unit_index (0x270), actor.mode_data.raw
//   (mode==4 is _actor_mode_death); prop.is_unit (0x60), prop.owner_actor_index (0x1c),
//   prop.object_index (0x18); types/tags.h Actor.friend_killed_panic_chance (0x2a0),
//   Actor.more_flags bit 0x20 "panic_in_groups". Calls random_real (math module),
//   actor_scale_value_by_ally_exposure (0x420c90) and actor_find_prop_for_object, both already established
//   in this module. Sibling of actor_scan_backup_and_panic_reaction @0x423220, which shares
//   the identical friend_killed_panic_chance / panic_in_groups / ally-exposure pattern.
// VERIFIED against disassembly 0x4233d0..0x4234e2 (2026-09-30): the exposure scaler's return value is tested (nonzero skips the
//   random roll, exactly as the C does), so no branch is omitted.
// register convention: EAX -> target_prop_index, EBX -> actor_index (both unaff_).
//   // blam-cc: EAX -> target_prop_index, EBX -> actor_index

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
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void); // 0x4019f0
extern uint8_t actor_scale_value_by_ally_exposure(datum_index actor_index, float *value); // 0x420c90, EAX, stack
extern datum_index actor_find_prop_for_object(datum_index object_index, datum_index actor_index); // 0x43ea80, stack, ECX

// blam-cc: EAX -> target_prop_index, EBX -> actor_index
// If the target prop is not a unit, the actor's tag allows group panic, and the actor's own
// panic cooldown (unknown_39c) has elapsed, rolls (and exposure-scales) a chance against
// Actor.friend_killed_panic_chance; when it succeeds and no higher-priority danger is already
// claimed, claims danger code 2 for the target prop's owning actor's death-mode killer prop
// (resolved through actor_find_prop_for_object), falling back to the actor's own current target when that
// owning actor isn't in death mode or has none recorded.
void actor_scan_ally_death_panic_reaction(datum_index target_prop_index, datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    Actor *actor_tag = (Actor *)(tag_instances[self->actor_definition_tag & 0xffff].data);
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];

    if (target->enemy == 0 && (actor_tag->more_flags & 0x20) != 0 /* panic_in_groups */ &&
        self->panic_cooldown_time < (int32_t)game_time->game_time) {
        float chance = actor_tag->friend_killed_panic_chance;

        // 0x423454: the roll (0x423460) only runs when the scaler returns 0; it fails on random >= chance
        if (!actor_scale_value_by_ally_exposure(actor_index, &chance) && !(random_real() < chance)) {
            return;
        }

        if (self->pending_panic_type < 3) {
            datum_index owner_actor_index = target->owner_actor_index;
            uint32_t payload = self->target_unit_index;

            if (owner_actor_index != (datum_index)k_datum_index_none) {
                actor *owner = &((actor *)actor_data->data)[owner_actor_index & 0xffff];
                if (owner->mode == _actor_mode_death) {
                    // mode_data[0x1c]: a prop index recorded for the killer, valid only in
                    // death mode; see types/ai.h's actor.mode_data.raw union.
                    uint32_t killer_prop = *(uint32_t *)&owner->mode_data.raw[0x1c];
                    if (killer_prop != (uint32_t)k_datum_index_none) {
                        prop *killer = &((prop *)prop_data->data)[killer_prop & 0xffff];
                        payload = actor_find_prop_for_object(killer->object_index, actor_index); // 0x4234c5: ECX = actor (ebx)
                    }
                }

                self->pending_panic_type = 2;
                self->pending_panic_prop_index = payload;
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4233d0):

void FUN_004233d0(void)

{
  uint uVar1;
  int iVar2;
  char cVar3;
  uint in_EAX;
  int iVar4;
  undefined4 uVar5;
  uint unaff_EBX;
  int iVar6;
  int iVar7;
  float fVar8;
  float local_4;

  iVar2 = DAT_00880360;
  iVar6 = (unaff_EBX & 0xffff) * 0x724;
  iVar7 = iVar6 + *(int *)(DAT_00880360 + 0x34);
  iVar6 = *(int *)((*(uint *)(iVar6 + 0x58 + *(int *)(DAT_00880360 + 0x34)) & 0xffff) * 0x20 + 0x14
                  + DAT_0087bc14);
  iVar4 = (in_EAX & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (((*(char *)(iVar4 + 0x60) == '\0') && ((*(byte *)(iVar6 + 4) & 0x20) != 0)) &&
     (*(int *)(iVar7 + 0x39c) < *(int *)(DAT_006f1d6c + 0xc))) {
    local_4 = *(float *)(iVar6 + 0x2a0);
    cVar3 = FUN_00420c90(&local_4);
    if ((cVar3 == '\0') && (fVar8 = random_real(), local_4 <= fVar8)) {
      return;
    }
    if (*(short *)(iVar7 + 0x308) < 3) {
      uVar1 = *(uint *)(iVar4 + 0x1c);
      uVar5 = *(undefined4 *)(iVar7 + 0x270);
      if (uVar1 != 0xffffffff) {
        iVar6 = (uVar1 & 0xffff) * 0x724 + *(int *)(iVar2 + 0x34);
        if ((*(short *)(iVar6 + 0x6c) == 4) &&
           (uVar1 = *(uint *)(iVar6 + 0xb8), uVar1 != 0xffffffff)) {
          uVar5 = FUN_0043ea80(*(undefined4 *)
                                ((uVar1 & 0xffff) * 0x138 + 0x18 + *(int *)(DAT_008802c0 + 0x34)));
        }
        *(undefined2 *)(iVar7 + 0x308) = 2;
        *(undefined4 *)(iVar7 + 0x30c) = uVar5;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
