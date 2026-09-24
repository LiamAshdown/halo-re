// actor_apply_queued_look_to_unit  (Ghidra: actor_apply_queued_look_to_unit, renamed)
// address 0x42a640, size 384 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: types/ai.h actor.unknown_07/unit_index(0x18)/unknown_6ec/unknown_6d4/unknown_6d8;
//   types/units.h unit_data.controlling_player (object+0x218). Phase-4 summary: "Applies
//   queued look-direction and state changes to the actor's unit once it is no longer held in
//   a vehicle seat (or when a global debug flag forces it)." Calls unit_apply_control_block, unit_try_start_scripted_action_animation
//   and unit_refresh_targeting_flag_and_weapons, none established elsewhere in this repo.
//   UNSURE: DAT_0087a478+0x11 (a debug/force flag this rewrite could not independently name)
//   and unknown_6f0 (inside actor.unknown_6ee[14], passed as an out-parameter to
//   unit_try_start_scripted_action_animation) are both kept as raw offsets.
// register convention: EAX -> actor_index.
//   // blam-cc: EAX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;  // 0x00880360
extern data_array *object_data; // 0x008603b0
extern uint8_t force_look_apply_flag; // 0x0087a489 (0x0087a478 + 0x11), UNSURE name

extern void unit_refresh_targeting_flag_and_weapons(datum_index unit_index); // 0x569bf0, UNSURE signature
extern void unit_apply_control_block(uint32_t param); // 0x5639f0, UNSURE signature
extern void unit_try_start_scripted_action_animation(datum_index unit_index, int16_t value, void *out); // 0x569530, UNSURE signature

// blam-cc: EAX -> actor_index
void actor_apply_queued_look_to_unit(datum_index actor_index)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    object *unit_object = ((object_header *)object_data->data)[self->unit_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);

    if (unit->controlling_player == (datum_index)k_datum_index_none || force_look_apply_flag != 0) {
        if (self->unknown_07 != 0) {
            unit_refresh_targeting_flag_and_weapons(self->unit_index);
            self->unknown_07 = 0;
        }
        unit_apply_control_block(0xffffffff);
        if (self->unknown_6ec != -1) {
            unit_try_start_scripted_action_animation(self->unit_index, self->unknown_6ec, &self->unknown_6ee[2]); // offset 0x6f0
        }
        if (self->unknown_6d4 > 0) {
            unit->unknown_210 = self->unknown_6d4;
            unit->unknown_214 = self->unknown_6d8;
        }
    }
}

#if 0
Original Ghidra decompilation (0x42a640):

void FUN_0042a640(void)

{
  undefined4 uVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar2 = DAT_008603b0;
  iVar3 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  if ((*(int *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                        (*(uint *)(iVar3 + 0x18) & 0xffff) * 0xc) + 0x218) == -1) ||
     (*(char *)(DAT_0087a478 + 0x11) != '\0')) {
    if (*(char *)(iVar3 + 7) != '\0') {
      FUN_00569bf0(*(uint *)(iVar3 + 0x18));
      *(undefined1 *)(iVar3 + 7) = 0;
    }
    FUN_005639f0(0xffffffff);
    if (*(short *)(iVar3 + 0x6ec) != -1) {
      FUN_00569530(*(undefined4 *)(iVar3 + 0x18),*(short *)(iVar3 + 0x6ec),iVar3 + 0x6f0);
    }
    if (0 < *(short *)(iVar3 + 0x6d4)) {
      uVar1 = *(undefined4 *)(iVar3 + 0x6d8);
      iVar2 = *(int *)(*(int *)(iVar2 + 0x34) + 8 + (*(uint *)(iVar3 + 0x18) & 0xffff) * 0xc);
      *(int *)(iVar2 + 0x210) = (int)*(short *)(iVar3 + 0x6d4);
      *(undefined4 *)(iVar2 + 0x214) = uVar1;
    }
  }
  return;
}
#endif
