// actor_queue_velocity_search_from_prop  (Ghidra: actor_queue_velocity_search_from_prop, renamed)
// address 0x4221b0, size 63 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: types/ai.h prop.unknown_e0 (a real_point3d-sized scratch field, untyped beyond
//   that); calls actor_queue_search_position (0x421af0, already rewritten in this module) with
//   position = NULL and velocity = &prop.unknown_e0, priority 6, duration fields 90/150/0 and
//   the raw prop handle. Ghidra's own pseudocode ("actor_queue_velocity_search_from_prop(0xffffffff,0,0x5a)") drops
//   every register argument and mis-renders the stack arguments; the parameters and exact
//   values below were read out of bin/halo.exe with objdump. The sole caller
//   (src/ai/actor_target_relationship_think.c, `actor_queue_velocity_search_from_prop(actor_index)` around its
//   `LAB_0041b721`) loads the prop index into EAX and the actor index into [esp+0xc4] a few
//   instructions earlier and pushes the latter right before the call, matching exactly.
//   UNSURE: that caller's own extern for this function only declares the stack argument and
//   silently relies on EAX already holding the prop index from its own preceding code; that
//   file is outside this rewrite's range and was not corrected here.
// register convention: EAX -> prop_index, stack -> actor_index.
//   // blam-cc: EAX -> prop_index, stack -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data; // 0x008802c0

extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0

// blam-cc: EAX -> prop_index, stack -> actor_index
// Queues a priority-6 search request for the actor carrying no explicit position but a
// velocity vector taken from the prop's scratch field, a 90-tick duration, a 150 secondary
// duration, and the raw prop handle threaded through unknown_340.
void actor_queue_velocity_search_from_prop(datum_index prop_index, datum_index actor_index)
{
    prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];

    actor_queue_search_position(actor_index, 0, 6, (real_vector3d *)&p->unknown_e0,
                                0xffffffff, 0, 90, prop_index, 150, 0);
}

#if 0
Original Ghidra decompilation (0x4221b0):

void FUN_004221b0(void)

{
  FUN_00421af0(0xffffffff,0,0x5a);
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x4221b0..0x4221ee), since the pseudocode above
drops the incoming EAX register argument, the incoming stack argument, and five of the six
real stack arguments to FUN_00421af0:

  4221b0: mov ecx, ds:0x8802c0        ; prop_data
  4221b6: mov edx, [ecx+0x34]         ; prop_data->data
  4221b9: push esi                    ; (callee-save)
  4221ba: push 0x0                    ; -> unknown_348
  4221bc: push 0x96                   ; -> unknown_344 (150)
  4221c1: push eax                    ; -> unknown_340 (raw prop handle, EAX on entry)
  4221c2: and eax, 0xffff
  4221c7: imul eax, eax, 0x138
  4221cd: push 0x5a                   ; -> unknown_33c (90)
  4221cf: lea esi, [eax+edx+0xe0]     ; esi = &prop.unknown_e0
  4221d6: mov eax, [esp+0x18]         ; reloads this function's OWN stack argument (actor_index)
  4221da: push 0x0                    ; -> unknown_328
  4221dc: push 0xffffffff             ; -> unknown_324
  4221de: xor ecx, ecx                ; position = NULL
  4221e0: mov edx, 6                  ; priority = 6
  4221e5: call actor_queue_search_position   ; EAX = actor_index, ESI = &prop.unknown_e0
#endif
