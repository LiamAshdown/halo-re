// actor_squad_react_to_grenade  (Ghidra: actor_squad_react_to_grenade, already named)
// address 0x42a3a0, size 388 bytes
// name confidence: 0.5   rewrite confidence: 0.9 (VERIFIED against 0x42a3a0 (jump table 0x42a524: 0 dialogue, 1 react, 2 panic scan, 3 release; +0x66/+0x68 update))
// evidence: types/ai.h prop.unknown_66/68/12f/12e/127/combat_dirty(0x64)/unknown_30/34/36;
//   encounter.unknown_41 (ScenarioEncounter.flags bit 3). Calls actor_queue_recognized_target_dialogue
//   (0x422550), actor_react_to_seen_target (0x422ec0), actor_scan_backup_and_panic_reaction
//   (0x423220) and actor_set_units_active (0x427860), all already rewritten in this module,
//   plus actor_target_data_release (0x41b980, already rewritten in an earlier session's
//   range) and actor_target_data_release -- Ghidra shows every one of these calls with zero-to-one visible
//   arguments; the actor_index each of them additionally needs is assumed to be the same
//   unaff_ESI this function itself receives, not independently confirmed with objdump.
// register convention: ESI -> actor_index, stack -> target_prop_index, AX -> grenade_type.
//   // blam-cc: ESI -> actor_index, stack -> target_prop_index, EAX -> grenade_type

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "cache.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0
extern data_array *encounter_data; // 0x008802c8

extern void actor_queue_recognized_target_dialogue(datum_index actor_index, datum_index target_prop_index); // 0x422550
extern void actor_react_to_seen_target(datum_index actor_index, datum_index target_prop_index); // 0x422ec0
extern void actor_scan_backup_and_panic_reaction(datum_index target_prop_index, datum_index actor_index); // 0x423220
extern void actor_set_units_active(datum_index actor_index, uint8_t dormant); // 0x427860
extern uint32_t actor_target_data_release(datum_index target_prop_index, uint32_t actor_index, uint8_t *out_conflict_flag); // 0x41b980

// blam-cc: ESI -> actor_index, stack -> target_prop_index, EAX -> grenade_type
// Reacts a squad to an incoming grenade of a given type by triggering the corresponding
// avoidance behavior (0: recognized-target dialogue with a "notice" flag set; 1: seen-target
// reaction with a marked-for-attention flag; 2: a vault/cover flag and a backup/panic scan;
// 3: full target-data release), gated by whether the actor's encounter already forbids
// squad reactions (bit 3) and by a per-prop "already handling one" timer that is only
// raised (never lowered) unless the new grenade type outranks the currently recorded one.
void actor_squad_react_to_grenade(datum_index actor_index, datum_index target_prop_index, int16_t grenade_type)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];
    prop *target = &((prop *)prop_data->data)[target_prop_index & 0xffff];
    uint8_t encounter_forbids = 0;

    if (self->encounter_index != (datum_index)k_datum_index_none) {
        encounter *enc = &((encounter *)encounter_data->data)[self->encounter_index & 0xffff];
        encounter_forbids = *((uint8_t *)enc + 0x41) != 0; // ScenarioEncounter.flags bit 3
    }

    if (target->stimulus_type == -1 || target->stimulus_type <= grenade_type) {
        target->stimulus_type = grenade_type;
        target->stimulus_timer = (uint16_t)(((grenade_type != 3) - 1) & 0x78) + 0x1e;
    }

    switch (grenade_type) {
    case 0:
        if (target->disregarded == 0) { // UNSURE offset (unnamed byte)
            *(int16_t *)((uint8_t *)&target->auditory_perception + 2) = 3; // offset 0x36
            target->perception_level = 3;
            target->combat_dirty = 1;
            actor_queue_recognized_target_dialogue(actor_index, target_prop_index);
        }
        break;
    case 1:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->shooting = 1;
            target->combat_dirty = 1;
            *(int16_t *)&target->auditory_perception = 3;
            target->perception_level = 3;
            if (target->is_parented != 0) {
                actor_set_units_active(actor_index, 0); // BL = 0 at 0x42a470, 0x42a4b1, 0x42a50e
            }
            actor_react_to_seen_target(actor_index, target_prop_index);
        }
        break;
    case 2:
        if (encounter_forbids == 0 && target->disregarded == 0) {
            target->dead = 1;
            *(int16_t *)&target->auditory_perception = 3;
            target->perception_level = 3;
            target->combat_dirty = 1;
            actor_set_units_active(actor_index, 0); // BL = 0 at 0x42a470, 0x42a4b1, 0x42a50e
            actor_scan_backup_and_panic_reaction(target_prop_index, actor_index);
        }
        break;
    case 3:
        if (target->disregarded == 0) {
            *(int16_t *)((uint8_t *)&target->auditory_perception + 2) = 3; // offset 0x36
            target->perception_level = 3;
            target->combat_dirty = 1;
            if (target->is_parented != 0) {
                actor_set_units_active(actor_index, 0); // BL = 0 at 0x42a470, 0x42a4b1, 0x42a50e
            }
            actor_target_data_release(target_prop_index, actor_index, 0);
        }
        break;
    }
}

