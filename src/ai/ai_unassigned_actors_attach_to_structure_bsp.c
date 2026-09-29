// ai_unassigned_actors_attach_to_structure_bsp  (not a Ghidra function; structure bsp activate slot 2)
// address 0x42ce90, size 136 bytes
// name confidence: 0.4  rewrite confidence: 0.85
// evidence: structure_bsp_activate_procedures[2] (0x0069e8e4) holds 0x42ce90; its deactivate partner
//   ai_reset_fire_group_assignments 0x42c940 sits in deactivate slot 3.
// objdump 0x42ce90..0x42cf17: walks the unassigned actor list (ai_globals +0x08, chained through actor
//   +0x2c; the next link is read before anything changes). An actor whose encounter (+0x30) is set and
//   whose scenario encounter (scenario +0x430, 0xb0 each) has +0x7e equal to global_structure_bsp_index is
//   taken off the list (ai_actor_unlink_from_unassigned_list 0x436990, EDI actor) and added to it
//   (encounter_add_actor 0x436770: DX squad +0x38, stack actor, encounter, 1).
// blam-cc: no arguments

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern ai_globals *ai_globals_ptr;          // 0x00880354
extern data_array *actor_data;              // 0x00880360
extern int16_t global_structure_bsp_index;  // 0x0069e8d8
extern Scenario *global_scenario;


void ai_unassigned_actors_attach_to_structure_bsp(void)
{
    int16_t bsp_index = global_structure_bsp_index;
    datum_index actor_index = ai_globals_ptr->unknown_08;

    while (actor_index != k_datum_index_none) {
        actor *entry = &((actor *)actor_data->data)[actor_index & 0xffff];
        datum_index encounter_index = entry->unknown_30;
        datum_index next = entry->next_in_encounter;

        if (encounter_index != k_datum_index_none &&
            *(int16_t *)((uint8_t *)global_scenario->encounters.pointer + (encounter_index & 0xffff) * 0xb0 + 0x7e) ==
                bsp_index) {
            ai_actor_unlink_from_unassigned_list(actor_index);
            encounter_add_actor(entry->unknown_38, actor_index, entry->unknown_30, 1);
        }
        actor_index = next;
    }
}
