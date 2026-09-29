// actor_issue_multi_target_vocalization  (Ghidra: actor_issue_multi_target_vocalization; named for this rewrite)
// address 0x4303a0, size 69 bytes
// name confidence: 0.3   rewrite confidence: 0.25
// evidence: phase-4 summary ("issues a simple AI order with no explicit target once basic
// register-context validity checks pass"); called from ai_dispatch_queued_order.c's
// multi-target (target_count == 2) branch.
// register convention: EDI -> actor_index, BX -> variant, ESI -> vehicle_object_index, stack
// -> line (all but line are unresolved registers in Ghidra's own decompile).
// blam-cc: EDI -> actor_index, BX -> variant, ESI -> vehicle_object_index, stack -> line
//
// UNSURE: actor_begin_vocalization's context argument is not visibly built at this call
// site at all (unlike the near-identical actor_issue_order_or_vocalize.c); passed a
// zeroed context here since no source for one is apparent.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include "units.h"
#include "fn_ai.h"

extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0
extern uint8_t actor_begin_vocalization(datum_index actor_index, int16_t line, int16_t variant,
                                        actor_vocalization_context *context); // 0x4142d0

// blam-cc: EDI -> actor_index, BX -> variant, ESI -> vehicle_object_index, stack -> line
void actor_issue_multi_target_vocalization(int16_t line, datum_index actor_index, int16_t variant,
                                            datum_index vehicle_object_index)
{
    void *obj;
    actor_vocalization_context context;

    if (actor_index != (datum_index)k_datum_index_none && 0 < variant &&
        vehicle_object_index != (datum_index)k_datum_index_none) {
        obj = object_try_and_get(vehicle_object_index, -1);
        if (obj != 0) {
            context.kind = 0;
            context.unknown_02 = 0;
            context.handle = (datum_index)k_datum_index_none;
            context.unknown_08 = 0;
            context.unknown_0c = 0;
            actor_begin_vocalization(actor_index, line, variant, &context);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4303a0):

void FUN_004303a0(undefined4 param_1)

{
  int iVar1;
  short unaff_BX;
  int unaff_ESI;
  int unaff_EDI;

  if (((unaff_EDI != -1) && (0 < unaff_BX)) && (unaff_ESI != -1)) {
    iVar1 = object_try_and_get(0xffffffff);
    if (iVar1 != 0) {
      FUN_004142d0(param_1);
    }
  }
  return;
}
#endif
