// encounter_remove_actor  (Ghidra: squad_remove_actor; renamed for this rewrite)
// address 0x436620, size 229 bytes
// name confidence: 0.6   rewrite confidence: 0.75
// evidence: out/phase4/ai_types_notes.md ("squad_remove_actor @0x436620 and its partner
//   0x436770 remove/add an actor from an *encounter* member list, adjusting the per-squad
//   and per-platoon counters as a side effect"). It unlinks the actor from
//   encounter.first_actor / actor.next_in_encounter and decrements exactly the three
//   counters encounter_add_actor incremented.
// register convention: EAX -> actor_index (Ghidra's in_EAX), one stack argument.
//   // blam-cc: EAX -> actor_index, stack -> skip_counters
//
// UNSURE: the stack argument is passed as a literal 0 at every call site in this directory;
// when it is non-zero the three member counters are left alone, which reads as "the caller
// is moving the actor to another encounter and will re-add it immediately". Not proven.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;                        // 0x00880354
extern data_array *actor_data;                            // 0x00880360
extern data_array *encounter_data;                        // 0x008802c8
extern encounter_squad_state *encounter_squad_states;     // 0x008802cc
extern encounter_platoon_state *encounter_platoon_states; // 0x008802c4

// blam-cc: EAX -> actor_index, stack -> skip_counters
// Unlinks the actor from its encounter's member list, decrements the encounter, squad and
// platoon member counts (unless the caller asked for the counters to be left alone), and
// clears the actor's encounter / squad / platoon links.
void encounter_remove_actor(datum_index actor_index, uint8_t skip_counters)
{
    actor *actors;
    actor *a;
    encounter *enc;
    encounter_squad_state *squad_state;
    encounter_platoon_state *platoon_state;
    datum_index *link;
    datum_index current;

    if (ai_globals_ptr->actors_valid == 0) {
        return;
    }

    actors = (actor *)actor_data->data;
    a = &actors[actor_index & 0xffff];
    if (a->encounter_index == (datum_index)k_datum_index_none) {
        return;
    }
    enc = &((encounter *)encounter_data->data)[a->encounter_index & 0xffff];

    link = &enc->first_actor;
    current = enc->first_actor;
    while (current != actor_index) {
        link = &actors[current & 0xffff].next_in_encounter;
        current = *link;
    }
    *link = a->next_in_encounter;

    if (skip_counters == 0) {
        squad_state = &encounter_squad_states[(int16_t)(a->squad_index + enc->first_squad)];
        enc->member_count = enc->member_count - 1;
        if (a->counts_toward_encounter != 0) {
            enc->live_count = enc->live_count - 1;
        }
        squad_state->member_count = squad_state->member_count - 1;
        if (a->platoon_index != -1) {
            platoon_state =
                &encounter_platoon_states[(int16_t)(enc->first_platoon + a->platoon_index)];
            platoon_state->member_count = platoon_state->member_count - 1;
        }
    }

    a->next_in_encounter = (datum_index)k_datum_index_none;
    a->encounter_index = (datum_index)k_datum_index_none;
    a->platoon_index = -1;
    a->squad_index = -1;
    enc->dirty = 1;
}

#if 0
Original Ghidra decompilation (0x436620):

void squad_remove_actor(char param_1)

{
  uint *puVar1;
  short *psVar2;
  uint uVar3;
  uint in_EAX;
  int iVar4;
  int iVar5;
  int iVar6;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    iVar4 = (in_EAX & 0xffff) * 0x724;
    iVar6 = *(int *)(DAT_00880360 + 0x34);
    uVar3 = *(uint *)(iVar4 + 0x34 + iVar6);
    iVar4 = iVar4 + iVar6;
    if (uVar3 != 0xffffffff) {
      iVar5 = (uVar3 & 0xffff) * 0x6c + *(int *)(DAT_008802c8 + 0x34);
      puVar1 = (uint *)(iVar5 + 0x14);
      uVar3 = *(uint *)(iVar5 + 0x14);
      while (uVar3 != in_EAX) {
        puVar1 = (uint *)((uVar3 & 0xffff) * 0x724 + 0x2c + iVar6);
        uVar3 = *puVar1;
      }
      *puVar1 = *(uint *)(iVar4 + 0x2c);
      if (param_1 == '\0') {
        iVar6 = (short)(*(short *)(iVar4 + 0x3a) + *(short *)(iVar5 + 4)) * 0x20 + DAT_008802cc;
        *(short *)(iVar5 + 0x18) = *(short *)(iVar5 + 0x18) + -1;
        if (*(char *)(iVar4 + 0x1c) != '\0') {
          *(short *)(iVar5 + 0x1c) = *(short *)(iVar5 + 0x1c) + -1;
        }
        psVar2 = (short *)(iVar6 + 0x16);
        *psVar2 = *psVar2 + -1;
        if (*(short *)(iVar4 + 0x3c) != -1) {
          psVar2 = (short *)((short)(*(short *)(iVar5 + 8) + *(short *)(iVar4 + 0x3c)) * 0x10 +
                             DAT_008802c4 + 4);
          *psVar2 = *psVar2 + -1;
        }
      }
      *(undefined4 *)(iVar4 + 0x2c) = 0xffffffff;
      *(undefined4 *)(iVar4 + 0x34) = 0xffffffff;
      *(undefined2 *)(iVar4 + 0x3c) = 0xffff;
      *(undefined2 *)(iVar4 + 0x3a) = 0xffff;
      *(undefined1 *)(iVar5 + 0x28) = 1;
    }
  }
  return;
}
#endif
