// path_find_context_init  (Ghidra: path_find_context_init, already named)
// address 0x43a700, size 44 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/ai.h path_find_context (0x1008c bytes = 0x4023 dwords, confirmed by this
//   function's own zero loop); path_find_request (0x48 bytes = 0x12 dwords, confirmed by the
//   copy loop here); (uint32_t)global_structure_bsp global and path_find_context.structure_bsp (+0x64 =
//   dword index 0x19) and unknown_48 (+0x48 = dword index 0x12), both already named.
// register convention: EDX -> context; stack -> request, second_param.
//   // blam-cc: EDX -> context, stack -> request, second_param
// reconciled: R06 0x00746f9c is ScenarioStructureBSP *global_structure_bsp (was extern int32_t bsp_generation); ai.h path_find_context/actor_movement_context bsp_generation -> structure_bsp, bsp_index -> collision_bsp

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern ScenarioStructureBSP *global_structure_bsp; // 0x00746f9c, scenario.h (formerly bsp_generation)

// blam-cc: EDX -> context, stack -> request, second_param
void path_find_context_init(path_find_context *context, const path_find_request *request, uint32_t second_param)
{
    uint32_t *clear;
    int32_t i;
    const uint32_t *src;
    uint32_t *dst;

    clear = (uint32_t *)context;
    for (i = 0x4023; i != 0; i = i - 1) {
        *clear = 0;
        clear = clear + 1;
    }

    context->structure_bsp = (uint32_t)global_structure_bsp;

    src = (const uint32_t *)request;
    dst = (uint32_t *)context;
    for (i = 0x12; i != 0; i = i - 1) {
        *dst = *src;
        src = src + 1;
        dst = dst + 1;
    }

    context->unknown_48 = second_param;
}

#if 0
// ---- original Ghidra decompilation (path_find_context_init @ 0x43a700) ----
void path_find_context_init(undefined4 *param_1,undefined4 param_2)

{
  int iVar1;
  undefined4 *in_EDX;
  undefined4 *puVar2;

  puVar2 = in_EDX;
  for (iVar1 = 0x4023; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = 0;
    puVar2 = puVar2 + 1;
  }
  in_EDX[0x19] = DAT_00746f9c;
  puVar2 = in_EDX;
  for (iVar1 = 0x12; iVar1 != 0; iVar1 = iVar1 + -1) {
    *puVar2 = *param_1;
    param_1 = param_1 + 1;
    puVar2 = puVar2 + 1;
  }
  in_EDX[0x12] = param_2;
  return;
}
#endif
