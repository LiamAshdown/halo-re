// actor_push_recognition_entry  (Ghidra: actor_push_recognition_entry, renamed)
// address 0x4141a0, size 173 bytes
// name confidence: 0.6   rewrite confidence: 0.8
// evidence: types/ai.h cites this exact address as the writer of recognition[cursor].type
//   (+0x3c8 + i*4), recognition[cursor].firing_position_index (+0x3ca + i*4), the modulo-4
//   cursor at +0x3c6, recognition_valid / recognition_type at +0x3d8 / +0x3d9 and the
//   12-byte recognition_position at +0x3dc copied out of the encounter
//   ScenarioFiringPosition block at stride 0x18.
// register convention: actor_index in EAX, firing_position_index in CX, entry type in DL.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern data_array *actor_data;    // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: EAX -> actor_index, CX -> firing_position_index, DL -> type
// Records that the actor has just recognized something at one of its encounter firing
// positions. The current ring slot takes the type byte and the position index, the cursor
// wraps modulo 4, and the latched recognition_position is refreshed from the scenario
// firing position itself. A firing_position_index of -1 is ignored outright.
void actor_push_recognition_entry(datum_index actor_index, int16_t firing_position_index, uint8_t type)
{
    actor *self;
    ScenarioEncounter *encounter_definition;
    ScenarioFiringPosition *firing_positions;
    int16_t cursor;

    if (firing_position_index == -1) {
        return;
    }

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    // Both writes use the pre-increment cursor: the original re-reads +0x3c6 for the second
    // store, and it has not been advanced yet at that point.
    cursor = self->recognition_cursor;
    self->recognition[cursor].type = type;
    self->recognition[cursor].firing_position_index = firing_position_index;
    self->recognition_cursor = (int16_t)((cursor + 1) % 4);

    encounter_definition = &((ScenarioEncounter *)global_scenario->encounters.pointer)
                               [self->encounter_index & 0xffff];
    firing_positions = (ScenarioFiringPosition *)encounter_definition->firing_positions.pointer;

    self->recognition_valid = 1;
    self->recognition_type = type;
    self->recognition_position.x = firing_positions[firing_position_index].position.x;
    self->recognition_position.y = firing_positions[firing_position_index].position.y;
    self->recognition_position.z = firing_positions[firing_position_index].position.z;
}

#if 0
Original Ghidra decompilation (0x4141a0):

void FUN_004141a0(void)

{
  int iVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  short in_CX;
  undefined1 in_DL;
  uint uVar4;

  if (in_CX != -1) {
    iVar2 = (in_EAX & 0xffff) * 0x724;
    iVar3 = iVar2 + *(int *)(DAT_00880360 + 0x34);
    *(undefined1 *)(iVar3 + 0x3c8 + *(short *)(iVar2 + 0x3c6 + *(int *)(DAT_00880360 + 0x34)) * 4) =
         in_DL;
    *(short *)(iVar3 + 0x3ca + *(short *)(iVar3 + 0x3c6) * 4) = in_CX;
    iVar2 = global_scenario;
    uVar4 = (int)*(short *)(iVar3 + 0x3c6) + 1U & 0x80000003;
    if ((int)uVar4 < 0) {
      uVar4 = (uVar4 - 1 | 0xfffffffc) + 1;
    }
    *(short *)(iVar3 + 0x3c6) = (short)uVar4;
    iVar1 = *(int *)((*(uint *)(iVar3 + 0x34) & 0xffff) * 0xb0 + 0x9c + *(int *)(iVar2 + 0x430));
    *(undefined1 *)(iVar3 + 0x3d8) = 1;
    *(undefined1 *)(iVar3 + 0x3d9) = in_DL;
    iVar2 = iVar1 + in_CX * 0x18;
    *(undefined4 *)(iVar3 + 0x3dc) = *(undefined4 *)(iVar1 + in_CX * 0x18);
    *(undefined4 *)(iVar3 + 0x3e0) = *(undefined4 *)(iVar2 + 4);
    *(undefined4 *)(iVar3 + 0x3e4) = *(undefined4 *)(iVar2 + 8);
  }
  return;
}
#endif