#if 0
Original Ghidra decompilation (0x42a3a0):

void actor_squad_react_to_grenade(uint param_1)

{
  uint uVar1;
  short in_AX;
  int iVar2;
  char cVar3;
  uint unaff_ESI;

  uVar1 = *(uint *)((unaff_ESI & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x34);
  iVar2 = (param_1 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
  if (uVar1 == 0xffffffff) {
    cVar3 = '\0';
  }
  else {
    cVar3 = *(char *)((uVar1 & 0xffff) * 0x6c + 0x41 + *(int *)(DAT_008802c8 + 0x34));
  }
  if ((*(short *)(iVar2 + 0x66) == -1) || (*(short *)(iVar2 + 0x66) <= in_AX)) {
    *(short *)(iVar2 + 0x66) = in_AX;
    *(ushort *)(iVar2 + 0x68) = ((in_AX != 3) - 1 & 0x78) + 0x1e;
  }
  switch(in_AX) {
  case 0:
    if (*(char *)(iVar2 + 0x133) == '\0') {
      *(undefined2 *)(iVar2 + 0x36) = 3;
      *(undefined2 *)(iVar2 + 0x30) = 3;
      *(undefined1 *)(iVar2 + 100) = 1;
      FUN_00422550(param_1);
      return;
    }
    break;
  case 1:
    if ((cVar3 == '\0') && (*(char *)(iVar2 + 0x133) == '\0')) {
      *(undefined1 *)(iVar2 + 0x12f) = 1;
      *(undefined1 *)(iVar2 + 100) = 1;
      *(undefined2 *)(iVar2 + 0x34) = 3;
      *(undefined2 *)(iVar2 + 0x30) = 3;
      if (*(char *)(iVar2 + 0x12e) != '\0') {
        actor_set_units_active();
      }
      FUN_00422ec0(param_1);
      return;
    }
    break;
  case 2:
    if ((cVar3 == '\0') && (*(char *)(iVar2 + 0x133) == '\0')) {
      *(undefined1 *)(iVar2 + 0x127) = 1;
      *(undefined2 *)(iVar2 + 0x34) = 3;
      *(undefined2 *)(iVar2 + 0x30) = 3;
      *(undefined1 *)(iVar2 + 100) = 1;
      actor_set_units_active();
      FUN_00423220();
      return;
    }
    break;
  case 3:
    if (*(char *)(iVar2 + 0x133) == '\0') {
      *(undefined2 *)(iVar2 + 0x36) = 3;
      *(undefined2 *)(iVar2 + 0x30) = 3;
      *(undefined1 *)(iVar2 + 100) = 1;
      if (*(char *)(iVar2 + 0x12e) != '\0') {
        actor_set_units_active();
      }
      FUN_0041b980();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
