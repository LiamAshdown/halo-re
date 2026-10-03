// hs_vehicle_gunner_evaluate  (no Ghidra function; Ghidra only created the mid-body fragment
//   0x47c3d0 "player_health_pack_screen_effect")
// address 0x47c3a0, size 81 bytes (0x47c3a0..0x47c3f0: `ret` at 0x47c3f0, tail `jmp` to
//   hs_thread_return at 0x47c3ea)
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: the only reference is data: 0x00658568 is the +0x0c `evaluate` slot of the
//   hs_function_definition at 0x0065855c (types/hs.h), whose name is "vehicle_gunner", return
//   type 38 (unit) and one parameter, 38 (unit). objdump 0x47c3a0..0x47c3f1: the standard builtin
//   evaluator (definition = hs_function_definitions[index], hs_evaluate_typed_arguments), then
//   object_try_and_get(ECX = argument 0, stack mask 3 = biped | vehicle) and the unit's gunner
//   handle at object + 0x328 (types/units.h unit_data.gunner_unit_index), or -1 when the argument
//   is not a unit, returned through hs_thread_return (EAX = value, ECX = thread_index) by a tail
//   jump.
//   0x47c3d0 (the `call 0x4f6ec0`) was recorded by modules.json as a function of its own; it is
//   covered by this file (see out/phase4/orphans_notes.md).
// register convention: the hs_function_definition::evaluate shape, all on the stack.
//   // blam-cc: stack -> (function_index, thread_index, first)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "hs.h"
#include "game.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread_index
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, blam-cc: ECX object_index

// (vehicle_gunner <unit>): the unit's gunner, or none.
void hs_vehicle_gunner_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        object *unit_obj = object_try_and_get((datum_index)arguments[0], 3);
        datum_index gunner = (datum_index)0xffffffff;

        if (unit_obj != 0) {
            gunner = ((unit_data *)((uint8_t *)unit_obj + k_unit_data_offset))->gunner_unit_index;
        }
        hs_thread_return((int32_t)gunner, thread_index);
    }
}

#if 0
No Ghidra decompilation of 0x47c3a0 exists (Ghidra never created a function there). The fragment
Ghidra did create, 0x47c3d0 player_health_pack_screen_effect:

void player_health_pack_screen_effect(void)

{
  object_try_and_get();
  hs_thread_return();
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
