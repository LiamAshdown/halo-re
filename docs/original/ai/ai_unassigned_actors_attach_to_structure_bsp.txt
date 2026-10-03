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

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern ai_globals *ai_globals_ptr;          // 0x00880354
extern data_array *actor_data;              // 0x00880360
extern int16_t global_structure_bsp_index;  // 0x0069e8d8
extern Scenario *global_scenario;

extern void ai_actor_unlink_from_unassigned_list(datum_index actor_index); // 0x436990, EDI
extern void encounter_add_actor(int16_t squad_index, datum_index actor_index, datum_index encounter_index,
    uint8_t keep_team); // 0x436770, DX, stack

void ai_unassigned_actors_attach_to_structure_bsp(void)
{
    int16_t bsp_index = global_structure_bsp_index;
    datum_index actor_index = ai_globals_ptr->first_encounterless_actor;

    while (actor_index != k_datum_index_none) {
        actor *entry = &((actor *)actor_data->data)[actor_index & 0xffff];
        datum_index encounter_index = entry->original_encounter_index;
        datum_index next = entry->next_in_encounter;

        if (encounter_index != k_datum_index_none &&
            *(int16_t *)((uint8_t *)global_scenario->encounters.pointer + (encounter_index & 0xffff) * 0xb0 + 0x7e) ==
                bsp_index) {
            ai_actor_unlink_from_unassigned_list(actor_index);
            encounter_add_actor(entry->original_squad_index, actor_index, entry->original_encounter_index, 1);
        }
        actor_index = next;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
