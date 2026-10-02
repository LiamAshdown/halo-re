// ai_search_run  (Ghidra: ai_search_run, renamed)
// address 0x43be20, size 105 bytes
// name confidence: 0.4   rewrite confidence: 0.85 (verified against objdump 0x43be20..0x43be88)
// evidence: types/ai.h ai_search_context.result_node(+0x1e)/complete(+0x28)/best_node(+0x20).
// phase-4 summary "drives an AI point search to completion, returning whether a full or
// best-effort path was found." Calls ai_search_context_init @0x43b790 and ai_search_step
// @0x43bcb0 (both this rewrite), forwarding this function's own `context` explicitly since
// Ghidra shows both calls with no visible arguments (pure register forwarding via ESI).
// register convention: ESI -> context, EDX -> unknown_18, ECX -> unknown_29, EAX -> unknown_2a,
//   stack -> unknown_04, obstacles, unknown_00, position, z, origin.
//   // blam-cc: ESI -> context, EDX -> unknown_18, ECX -> unknown_29, EAX -> unknown_2a,
//   //   stack -> unknown_04, obstacles, unknown_00, position, z, origin
//
// FIXED (register inputs, objdump): EAX/ECX/EDX (read at 0x43be20/25/2a, each saved by a push
// before being clobbered) are not scratch -- objdump 0x43be20..0x43be4b shows this function
// marshalling its own EAX/ECX/EDX plus six of its own stack words into the 11-argument call to
// ai_search_context_init (whose real signature is already recovered in
// src/ai/ai_search_context_init.c: ECX -> unknown_0c, EAX -> obstacles, EDX -> origin, stack ->
// context, unknown_04, unknown_00, position, z, unknown_18, unknown_29, unknown_2a). Matching
// push order against that signature: this function's own EAX/ECX/EDX map to unknown_2a/
// unknown_29/unknown_18; its own stack words (by increasing offset) map to unknown_04,
// obstacles, unknown_00, position, z, origin. The callee's ECX argument (unknown_0c) is fed
// directly from the global at 0x746f9c (global_structure_bsp, reconciled R06 elsewhere in this
// module), not from one of this function's own parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void ai_search_context_init(ai_search_context *context, uint8_t unknown_04, uint32_t unknown_00,
    ai_search_obstacle_list *obstacles, real_point2d *origin, uint32_t unknown_0c,
    real_point2d *position, int32_t surface_index, uint32_t unknown_18,
    uint8_t unknown_29, uint8_t unknown_2a); // 0x43b790, see src/ai/ai_search_context_init.c
extern uint8_t ai_search_step(ai_search_context *context); // 0x43bcb0
extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, R06

// blam-cc: ESI -> context, EDX -> unknown_18, ECX -> unknown_29, EAX -> unknown_2a,
//   stack -> unknown_04, obstacles, unknown_00, position, z, origin
uint8_t ai_search_run(ai_search_context *context, uint8_t unknown_04, ai_search_obstacle_list *obstacles,
    uint32_t unknown_00, real_point2d *position, int32_t surface_index, real_point2d *origin,
    uint32_t unknown_18, uint8_t unknown_29, uint8_t unknown_2a)
{
    ai_search_context_init(context, unknown_04, unknown_00, obstacles, origin,
        (uint32_t)global_structure_bsp, position, surface_index, unknown_18, unknown_29, unknown_2a);

    while (ai_search_step(context) != 0) {
        // pop/expand until the open list empties or the goal is reached
    }

    if (context->result_node != -1) {
        context->complete = 1;
        return context->result_node != -1;
    }

    if (context->best_node != -1) {
        context->result_node = context->best_node;
    }
    return context->result_node != -1;
}

#if 0
// ---- original Ghidra decompilation (FUN_0043be20 @ 0x43be20) ----
bool FUN_0043be20(void)

{
  char cVar1;
  int unaff_ESI;

  FUN_0043b790();
  do {
    cVar1 = FUN_0043bcb0();
  } while (cVar1 != '\0');
  if (*(short *)(unaff_ESI + 0x1e) != -1) {
    *(undefined1 *)(unaff_ESI + 0x28) = 1;
    return *(short *)(unaff_ESI + 0x1e) != -1;
  }
  if (*(short *)(unaff_ESI + 0x20) != -1) {
    *(short *)(unaff_ESI + 0x1e) = *(short *)(unaff_ESI + 0x20);
  }
  return *(short *)(unaff_ESI + 0x1e) != -1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
