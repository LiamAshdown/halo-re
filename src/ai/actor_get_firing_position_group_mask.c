// actor_get_firing_position_group_mask  (Ghidra: actor_get_firing_position_group_mask, renamed)
// address 0x412880, size 224 bytes
// name confidence: 0.7   rewrite confidence: 0.85
// evidence: it indexes the owning ScenarioEncounter squads block at stride 0xe8 by
//   actor.squad_index and returns one of the seven consecutive ScenarioSquadAttacking
//   dwords at ScenarioSquad+0x54..+0x6c (attacking, attacking_search, attacking_guard,
//   defending, defending_search, defending_guard, pursuing, per types/tags.h). The result
//   is used by actor_find_best_firing_position @0x412ba0 and
//   actor_firing_position_near_point @0x412960 as a mask against each
//   ScenarioFiringPosition group_index bit, which is what the field is for.
// register convention: actor_index in EAX, kind in SI, and the search override is the one
//   Ghidra-recognized stack parameter.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;    // 0x00880360
extern Scenario *global_scenario; // 0x00746f8c

// blam-cc: EAX -> actor_index, SI -> kind, stack -> search_override
// Picks which of the seven per-squad firing position group masks applies to this actor
// right now. unknown_374 (the platoon defending flag) selects the attacking or the
// defending half; unknown_98 (the searching flag, overridable by the caller with 1 to force
// it on or 2 to force it off) selects the plain or the _search variant. kind 4 asks for the
// guard variant of whichever half applies, kind 5 for pursuing, and kind 1 for
// defending_guard unconditionally.
uint32_t actor_get_firing_position_group_mask(datum_index actor_index, int16_t kind,
                                              int16_t search_override)
{
    actor *self;
    ScenarioEncounter *encounters;
    ScenarioSquad *squad;
    uint32_t *groups; // the seven masks at ScenarioSquad+0x54
    uint8_t searching;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));
    if (self->encounter_index == (datum_index)0xffffffff) {
        return 0;
    }

    encounters = (ScenarioEncounter *)global_scenario->encounters.pointer;
    squad = &((ScenarioSquad *)encounters[self->encounter_index & 0xffff].squads.pointer)
                 [self->squad_index];
    groups = &squad->attacking;

    searching = self->search_firing_positions;
    if (search_override == 1) {
        searching = 1;
    } else if (search_override == 2) {
        searching = 0;
    }

    // UNSURE: kind 1 returns defending_guard whatever the defending flag says. The original
    // reads ScenarioSquad+0x68 outright, with no index arithmetic, so this is what the
    // binary does and not a transcription slip.
    if (kind == 1) {
        return squad->defending_guard;
    }
    if (kind == 4) {
        return groups[(self->defending != 0 ? 3 : 0) + 2]; // attacking_guard or defending_guard
    }
    if (kind == 5) {
        return squad->pursuing;
    }
    if (self->defending != 0) {
        return groups[(searching != 0 ? 1 : 0) + 3];         // defending or defending_search
    }
    return groups[searching != 0 ? 1 : 0];                   // attacking or attacking_search
}

#if 0
Original Ghidra decompilation (0x412880):

undefined4 FUN_00412880(short param_1)

{
  uint uVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  char cVar4;
  short unaff_SI;

  iVar2 = (in_EAX & 0xffff) * 0x724;
  uVar1 = *(uint *)(iVar2 + 0x34 + *(int *)(DAT_00880360 + 0x34));
  iVar2 = iVar2 + *(int *)(DAT_00880360 + 0x34);
  if (uVar1 == 0xffffffff) {
    return 0;
  }
  iVar3 = *(short *)(iVar2 + 0x3a) * 0xe8 +
          *(int *)((uVar1 & 0xffff) * 0xb0 + 0x84 + *(int *)(global_scenario + 0x430));
  cVar4 = *(char *)(iVar2 + 0x98);
  if (param_1 == 1) {
    cVar4 = '\x01';
  }
  else if (param_1 == 2) {
    cVar4 = '\0';
  }
  if (unaff_SI == 1) {
    return *(undefined4 *)(iVar3 + 0x68);
  }
  if (unaff_SI == 4) {
    return *(undefined4 *)
            (iVar3 + 0x54 + (short)((-(ushort)(*(char *)(iVar2 + 0x374) != '\0') & 3) + 2) * 4);
  }
  if (unaff_SI == 5) {
    return *(undefined4 *)(iVar3 + 0x6c);
  }
  if (*(char *)(iVar2 + 0x374) != '\0') {
    return *(undefined4 *)(iVar3 + 0x54 + (short)((cVar4 != '\0') + 3) * 4);
  }
  return *(undefined4 *)(iVar3 + 0x54 + (short)(ushort)(cVar4 != '\0') * 4);
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
