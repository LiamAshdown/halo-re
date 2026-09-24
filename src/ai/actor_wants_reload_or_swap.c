// actor_wants_reload_or_swap  (Ghidra: actor_wants_reload_or_swap, renamed)
// address 0x40ab80, size 161 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.unknown_90/unknown_92/encounter_index/squad_index/unknown_6e/
//   mode; encounter.first_squad (0x04) and encounter_squad_state.unknown_12 (0x12), reached
//   through encounter_data/encounter_squad_states exactly as established elsewhere; phase-4
//   summary "returns whether the actor currently wants to reload or swap weapons, based on
//   ammo state, squad-wide low-ammo signaling, and morale".
// register convention: actor index in EAX, the sole real parameter.
//   // blam-cc: EAX -> actor_index
// UNSURE: actor+0x9e/0xa1 fall inside actor.mode_data (a per-mode union); read here as two
//   flee-mode flags.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *actor_data;             // 0x00880360
extern data_array *encounter_data;         // 0x008802c8
extern encounter_squad_state *encounter_squad_states; // 0x008802cc

extern void encounter_squad_clear_spawn_delay(void); // 0x439270, not yet rewritten (called with no visible arguments/return used)

uint8_t actor_wants_reload_or_swap(uint32_t actor_index)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint8_t result = 0;

    if (a->unknown_90 != -1 && a->unknown_92 > 0) {
        result = 1;
    }

    if (a->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[a->encounter_index & 0xffff];
        encounter_squad_state *squad = &encounter_squad_states[enc->first_squad + a->squad_index];

        if (squad->squad_delay_ticks > 0) {
            if (a->unknown_6e < 5) {
                result = 1;
            } else {
                encounter_squad_clear_spawn_delay();
            }
        }
    }

    if (a->mode == _actor_mode_flee) {
        if (a->mode_data[0x9e - 0x9c] == 0 && a->mode_data[0xa1 - 0x9c] == 0) {
            return 1;
        }
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x40ab80):

undefined1 FUN_0040ab80(void)

{
  uint in_EAX;
  int iVar1;
  undefined1 uVar2;

  iVar1 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  uVar2 = 0;
  if ((*(short *)(iVar1 + 0x90) != -1) && (0 < *(short *)(iVar1 + 0x92))) {
    uVar2 = 1;
  }
  if ((*(uint *)(iVar1 + 0x34) != 0xffffffff) &&
     (0 < *(short *)((short)(*(short *)((*(uint *)(iVar1 + 0x34) & 0xffff) * 0x6c + 4 +
                                       *(int *)(DAT_008802c8 + 0x34)) + *(short *)(iVar1 + 0x3a)) *
                     0x20 + 0x12 + DAT_008802cc))) {
    if (*(short *)(iVar1 + 0x6e) < 5) {
      uVar2 = 1;
    }
    else {
      FUN_00439270();
    }
  }
  if (*(short *)(iVar1 + 0x6c) == 0xb) {
    if ((*(char *)(iVar1 + 0x9e) == '\0') && (*(char *)(iVar1 + 0xa1) == '\0')) {
      return 1;
    }
  }
  return uVar2;
}
#endif
