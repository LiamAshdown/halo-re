// actor_reassign_vehicle_seat  (Ghidra: actor_reassign_vehicle_seat; named for this rewrite)
// address 0x42b880, size 179 bytes
// name confidence: 0.4   rewrite confidence: 0.5
// evidence: phase-4 summary ("reassigns an actor's vehicle seat when it differs from the
// vehicle's currently assigned seat, then runs follow-up squad/vehicle bookkeeping").
// Ghidra dropped every register argument (EBX, EDI, and object_try_and_get's ECX) and
// mis-showed the ai_communication_broadcast/squad_clear_unit_references calls with far
// fewer arguments than they take; fully re-derived from the real disassembly (objdump -d
// -M intel --start-address=0x42b880 --stop-address=0x42b933 bin/halo.exe).
// squad_clear_unit_references (0x430d30) is renamed here to
// ai_conversation_clear_object_references per out/phase4/ai_types_notes.md's "Misattributed
// functions" table (the 0x430830..0x431e70 block is the AI conversation system, not squads).
// register convention: EBX -> vehicle_object_index, EDI -> self_object_index; stack ->
// seat_selector.
// blam-cc: EBX -> vehicle_object_index, EDI -> self_object_index, stack -> seat_selector
//
// UNSURE: seat_selector (Ghidra's param_1, compared against the literal 9 to decide whether
// to check the gunner seat at all) is ALSO forwarded verbatim as ai_communication_broadcast's
// object_b argument -- it may actually be a datum_index the caller reuses for both purposes,
// not a small enum. UNSURE: object+0xb8 is read here as a team index for teams_are_enemies,
// which conflicts with types/objects.h's name_index at the same offset (a conflict this
// project's devices module review already flagged). UNSURE: encounters_note_hostile_object is genuinely
// tail-called with EAX/ESI/EBX left exactly as this function computed them; represented here
// as an explicit 3-argument call since C cannot express an open tail-call passthrough.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "ai.h"

extern data_array *object_data; // 0x008603b0
extern void *object_try_and_get(datum_index object_index, int32_t kind); // 0x4f6ec0, blam-cc: ECX -> object_index, stack -> kind
extern int8_t teams_are_enemies(int16_t team_a, int16_t team_b); // 0x45bd50, teams_are_enemies; blam-cc: CX -> team_a, DX -> team_b
extern void ai_communication_broadcast(int32_t event_code, datum_index unit_index, datum_index object_a, int32_t reason, datum_index object_b, datum_index object_c, uint32_t *extra_data); // 0x42d340
extern void ai_conversation_clear_object_references(datum_index object_index, uint8_t force_full_scan); // 0x430d30
extern void encounters_note_hostile_object(datum_index object_index); // 0x435f90, EAX (0x42b92b: EAX = self_object_index, tail jump)

// blam-cc: EBX -> vehicle_object_index, EDI -> self_object_index, stack -> seat_selector
// Resolves which occupant (if any) currently sits in vehicle_object_index's gunner seat, or
// its driver seat when the gunner seat is empty or seat_selector requests the driver seat
// directly (== 9), falling back to vehicle_object_index itself when both seats are empty.
// If that occupant differs from self_object_index, checks whether their teams are hostile
// (2 = not hostile, 3 = hostile) and broadcasts a communication event with the result (1
// when the occupant already IS self_object_index, 0 when there is no occupant to compare).
// Then clears self_object_index's AI-conversation references and runs the vehicle's
// follow-up bookkeeping.
int32_t actor_reassign_vehicle_seat(datum_index vehicle_object_index, datum_index self_object_index,
                                     int32_t seat_selector)
{
    datum_index occupant;
    object *vehicle_obj;
    unit_data *vehicle_unit;
    int32_t reason;

    occupant = (datum_index)k_datum_index_none;
    if (vehicle_object_index != (datum_index)k_datum_index_none) {
        vehicle_obj = (object *)object_try_and_get(vehicle_object_index, 3);
        if (vehicle_obj != 0) {
            vehicle_unit = (unit_data *)((uint8_t *)vehicle_obj + k_unit_data_offset);
            occupant = (datum_index)k_datum_index_none;
            if (seat_selector != 9) {
                occupant = vehicle_unit->gunner_unit_index;
            }
            if (seat_selector == 9 || occupant == (datum_index)k_datum_index_none) {
                occupant = vehicle_unit->driver_unit_index;
                if (occupant == (datum_index)k_datum_index_none) {
                    occupant = vehicle_object_index;
                }
            }
        }
    }

    if (self_object_index == occupant) {
        reason = 1;
    } else if (occupant == (datum_index)k_datum_index_none) {
        reason = 0;
    } else {
        object *occupant_obj = ((object_header *)object_data->data)[occupant & 0xffff].data;
        object *self_obj = ((object_header *)object_data->data)[self_object_index & 0xffff].data;
        reason = teams_are_enemies(((struct object *)occupant_obj)->owner_team,
                               ((struct object *)self_obj)->owner_team) ? 3 : 2;
    }

    ai_communication_broadcast(0, self_object_index, occupant, reason, seat_selector,
                                (datum_index)k_datum_index_none, 0);
    ai_conversation_clear_object_references(self_object_index, 0);
    encounters_note_hostile_object(self_object_index);
    return 0;
}

