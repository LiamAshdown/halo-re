// actor_replace_object_reference  (Ghidra: actor_replace_object_reference, already named)
// address 0x428470, size 469 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (REWRITTEN 2026-09-27 static loop against objdump 0x428470..0x42864d: active movement (not secondary action) field, 3-argument mode proc; offsets probed)
// evidence: types/ai.h actor.target_unit_index(0x270)/target_combat_status(0x268)/
//   unknown_60c/unknown_610/unknown_6b4/look_at_unknown_2f4(0x2f4)/unknown_30c/
//   search_unknown_340(0x340)/unknown_3ac/unknown_3a8/unknown_1d0/unknown_1e8/unknown_1e4/
//   secondary_action(0x46c)/vocalization_unknown_54c/vocalization_unknown_550/unknown_56c;
//   swarm.component_count/component_index; swarm_component.unknown_14. Calls no other
//   AI-module function; the final indirect call goes through a table at 0x00655274, which is
//   actor_mode_definitions[mode] + 0x20 -- a fourth per-mode procedure slot inside
//   types/ai.h's actor_mode_definition.unknown_1c[28] padding that no other function in this
//   module names.
//   UNSURE: two already-completed callers elsewhere in this module
//   (src/ai/actor_target_data_release.c, src/ai/actor_target_relationship_think.c, and
//   actor_clear_perceived_props.c in this rewrite) declare this function's extern with only
//   one visible parameter (actor_index); the real function also needs the old and new
//   reference values, which Ghidra shows only as unaff_ESI/unaff_EDI here. This file uses
//   the full three-parameter signature; the other callers were not corrected (out of this
//   rewrite's range or already-completed).
// ESI -> new_reference, EDI -> old_reference; actor_index is the one STACK argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "objects.h"
#include "units.h"

extern data_array *actor_data;           // 0x00880360
extern data_array *swarm_data;           // 0x0088035c
extern data_array *swarm_component_data; // 0x00880358
extern actor_mode_definition actor_mode_definitions[16]; // 0x00655254

// FIXED (objdump 0x428479): actor_index is read from [esp+8] after push ebx -- a stack argument (EAX is
// overwritten with actor_data first); callers push it (0x4383a5 push ecx).
// blam-cc: ESI -> new_reference, EDI -> old_reference, stack -> actor_index
// Scans an actor's numerous cached object-index fields (and its swarm members') for a stale
// object reference (old_reference) and replaces it with new_reference, clearing dependent
// state (a combat-status/kind byte alongside it) when the replacement is
// k_datum_index_none, and finally invokes the per-mode callback at
// actor_mode_definitions[mode]+0x20 (see UNSURE) with the actor index.
void actor_replace_object_reference(datum_index actor_index, uint32_t new_reference, uint32_t old_reference)
{
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    if (self->target_unit_index == old_reference) {
        self->target_unit_index = new_reference;
        if (new_reference == 0xffffffff) {
            self->target_combat_status = 0;
        }
    }

    if (self->firing_target_type == 1 && self->firing_target_prop_index == old_reference) {
        self->firing_target_prop_index = new_reference;
        if (new_reference == 0xffffffff) {
            self->firing_target_type = 0;
        }
    }

    if (self->grenade_target_prop_index == old_reference) {
        self->grenade_target_prop_index = new_reference;
    }
    if (self->look_at_unknown_2f4 == old_reference) {
        self->look_at_unknown_2f4 = new_reference;
    }
    if (self->pending_panic_prop_index == old_reference) {
        self->pending_panic_prop_index = new_reference;
    }
    if (self->search_unknown_340 == old_reference) {
        self->search_unknown_340 = new_reference;
    }
    if (self->retreat_prop_index == old_reference) {
        if (new_reference == 0xffffffff) {
            self->retreat_timer = 0;
        }
        self->retreat_prop_index = new_reference;
    }
    if (self->nearby_friend_prop_index == old_reference) {
        self->nearby_friend_prop_index = new_reference;
    }
    if (self->post_combat_prop_index == old_reference) {
        self->post_combat_prop_index = new_reference;
        if (new_reference == 0xffffffff) {
            self->post_combat_action = 0;
        }
    }

    // 0x428551..0x428577: the ACTIVE movement action (type +0x46c == 5, object +0x470). FIXED 2026-09-27: the draft
    // tested and cleared secondary_action (+0x418), so dropping a reference wiped the actor's queued secondary action.
    if (self->active_movement.type == 5 && *(uint32_t *)&((struct actor *)self)->active_movement.destination.x == old_reference) {
        if (new_reference == 0xffffffff) {
            self->active_movement.type = 0;
            self->active_movement.extra = 0xffffffff; // offset 0x480
        } else {
            *(uint32_t *)&((struct actor *)self)->active_movement.destination.x = new_reference;
        }
    }

    if (self->vocalization_unknown_54c == 1 && self->vocalization_unknown_550 == old_reference) {
        self->vocalization_unknown_550 = new_reference;
    }
    if (self->idle_major_direction_type == 1 && *(uint32_t *)((uint8_t *)self + 0x570) == old_reference) { // UNSURE offset
        *(uint32_t *)((uint8_t *)self + 0x570) = new_reference; // UNSURE offset
    }
    if (*(int16_t *)((uint8_t *)self + 0x57c) == 1 && *(uint32_t *)((uint8_t *)self + 0x580) == old_reference) { // UNSURE offsets
        *(uint32_t *)((uint8_t *)self + 0x580) = new_reference; // UNSURE offset
    }

    if (self->swarm != 0 && self->swarm_index != (datum_index)k_datum_index_none) {
        swarm *s = &((swarm *)swarm_data->data)[self->swarm_index & 0xffff];
        int16_t i;
        for (i = 0; i < s->component_count; i++) {
            swarm_component *component = &((swarm_component *)swarm_component_data->data)[s->component_index[i] & 0xffff];
            if (component->leap_target_index == old_reference) {
                component->leap_target_index = new_reference;
            }
        }
    }

    {
        // 0x428623..0x42864a: the mode's replace-reference procedure (definition +0x20: 0x404300 flee, 0x405270 guard,
        // 0x402f00 converse) is called with (actor, old_reference, new_reference) -- push esi / push edi / push ecx.
        // FIXED 2026-09-27: the draft passed only the actor, so those procedures compared and wrote garbage.
        uint32_t proc = *(uint32_t *)((uint8_t *)&actor_mode_definitions[self->mode] + 0x20);
        if (proc != 0) {
            ((void (*)(datum_index, datum_index, datum_index))proc)(actor_index, old_reference, new_reference);
        }
    }
}

