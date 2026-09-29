// ai_unit_create_actor  (Ghidra: ai_unit_create_actor; named for this rewrite)
// address 0x435420, size 276 bytes
// name confidence: 0.4   rewrite confidence: 0.95
// evidence: phase-4 summary ("allocates and initializes a fresh actor record with default
//   combat/status field values for a newly recognized unit"). It calls actor_new (0x426760,
//   types/ai.h's primary evidence for the actor layout) and actor_attach_to_unit (0x427560),
//   links the new actor onto the global unassigned list through
//   ai_actor_link_to_unassigned_list (0x436940, already rewritten) and seeds exactly the
//   fields types/ai.h already credits to "0x435420" (actor.unknown_60, unknown_62,
//   unknown_68, command_list_index, unknown_92).
// register convention: EAX -> actor_variant_tag, stack -> unit_index.
//   // blam-cc: EAX -> actor_variant_tag, stack -> unit_index
//
// UNSURE:
//  - The argument in EAX is a tag index: it is resolved through tag_instances twice (the
//    variant's own data, then the tag at +0x10, whose first dword is tested for bit 26).
//    That matches ActorVariant -> actor_definition -> Actor.flags "swarm" bit, so the guard
//    reads as "refuse to build a normal actor for a swarm variant".
//  - The vtable check `actor.unknown_06 != actor_type_procs[actor.type][0x0d]` compares the
//    actor's swarm flag against the per-type procedure table's 14th dword; if they disagree
//    the actor is deleted again. Preserved verbatim.
//  - actor_new (0x426760) and actor_delete (0x427e60) are shown by Ghidra with fewer
//    arguments than their own rewrites declare; the declarations below match this call site.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "ai.h"

extern ai_globals *ai_globals_ptr;   // 0x00880354
extern tag_instance *tag_instances;  // 0x0087bc14
extern data_array *actor_data;       // 0x00880360
extern uint8_t *actor_type_procs[]; // 0x006853b8, one definition pointer per actor type (+0xd swarm byte)

extern datum_index actor_new(datum_index actor_variant_tag);          // 0x426760, stack
extern void actor_attach_to_unit(datum_index actor_index, datum_index unit_index); // 0x427560
extern void actor_delete(datum_index actor_index, uint32_t flag);     // 0x427e60, blam-cc: EBX -> actor_index, stack -> flag
extern void ai_actor_link_to_unassigned_list(datum_index actor_index); // 0x436940
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, ECX, stack

// blam-cc: EAX -> actor_variant_tag, stack -> unit_index
// Creates the controlling actor for a unit that has just come under AI control, unless the
// variant is a swarm variant, the unit's definition forbids it, or the freshly built actor
// disagrees with its own actor-type procedure table.
void ai_unit_create_actor(datum_index actor_variant_tag, datum_index unit_index)
{
    datum_index actor_definition_tag;
    object *unit_definition;
    datum_index actor_index;
    actor *a;

    if (ai_globals_ptr->actors_valid == 0 ||
        unit_index == (datum_index)k_datum_index_none ||
        actor_variant_tag == (datum_index)k_datum_index_none) {
        return;
    }

    actor_definition_tag = *(datum_index *)((uint8_t *)tag_instances
        [actor_variant_tag & 0xffff].data + 0x10);
    if (actor_definition_tag == (datum_index)k_datum_index_none) {
        return;
    }
    if ((**(uint32_t **)&tag_instances[actor_definition_tag & 0xffff].data & 0x4000000) != 0) {
        return; // Actor.flags bit 26, "swarm"
    }

    // FIXED (0x435486..0x4354a9): ECX = the unit (ebp) and actor_new gets the variant (esi); the draft passed
    //   neither. The type byte compared at 0x435510 is +0xd of the type definition, not uint32 [0xd].
    unit_definition = object_try_and_get(unit_index, 1);
    if (unit_definition == 0) {
        return;
    }
    if ((*((uint8_t *)unit_definition + 0x106) & 4) != 0) {
        return;
    }

    actor_index = actor_new(actor_variant_tag);
    if (actor_index == (datum_index)k_datum_index_none) {
        return;
    }

    a = &((actor *)actor_data->data)[actor_index & 0xffff];
    ai_actor_link_to_unassigned_list(actor_index);
    a->awareness_level = 2;
    a->unknown_60 = 2;
    a->unknown_62 = 2;
    a->unknown_8e = 0;
    a->unknown_92 = 2;
    a->command_list_index = -1;
    a->unknown_68 = 0;

    if (*((uint8_t *)a + 0x6) != actor_type_procs[((actor *)a)->type][0xd]) {
        actor_delete(actor_index, 0);
        return;
    }
    actor_attach_to_unit(actor_index, unit_index);
}

#if 0
Original Ghidra decompilation (0x435420):

void FUN_00435420(int param_1)

{
  uint in_EAX;
  int iVar1;
  uint uVar2;

  if (((((*(char *)(DAT_00880354 + 1) != '\0') && (param_1 != -1)) && (in_EAX != 0xffffffff)) &&
      ((uVar2 = *(uint *)(*(int *)((in_EAX & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) + 0x10),
       uVar2 != 0xffffffff &&
       ((**(uint **)((uVar2 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14) & 0x4000000) == 0)))) &&
     ((iVar1 = object_try_and_get(1), iVar1 != 0 &&
      (((*(byte *)(iVar1 + 0x106) & 4) == 0 && (uVar2 = actor_new(), uVar2 != 0xffffffff)))))) {
    iVar1 = (uVar2 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
    FUN_00436940();
    *(undefined2 *)(iVar1 + 0x6a) = 2;
    *(undefined2 *)(iVar1 + 0x60) = 2;
    *(undefined2 *)(iVar1 + 0x62) = 2;
    *(undefined1 *)(iVar1 + 0x8e) = 0;
    *(undefined2 *)(iVar1 + 0x92) = 2;
    *(undefined2 *)(iVar1 + 0x90) = 0xffff;
    *(undefined1 *)(iVar1 + 0x68) = 0;
    if (*(char *)(iVar1 + 6) != (&PTR_PTR_006853b8)[*(short *)(iVar1 + 4)][0xd]) {
      actor_delete(0);
      return;
    }
    actor_attach_to_unit(uVar2,param_1);
  }
  return;
}
#endif
