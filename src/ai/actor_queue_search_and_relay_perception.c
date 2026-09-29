// actor_queue_search_and_relay_perception  (Ghidra: actor_queue_search_and_relay_perception, renamed)
// address 0x4221f0, size 121 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: types/ai.h prop.unknown_e0, prop.owner_actor_index (0x1c), actor.unknown_74
//   (0x74); calls actor_queue_search_position (0x421af0) exactly as
//   actor_queue_velocity_search_from_prop @0x4221b0 does but with priority 1, then, if the
//   prop's owning actor exists and has a positive unknown_74 (the running perception-event
//   priority actor_update_awareness_level also reads), relays that value as a new perception
//   event on this actor via actor_record_perception_event (0x422070, already rewritten) with a
//   fixed data payload of 0x1c2. Ghidra's own pseudocode under-renders both calls; the
//   parameters below were confirmed with objdump against bin/halo.exe.
// register convention: EAX -> prop_index, EBX -> actor_index (both unaff_, passed through from
//   this function's own caller).
//   // blam-cc: EAX -> prop_index, EBX -> actor_index

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

extern data_array *prop_data;  // 0x008802c0
extern data_array *actor_data; // 0x00880360

extern void actor_queue_search_position(datum_index actor_index, real_point3d *position, int16_t priority,
                                        real_vector3d *velocity, uint32_t unknown_324, uint32_t unknown_328,
                                        uint32_t unknown_33c, uint32_t unknown_340, uint32_t unknown_344,
                                        uint8_t unknown_348); // 0x421af0
extern void actor_record_perception_event(datum_index actor_index, int16_t event, int32_t data); // 0x422070

// blam-cc: EAX -> prop_index, EBX -> actor_index
// Queues a priority-1 velocity-only search request from the prop, as
// actor_queue_velocity_search_from_prop does at priority 6, then forwards the prop's owning
// actor's running perception priority (if positive) as a new perception event on this actor.
void actor_queue_search_and_relay_perception(datum_index prop_index, datum_index actor_index)
{
    prop *p = &((prop *)prop_data->data)[prop_index & 0xffff];
    datum_index owner_index;

    actor_queue_search_position(actor_index, 0, 1, (real_vector3d *)&p->direction,
                                0xffffffff, 0, 90, prop_index, 150, 0);

    owner_index = p->owner_actor_index;
    if (owner_index != (datum_index)k_datum_index_none) {
        actor *owner = &((actor *)actor_data->data)[owner_index & 0xffff];
        if (owner->suspicion_status > 0) {
            actor_record_perception_event(actor_index, owner->suspicion_status, 0x1c2);
        }
    }
}

#if 0
Original Ghidra decompilation (0x4221f0):

void FUN_004221f0(void)

{
  int iVar1;
  uint uVar2;
  uint in_EAX;

  iVar1 = *(int *)(DAT_008802c0 + 0x34);
  FUN_00421af0(0xffffffff,0,0x5a);
  uVar2 = *(uint *)((in_EAX & 0xffff) * 0x138 + iVar1 + 0x1c);
  if ((uVar2 != 0xffffffff) &&
     (0 < *(short *)((uVar2 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34) + 0x74))) {
    FUN_00422070();
  }
  return;
}

Ground truth from objdump (bin/halo.exe @ 0x4221f0..0x422268), since the pseudocode above
drops the incoming EAX/EBX register arguments and every argument of both calls:

  4221f0: mov ecx, ds:0x8802c0        ; prop_data
  4221f6: mov edx, [ecx+0x34]         ; prop_data->data
  4221f9: push esi / push edi         ; (callee-save)
  4221fb: mov edi, eax                ; edi = prop_index (EAX on entry)
  422203: push 0x0                    ; -> unknown_348
  422205: imul edi, edi, 0x138
  42220b: push 0x96                   ; -> unknown_344 (150)
  422210: push eax                    ; -> unknown_340 (raw prop handle)
  422211: push 0x5a                   ; -> unknown_33c (90)
  422213: add edi, edx                ; edi = &prop[index]
  422215: push 0x0                    ; -> unknown_328
  422217: lea esi, [edi+0xe0]         ; esi = &prop.unknown_e0
  42221d: push 0xffffffff             ; -> unknown_324
  42221f: xor ecx, ecx                ; position = NULL
  422221: mov edx, 1                  ; priority = 1
  422226: mov eax, ebx                ; eax = actor_index
  422228: call actor_queue_search_position
  42222d: mov edi, [edi+0x1c]         ; edi = prop.owner_actor_index
  422233: cmp edi, 0xffffffff
  422236: je 0x422266
  422238: mov edx, ds:0x880360        ; actor_data
  42223e: mov ecx, [edx+0x34]
  422247: imul edi, edi, 0x724
  42224d: add edi, ecx                ; edi = &actor[owner]
  42224f: xor edx, edx
  422251: mov dx, [edi+0x74]          ; edx = owner.unknown_74 (zero-extended)
  422255: test dx, dx
  422258: jle 0x422266
  42225a: mov esi, 0x1c2              ; data = 0x1c2
  42225f: mov eax, ebx                ; eax = actor_index
  422261: call actor_record_perception_event   ; EAX=actor_index, EDX=event, ESI=data
#endif
