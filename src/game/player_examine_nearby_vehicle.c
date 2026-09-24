// player_examine_nearby_vehicle  (Ghidra: player_examine_nearby_vehicle, already named --
// CEA/PDB-adjacent confidence per out/phase4/game_functions.md)
// address 0x47b140, size 100 bytes
// name confidence: 0.7   rewrite confidence: 0.45
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x47b140
//   --stop-address=0x47b1a4): the call `hs_evaluate_typed_arguments(esi, eax, edx, ecx)` with
//   esi/eax/edx pushed from this function's own [esp+0xc]/[eax+0x1a]/[eax+0x1c] and ecx pushed
//   verbatim from this function's OWN incoming ECX register -- matching
//   src/hs/hs_evaluate_typed_arguments.c's established (thread_index, parameter_count,
//   expected_types, first) signature exactly once EAX is recognized as the calling
//   hs_function_definition* (types/hs.h; parameter_count at +0x1a, parameters at +0x1c,
//   matching hs_function_definition::evaluate's own documented shape). types/objects.h
//   object::vitality_flags (0x106/0x107).
// register convention: the hs_function_definition* dispatching this evaluate call in EAX
//   (in_EAX), the "first call" flag in ECX (in_ECX); `index` and `thread_index` are this
//   function's own two stack parameters (`index` is never read).
//   // blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
// UNSURE: object::vitality_flags bit 0x0100 (byte 0x107 bit 0x01)'s meaning in this context --
//   types/objects.h derives that same bit's name from an unrelated region-damage function, so
//   it is not reused here; kept as a raw literal.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "hs.h"
#include "game.h"

extern data_array *object_headers; // 0x008603b0

extern int32_t *hs_evaluate_typed_arguments(uint32_t thread_index, int16_t parameter_count,
    int16_t *expected_types, char first); // 0x48a850
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640

// blam-cc: EAX -> definition, ECX -> first, stack -> index, thread_index
// Evaluates this builtin's one (object, boolean) argument pair; once both are ready, and unless
// the object argument is -1, sets or clears object::vitality_flags bit 0x0100 on it according to
// the boolean, then returns from the HS thread (result unused, i.e. void).
void player_examine_nearby_vehicle(int16_t index, uint32_t thread_index, hs_function_definition *definition,
    char first)
{
    int32_t *args = hs_evaluate_typed_arguments(thread_index, definition->parameter_count,
        (int16_t *)definition->parameters, first);
    (void)index;

    if (args == 0) {
        return;
    }
    if (args[0] != (int32_t)0xffffffff) {
        object *target = (object *)((object_header *)object_headers->data)[args[0] & 0xffff].data;
        if ((char)args[1] != 0) {
            *((uint8_t *)&target->vitality_flags + 1) |= 0x01;
            hs_thread_return(0, thread_index);
            return;
        }
        *((uint8_t *)&target->vitality_flags + 1) &= 0xfe;
    }
    hs_thread_return(0, thread_index);
}

#if 0
Original Ghidra decompilation (0x47b140), from tools/pack.py 0x47b140:

void player_examine_nearby_vehicle(undefined4 param_1,undefined4 param_2)

{
  byte *pbVar1;
  int iVar2;
  int in_EAX;
  uint *puVar3;

  puVar3 = (uint *)hs_evaluate_typed_arguments(param_2,(int)*(short *)(in_EAX + 0x1a),in_EAX + 0x1c)
  ;
  if (puVar3 == (uint *)0x0) {
    return;
  }
  if (*puVar3 != 0xffffffff) {
    iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*puVar3 & 0xffff) * 0xc);
    if ((char)puVar3[1] != '\0') {
      pbVar1 = (byte *)(iVar2 + 0x107);
      *pbVar1 = *pbVar1 | 1;
      hs_thread_return();
      return;
    }
    pbVar1 = (byte *)(iVar2 + 0x107);
    *pbVar1 = *pbVar1 & 0xfe;
  }
  hs_thread_return();
  return;
}
#endif
