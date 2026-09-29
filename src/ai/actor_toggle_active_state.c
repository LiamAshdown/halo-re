// actor_toggle_active_state  (Ghidra: actor_toggle_active_state, renamed)
// address 0x4277c0, size 155 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: types/ai.h actor.active(0x08)/swarm(0x06)/swarm_index(0x28)/awareness_level(0x6a)/
//   unknown_0b (adjacent byte, see UNSURE); calls actor_clear_perceived_props (0x427e00),
//   actor_delete_swarm (0x4280b0), actor_create_swarm (0x427f40) and actor_set_units_active
//   (0x427860), all already rewritten in this module.
//   UNSURE: actor+0xb (written to 1 when actor_create_swarm fails) has no established name;
//   types/ai.h only names 0x0a/0x0c individually near there. Left as a raw offset.
// register convention: AL -> activate, EDI -> actor_index.
//   // blam-cc: EAX -> activate, EDI -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "fn_ai.h"

extern data_array *actor_data; // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c


extern datum_index actor_create_swarm(datum_index actor_index); // 0x427f40


// blam-cc: EAX -> activate, EDI -> actor_index
// Toggles the actor's active/dormant flag. Deactivating clears its perceived-prop list and
// swarm, deactivates its units, and stamps the current tick into a scratch field.
// Reactivating a swarm actor first (re)creates its swarm, bailing out if that fails; either
// way marks the actor active and, if its awareness level is still 0 (freshly created),
// immediately activates its units too. Returns true unless swarm creation failed.
uint8_t actor_toggle_active_state(uint8_t activate, datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->active == activate) {
        return 1;
    }

    if (activate == 0) {
        actor_clear_perceived_props(actor_index);
        actor_delete_swarm(actor_index);
        actor_set_units_active(actor_index, 1); // 0x42781d: BL still 1 from 0x4277e2 (dormant)
        self->active = 0;
        self->deactivation_tick = (int32_t)game_time->game_time;
        return 1;
    }

    if (self->swarm != 0) {
        actor_create_swarm(actor_index);
        if (self->swarm_index == (datum_index)k_datum_index_none) {
            self->swarm_pending = 1; // offset 0x0b, UNSURE meaning, see file header
            return 0;
        }
    }

    self->active = 1;
    if (self->awareness_level == 0) {
        actor_set_units_active(actor_index, 0); // 0x427844: BL = 0 (wake)
        return 1;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4277c0):

undefined1 FUN_004277c0(void)

{
  char in_AL;
  int iVar1;
  int iVar2;
  uint unaff_EDI;

  iVar1 = (unaff_EDI & 0xffff) * 0x724;
  iVar2 = iVar1 + *(int *)(DAT_00880360 + 0x34);
  if (*(char *)(iVar1 + 8 + *(int *)(DAT_00880360 + 0x34)) != in_AL) {
    if (in_AL == '\0') {
      FUN_00427e00();
      actor_delete_swarm();
      actor_set_units_active();
      iVar1 = DAT_006f1d6c;
      *(undefined1 *)(iVar2 + 8) = 0;
      *(undefined4 *)(iVar2 + 0xc) = *(undefined4 *)(iVar1 + 0xc);
      return 1;
    }
    if (*(char *)(iVar2 + 6) != '\0') {
      actor_create_swarm();
      if (*(int *)(iVar2 + 0x28) == -1) {
        *(undefined1 *)(iVar2 + 0xb) = 1;
        return 0;
      }
    }
    *(undefined1 *)(iVar2 + 8) = 1;
    if (*(short *)(iVar2 + 0x6a) == 0) {
      actor_set_units_active();
      return 1;
    }
  }
  return 1;
}
#endif
