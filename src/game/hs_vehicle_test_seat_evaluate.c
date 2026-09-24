// hs_vehicle_test_seat_evaluate  (no Ghidra function; Ghidra only created the mid-body fragment
//   0x47bf23 "player_handle_action_jmp_table_adjust_size")
// address 0x47bef0, size 95 bytes (0x47bef0..0x47bf4e: `ret` at 0x47bf4e, tail `jmp` to
//   hs_thread_return at 0x47bf47)
// name confidence: 0.75   rewrite confidence: 0.85
// evidence: the only reference is data: 0x00658394 is the +0x0c `evaluate` slot of the
//   hs_function_definition at 0x00658388 (types/hs.h), whose name is "vehicle_test_seat", return
//   type 5 (boolean) and parameters (39, 9, 38) = (vehicle, string, unit). The body (objdump
//   0x47bef0..0x47bf4f) is the standard builtin evaluator: definition = hs_function_definitions
//   [index] (0x00688b58), arguments via hs_evaluate_typed_arguments (0x48a850, cdecl
//   (thread_index, parameter_count, parameters, first)), then
//   unit_is_child_seated_at_named_marker (0x56b520: stack (vehicle, seat label), EBX = unit,
//   `mov ebx,[eax+0x8]` at 0x47bf2b) and the result, zero-extended from AL through a zeroed
//   dword slot, handed to hs_thread_return (0x48a640, EAX = value, ECX = thread_index) by a tail
//   jump.
//   0x47bf23 (the `je` on a NULL argument block) was recorded by modules.json as a function of its
//   own; it is covered by this file (see out/phase4/orphans_notes.md).
// register convention: the hs_function_definition::evaluate shape, all on the stack.
//   // blam-cc: stack -> (function_index, thread_index, first)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"
#include "game.h"
#include <stdint.h> // uintptr_t: the string argument is a 32-bit pointer in the argument block

extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640, blam-cc: EAX value, ECX thread_index
extern uint8_t unit_is_child_seated_at_named_marker(datum_index unit_index, const char *seat_label,
    datum_index child_object_index); // 0x56b520, blam-cc: stack (unit_index, seat_label), EBX child_object_index

// (vehicle_test_seat <vehicle> <string> <unit>): whether the unit sits in the vehicle's seat with
// that label.
void hs_vehicle_test_seat_evaluate(int16_t function_index, uint32_t thread_index, char first)
{
    hs_function_definition *definition = hs_function_definitions[function_index];
    int32_t *arguments = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);

    if (arguments != 0) {
        uint8_t seated = unit_is_child_seated_at_named_marker((datum_index)arguments[0],
            (const char *)(uintptr_t)(uint32_t)arguments[1], (datum_index)arguments[2]);
        hs_thread_return((int32_t)seated, thread_index);
    }
}

#if 0
No Ghidra decompilation of 0x47bef0 exists (Ghidra never created a function there). The fragment
Ghidra did create, 0x47bf23 player_handle_action_jmp_table_adjust_size:

void player_handle_action_jmp_table_adjust_size(void)

{
  undefined4 *in_EAX;
  bool in_ZF;

  if (!in_ZF) {
    unit_is_child_seated_at_named_marker(*in_EAX,in_EAX[1]);
    hs_thread_return();
    return;
  }
  return;
}
#endif
