// ai_search_run  (Ghidra: ai_search_run, renamed)
// address 0x43be20, size 105 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h ai_search_context.result_node(+0x1e)/complete(+0x28)/best_node(+0x20).
// phase-4 summary "drives an AI point search to completion, returning whether a full or
// best-effort path was found." Calls ai_search_context_init @0x43b790 and ai_search_step
// @0x43bcb0 (both this rewrite), forwarding this function's own `context` explicitly since
// Ghidra shows both calls with no visible arguments (pure register forwarding via ESI).
// register convention: ESI -> context (the only `unaff_` register Ghidra's decompile
//   shows).
//   // blam-cc: ESI -> context
//
// UNSURE: ai_search_context_init's own real signature takes many more parameters than are
// visible at this call site; called here with none, matching this module's established
// convention for that situation.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern void ai_search_context_init(void); // 0x43b790, see header UNSURE on the arity mismatch
extern uint8_t ai_search_step(ai_search_context *context); // 0x43bcb0

// blam-cc: ESI -> context
uint8_t ai_search_run(ai_search_context *context)
{
    ai_search_context_init();

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