#if 0
Original Ghidra decompilation (0x42b880):

void FUN_0042b880(short param_1)

{
  int iVar1;
  int unaff_EBX;
  int iVar2;
  int unaff_EDI;

  iVar2 = -1;
  if ((((unaff_EBX != -1) && (iVar1 = object_try_and_get(3), iVar1 != 0)) &&
      ((param_1 == 9 || (iVar2 = *(int *)(iVar1 + 0x328), *(int *)(iVar1 + 0x328) == -1)))) &&
     (iVar2 = *(int *)(iVar1 + 0x324), *(int *)(iVar1 + 0x324) == -1)) {
    iVar2 = unaff_EBX;
  }
  if ((unaff_EDI != iVar2) && (iVar2 != -1)) {
    FUN_0045bd50();
  }
  ai_communication_broadcast(0);
  squad_clear_unit_references(unaff_EDI,'\0');
  FUN_00435f90();
  return;
}

Real disassembly (0x42b880-0x42b932), used because Ghidra dropped EBX/EDI/ECX entirely and
undercounted both call sites arguments:

0042b880: push   ebp
0042b881: mov    ebp,[esp+0x8]        ; ebp = seat_selector
0042b885: push   esi
0042b886: or     esi,0xffffffff       ; occupant = -1
0042b889: cmp    ebx,0xffffffff       ; ebx = vehicle_object_index
0042b88c: je     0x42b8be
0042b88e: push   0x3
0042b890: mov    ecx,ebx
0042b892: call   0x4f6ec0             ; object_try_and_get(ebx, 3)
0042b89a: test   eax,eax
0042b89c: je     0x42b8be
0042b89e: cmp    bp,0x9
0042b8a2: je     0x42b8af
0042b8a4: mov    esi,[eax+0x328]      ; occupant = gunner_unit_index
0042b8aa: cmp    esi,0xffffffff
0042b8ad: jne    0x42b8be
0042b8af: mov    eax,[eax+0x324]      ; driver_unit_index
0042b8b5: cmp    eax,0xffffffff
0042b8b8: mov    esi,eax
0042b8ba: jne    0x42b8be
0042b8bc: mov    esi,ebx              ; occupant = vehicle_object_index
0042b8be: xor    eax,eax
0042b8c0: cmp    edi,esi              ; edi = self_object_index
0042b8c2: jne    0x42b8cb
0042b8c4: mov    eax,0x1
0042b8c9: jmp    0x42b910
0042b8cb: cmp    esi,0xffffffff
0042b8ce: je     0x42b910
0042b8d0: ...                         ; look up object+0xb8 for esi and edi
0042b8f4: mov    cx,[edx+0xb8]
0042b8fb: mov    dx,[eax+0xb8]
0042b902: call   0x45bd50             ; teams_are_enemies(cx, dx)
0042b907: neg al / sbb eax,eax / neg eax / add eax,0x2   ; reason = enemies ? 3 : 2
0042b910: push   0x0                  ; param_g
0042b912: push   0xffffffff           ; object_c
0042b914: push   ebp                  ; object_b = seat_selector
0042b915: push   eax                  ; param_d = reason
0042b916: push   esi                  ; object_a = occupant
0042b917: push   edi                  ; unit_index = self_object_index
0042b918: push   0x0                  ; event_code
0042b91a: call   0x42d340             ; ai_communication_broadcast
0042b91f: push   0x0
0042b921: push   edi
0042b922: call   0x430d30             ; ai_conversation_clear_object_references(edi, 0)
0042b927: add    esp,0x24
0042b92a: pop    esi
0042b92b: mov    eax,edi
0042b92d: pop    ebp
0042b92e: jmp    0x435f90             ; tail call, EAX=edi, ESI=occupant, EBX=vehicle_object_index
#endif
