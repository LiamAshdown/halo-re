// actor_start_search_timer  (Ghidra: actor_start_search_timer, renamed per out/phase2 hint "actor_start_search_timer")
// address 0x422130, size 115 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: types/ai.h prop.ground_position (0xf0), prop.path_surface_index (0xec),
//   actor.unknown_3a0 (a datum_index, actor_new sets none); calls
//   actor_target_get_relationship_object (0x41f3a0, already rewritten in this module) and
//   actor_queue_search_position (0x421af0, already rewritten). Ghidra's own pseudocode for
//   this function shows no visible call arguments at all (everything arrives via unaff_EBX /
//   unaff_EDI and hidden register reads); the parameters below and the exact values forwarded
//   into actor_queue_search_position were read out of bin/halo.exe directly with objdump
//   because the decompiled call sites could not be trusted (confirmed against both callers of
//   this function inside src/ai/actor_target_relationship_think.c, which independently loads
//   the same prop-index/actor-index pair from [esp+0x2c] / [esp+0xc4] into EDI/EBX right
//   before each call).
// register convention: EBX -> actor_index, EDI -> prop_index (both unaff_, i.e. passed through
//   unchanged from this function's own caller).
//   // blam-cc: EBX -> actor_index, EDI -> prop_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"
#include "fn_ai.h"

extern data_array *prop_data;  // 0x008802c0
extern data_array *actor_data; // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c


// blam-cc: EBX -> actor_index, EDI -> prop_index
// Marks the start of an investigation timer: stamps the current tick on the actor (a scratch
// field also read elsewhere), resolves the prop's relationship/obstruction object, and queues
// a priority-2, 90-tick search position at the prop's ground position (with its path surface
// index and a 1.25 radius/weight scalar carried along, plus the raw prop handle and a 1-flag
// tail byte).
void actor_start_search_timer(datum_index actor_index, datum_index prop_index)
{
    prop *target = &((prop *)prop_data->data)[prop_index & 0xffff];
    actor *self = &((actor *)actor_data->data)[actor_index & 0xffff];

    // UNSURE: types/ai.h types actor.unknown_3a0 as a datum_index (actor_new sets it to
    // "none"), but this store is plainly the current game tick, not a handle; datum_index is
    // a plain uint32_t so the store is bit-compatible either way.
    self->unknown_3a0 = (uint32_t)game_time->game_time;

    actor_target_get_relationship_object(prop_index);

    actor_queue_search_position(actor_index, &target->ground_position, 2, 0,
                                target->path_surface_index, 0x3fc00000 /* 1.25f */, 90,
                                prop_index, 90, 1);
}

#if 0
Original Ghidra decompilation (0x422130):

void FUN_00422130(void)

{
  int iVar1;
  uint unaff_EBX;
  uint unaff_EDI;

  iVar1 = *(int *)(DAT_008802c0 + 0x34);
  *(undefined4 *)((unaff_EBX & 0xffff) * 0x724 + 0x3a0 + *(int *)(DAT_00880360 + 0x34)) =
       *(undefined4 *)(DAT_006f1d6c + 0xc);
  actor_target_get_relationship_object();
  FUN_00421af0(*(undefined4 *)((unaff_EDI & 0xffff) * 0x138 + iVar1 + 0xec),0x3fc00000,0x5a);
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x422130..0x42219e), since the pseudocode above
drops every register argument and under-counts FUN_00421af0's stack arguments:

  422130: mov eax, ds:0x8802c0        ; prop_data
  422135: mov ecx, [eax+0x34]         ; prop_data->data
  422138: mov edx, ds:0x880360        ; actor_data
  42213e: mov eax, [edx+0x34]         ; actor_data->data
  422141: push esi
  422142: mov edx, ds:0x6f1d6c
  422148: mov edx, [edx+0xc]          ; current tick
  42214b: mov esi, edi
  42214d: and esi, 0xffff
  422153: imul esi, esi, 0x138
  422159: add esi, ecx                ; esi = &prop[edi]
  42215b: mov ecx, ebx
  42215d: and ecx, 0xffff
  422163: imul ecx, ecx, 0x724
  422169: mov [ecx+eax+0x3a0], edx    ; actor[ebx].unknown_3a0 = tick
  422170: mov eax, edi
  422172: call actor_target_get_relationship_object   ; EAX = prop_index
  422177: mov eax, [esi+0xec]         ; prop.path_surface_index
  42217d: push 1                      ; -> unknown_348
  42217f: push 0x5a                   ; -> unknown_344 (90)
  422181: push edi                    ; -> unknown_340 (raw prop handle)
  422182: push 0x5a                   ; -> unknown_33c (90)
  422184: push 0x3fc00000             ; -> unknown_328 (1.25f)
  422189: push eax                    ; -> unknown_324 (prop.path_surface_index)
  42218a: lea ecx, [esi+0xf0]         ; position = &prop.ground_position
  422190: xor esi, esi                ; velocity = NULL
  422192: mov edx, 2                  ; priority = 2
  422197: mov eax, ebx                ; actor_index
  422199: call actor_queue_search_position
#endif
