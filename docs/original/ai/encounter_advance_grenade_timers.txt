// encounter_advance_grenade_timers  (Ghidra: encounter_advance_grenade_timers, renamed)
// address 0x438db0, size 107 bytes
// name confidence: 0.3   rewrite confidence: 0.75
// evidence: types/ai.h encounter (0x6c stride, confirmed by encounter_data element size);
//   the three fields touched (unknown_44/0x45 gate unknown_54, unknown_47/0x48 gate
//   unknown_4a, and unknown_45 alone gates unknown_50) are still unnamed in the header, so
//   the exact meaning of each timer is UNSURE; phase-4's one-line summary ("grenade-related
//   cooldown timers") is kept as a hint only. Caller evidence: out/phase2/ai/07.md shows
//   encounters_update calling this once per live encounter alongside encounter_process_squad_reinforcements (reinforcement
//   spawn), encounter_decay_squad_spawn_delays (squad grenade-cooldown decay) and encounter_update_platoon_defending_flag/encounter_redistribute_squads_toward_targets in the
//   same per-tick sweep, all keyed off the same encounter datum index left in EAX.
// register convention: EAX -> encounter_index (blam-cc, matches the sibling calls in the
//   same caller that pass the identical value explicitly).
//
// UNSURE: which specific cooldowns encounter.unknown_50/0x54/0x4a track; the header leaves
// them unnamed. Preserved verbatim: decrement-by-0xf-while-nonzero on two datum-typed
// fields when their gate byte is clear, snap-to-zero when the gate byte is set; and a
// straight countdown-by-0xf (floor at zero) on a third field only while two other gate
// bytes are both nonzero.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *encounter_data; // 0x008802c8

// blam-cc: EAX -> encounter_index
void encounter_advance_grenade_timers(datum_index encounter_index)
{
    encounter *self;

    self = (encounter *)((uint8_t *)encounter_data->data + (encounter_index & 0xffff) * sizeof(encounter));

    if (self->engaged == 0) {
        if (self->ticks_since_engaged != (datum_index)0xffffffff) {
            self->ticks_since_engaged = self->ticks_since_engaged + 0xf;
        }
    } else {
        self->ticks_since_engaged = 0;
    }

    if (self->has_live_target == 0) {
        if (self->ticks_since_live_target != (datum_index)0xffffffff) {
            self->ticks_since_live_target = self->ticks_since_live_target + 0xf;
        }
    } else {
        self->ticks_since_live_target = 0;
    }

    if ((self->post_combat != 0) && (self->post_combat_quiet != 0)) {
        if (0xf < self->post_combat_timer) {
            self->post_combat_timer = self->post_combat_timer - 0xf;
            return;
        }
        self->post_combat_timer = 0;
    }
    return;
}

#if 0
// ---- original Ghidra decompilation (FUN_00438db0 @ 0x438db0) ----
void FUN_00438db0(void)

{
  uint in_EAX;
  int iVar1;
  int iVar2;

  iVar1 = (in_EAX & 0xffff) * 0x6c;
  iVar2 = iVar1 + *(int *)(DAT_008802c8 + 0x34);
  if (*(char *)(iVar1 + 0x45 + *(int *)(DAT_008802c8 + 0x34)) == '\0') {
    if (*(int *)(iVar2 + 0x50) != -1) {
      *(int *)(iVar2 + 0x50) = *(int *)(iVar2 + 0x50) + 0xf;
    }
  }
  else {
    *(undefined4 *)(iVar2 + 0x50) = 0;
  }
  if (*(char *)(iVar2 + 0x44) == '\0') {
    if (*(int *)(iVar2 + 0x54) != -1) {
      *(int *)(iVar2 + 0x54) = *(int *)(iVar2 + 0x54) + 0xf;
    }
  }
  else {
    *(undefined4 *)(iVar2 + 0x54) = 0;
  }
  if ((*(char *)(iVar2 + 0x47) != '\0') && (*(char *)(iVar2 + 0x48) != '\0')) {
    if (0xf < *(short *)(iVar2 + 0x4a)) {
      *(short *)(iVar2 + 0x4a) = *(short *)(iVar2 + 0x4a) + -0xf;
      return;
    }
    *(undefined2 *)(iVar2 + 0x4a) = 0;
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
