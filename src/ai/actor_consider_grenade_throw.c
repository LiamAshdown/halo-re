// actor_consider_grenade_throw  (Ghidra: actor_consider_grenade_throw, renamed)
// address 0x40dc30, size 287 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (VERIFIED 2026-09-28 against objdump 0x40dc30..0x40dd4e.)
// evidence: phase-4 summary "periodically rolls whether the actor decides to throw a
// grenade, using a randomized cooldown/probability from its grenade tag data"; reads
// ActorVariant.grenade_stimulus/minimum_enemy_count/grenade_check_time/
// encounter_grenade_timeout and rolls against random_real(), then commits through
// actor_check_grenade_facing_and_commit (0x40db00, this module).
// Ghidra's float comparison idiom `(a < b) == (a == b)` is simplified to the equivalent
// `a > b` throughout (true only when a is neither less than nor equal to b).
// UNSURE: FUN_0046fe70 is called with no visible argument in the decompiled C; treated as
// taking one float argument passed through the FPU stack (ST0), which Ghidra's decompiler
// does not surface as a formal parameter -- same call as in actor_can_throw_grenade_at_target.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include "ai.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;      // 0x00880360
extern tag_instance *tag_instances; // 0x0087bc14
extern ai_globals *ai_globals_ptr;  // 0x00880354
extern game_time_globals *game_time; // 0x006f1d6c

extern real random_real(void);                       // 0x4019f0
extern real weapon_get_zoom_fov_resolved(int16_t zoom_table_index, int16_t substitution_check_index); // 0x46fe70, ECX table, AX index: the difficulty scale
extern uint8_t actor_can_throw_grenade_at_target(datum_index actor_index); // 0x40d9c0, this module
extern uint8_t actor_check_grenade_facing_and_commit(datum_index actor_index, uint8_t force_commit); // 0x40db00, this module

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> actor_index
uint8_t actor_consider_grenade_throw(datum_index actor_index)
{
    actor *self;
    ActorVariant *variant;
    int32_t now;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    variant = (ActorVariant *)tag_instances[self->actor_variant_tag & 0xffff].data;

    if (self->grenade_throw_pending != 0) {
        return 1;
    }
    if (ai_globals_ptr->grenades_enabled == 0 || variant->grenade_stimulus == -1 ||
        variant->minimum_enemy_count == -1) {
        return 0;
    }

    now = game_time->game_time; // +0x0c

    if (self->last_grenade_check_time != (uint32_t)-1 &&
        (variant->grenade_check_time * 30.0f + (float)(int32_t)self->last_grenade_check_time) > (float)now) { // 0x40dccc: +0x1a4
        return 0;
    }

    {
        // 0x40dce5..0x40dd10: the throw probability is grenade_chance (+0x1a0) times difficulty scale 0x17 for
        // the actor's team (actor+0x3e); a uniform roll below it proceeds
        float scaled = variant->grenade_chance * weapon_get_zoom_fov_resolved(0x17, ((struct actor *)self)->team);
        float roll;

        self->last_grenade_check_time = now;
        roll = random_real();
        if (roll < scaled && actor_can_throw_grenade_at_target(actor_index) != 0) {
            self->grenade_throw_pending = 1;
            actor_check_grenade_facing_and_commit(actor_index, 1);
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x40dc30):

undefined4 FUN_0040dc30(uint param_1)

{
  int iVar1;
  int iVar2;
  float fVar3;
  char cVar4;
  int iVar5;
  int iVar6;
  float10 fVar7;
  float fVar8;

  iVar1 = *(int *)(DAT_00880360 + 0x34);
  iVar5 = (param_1 & 0xffff) * 0x724;
  iVar6 = iVar5 + iVar1;
  iVar2 = *(int *)((*(uint *)(iVar5 + 0x5c + iVar1) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  if (*(char *)(iVar5 + 0x6a0 + iVar1) != '\0') {
    return 1;
  }
  if (((*(char *)(DAT_00880354 + 0x3b4) == '\0') || (*(short *)(iVar2 + 0x180) == -1)) ||
     (*(short *)(iVar2 + 0x182) == -1)) {
    return 0;
  }
  iVar1 = *(int *)(DAT_006f1d6c + 0xc);
  if ((*(int *)(iVar6 + 0x6a4) != -1) &&
     (fVar8 = (float)iVar1,
     fVar3 = *(float *)(iVar2 + 0x1a4) * 30.0 + (float)*(int *)(iVar6 + 0x6a4),
     fVar3 < fVar8 == (fVar3 == fVar8))) {
    return 0;
  }
  fVar3 = *(float *)(iVar2 + 0x1a0);
  fVar7 = (float10)FUN_0046fe70();
  *(int *)(iVar6 + 0x6a4) = iVar1;
  fVar8 = random_real();
  if ((fVar8 < (float)(fVar7 * (float10)fVar3)) && (cVar4 = FUN_0040d9c0(), cVar4 != '\0')) {
    *(undefined1 *)(iVar6 + 0x6a0) = 1;
    FUN_0040db00(param_1,1);
    return 1;
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