#if 0
Original Ghidra decompilation (0x428470):

void actor_replace_object_reference(uint param_1)

{
  int iVar1;
  int iVar2;
  int iVar3;
  short sVar4;
  int iVar5;
  int unaff_ESI;
  int unaff_EDI;

  iVar5 = (param_1 & 0xffff) * 0x724;
  iVar2 = *(int *)(DAT_00880360 + 0x34) + iVar5;
  if ((*(int *)(*(int *)(DAT_00880360 + 0x34) + 0x270 + iVar5) == unaff_EDI) &&
     (*(int *)(iVar2 + 0x270) = unaff_ESI, unaff_ESI == -1)) {
    *(undefined2 *)(iVar2 + 0x268) = 0;
  }
  sVar4 = 0;
  if (((*(short *)(iVar2 + 0x60c) == 1) && (*(int *)(iVar2 + 0x610) == unaff_EDI)) &&
     (*(int *)(iVar2 + 0x610) = unaff_ESI, unaff_ESI == -1)) {
    *(undefined2 *)(iVar2 + 0x60c) = 0;
  }
  if (*(int *)(iVar2 + 0x6b4) == unaff_EDI) {
    *(int *)(iVar2 + 0x6b4) = unaff_ESI;
  }
  if (*(int *)(iVar2 + 0x2f4) == unaff_EDI) {
    *(int *)(iVar2 + 0x2f4) = unaff_ESI;
  }
  if (*(int *)(iVar2 + 0x30c) == unaff_EDI) {
    *(int *)(iVar2 + 0x30c) = unaff_ESI;
  }
  if (*(int *)(iVar2 + 0x340) == unaff_EDI) {
    *(int *)(iVar2 + 0x340) = unaff_ESI;
  }
  if (*(int *)(iVar2 + 0x3ac) == unaff_EDI) {
    if (unaff_ESI == -1) {
      *(undefined2 *)(iVar2 + 0x3a8) = 0;
    }
    *(int *)(iVar2 + 0x3ac) = unaff_ESI;
  }
  if (*(int *)(iVar2 + 0x1d0) == unaff_EDI) {
    *(int *)(iVar2 + 0x1d0) = unaff_ESI;
  }
  if ((*(int *)(iVar2 + 0x1e8) == unaff_EDI) &&
     (*(int *)(iVar2 + 0x1e8) = unaff_ESI, unaff_ESI == -1)) {
    *(undefined2 *)(iVar2 + 0x1e4) = 0;
  }
  if ((*(short *)(iVar2 + 0x46c) == 5) && (*(int *)(iVar2 + 0x470) == unaff_EDI)) {
    if (unaff_ESI == -1) {
      *(undefined2 *)(iVar2 + 0x46c) = 0;
      *(undefined4 *)(iVar2 + 0x480) = 0xffffffff;
    }
    else {
      *(int *)(iVar2 + 0x470) = unaff_ESI;
    }
  }
  if ((*(short *)(iVar2 + 0x54c) == 1) && (*(int *)(iVar2 + 0x550) == unaff_EDI)) {
    *(int *)(iVar2 + 0x550) = unaff_ESI;
  }
  if ((*(short *)(iVar2 + 0x56c) == 1) && (*(int *)(iVar2 + 0x570) == unaff_EDI)) {
    *(int *)(iVar2 + 0x570) = unaff_ESI;
  }
  if ((*(short *)(iVar2 + 0x57c) == 1) && (*(int *)(iVar2 + 0x580) == unaff_EDI)) {
    *(int *)(iVar2 + 0x580) = unaff_ESI;
  }
  iVar1 = DAT_00880358;
  if (((*(char *)(iVar2 + 6) != '\0') && (*(uint *)(iVar2 + 0x28) != 0xffffffff)) &&
     (iVar2 = (*(uint *)(iVar2 + 0x28) & 0xffff) * 0x98 + *(int *)(DAT_0088035c + 0x34),
     0 < *(short *)(iVar2 + 2))) {
    do {
      iVar3 = (*(uint *)(iVar2 + 0x58 + sVar4 * 4) & 0xffff) * 0x40 + *(int *)(iVar1 + 0x34);
      if (*(int *)(iVar3 + 0x14) == unaff_EDI) {
        *(int *)(iVar3 + 0x14) = unaff_ESI;
      }
      sVar4 = sVar4 + 1;
    } while (sVar4 < *(short *)(iVar2 + 2));
  }
  if (*(code **)(&DAT_00655274 + *(short *)(*(int *)(DAT_00880360 + 0x34) + iVar5 + 0x6c) * 0x38) !=
      (code *)0x0) {
    (**(code **)(&DAT_00655274 + *(short *)(*(int *)(DAT_00880360 + 0x34) + iVar5 + 0x6c) * 0x38))
              (param_1);
  }
  return;
}
#endif
