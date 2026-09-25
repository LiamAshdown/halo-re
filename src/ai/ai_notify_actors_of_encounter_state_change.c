// ai_notify_actors_of_encounter_state_change  (Ghidra: ai_notify_actors_of_encounter_state_change, already named)
// address 0x42b940, size 304 bytes
// name confidence: 0.8   rewrite confidence: 0.5
// evidence: out/phase4/ai_functions.md signature (already __cdecl with named parameters).
// Ghidra dropped actor_iterator_next's one real argument entirely (a 0x20-byte caller-owned
// filter/state block it builds on the stack); recovered from the real disassembly (objdump
// -d -M intel --start-address=0x42b940 --stop-address=0x42ba80 bin/halo.exe). The block's
// field at +0x0c is the data_array pointer it filters by (encounter_data here) XORed with
// the 4-byte tag 0x69746572 ('iter' read little-endian) -- some kind of signature/self-check
// the iterator apparently validates, not otherwise established in this repo.
// register convention: plain __cdecl, all four arguments on the stack.
// blam-cc: stack -> zone_a, zone_b, status, force_update
//
// UNSURE: when ai_globals_ptr->actors_valid is false, the iterator block is never
// initialized at all, yet actor_iterator_next is still called with it -- reproduced exactly
// (an uninitialized local, matching Ghidra's own local_8 being read unconditionally).
// UNSURE: actor_target_update_active_flag's argument (EAX -> actor_index here) is not confirmed elsewhere;
// other callers in this module also call it with no traced argument.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include <stdint.h>

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern data_array *actor_data;     // 0x00880360
extern data_array *prop_data;      // 0x008802c0

extern actor *actor_iterator_next(actor_iterator_state *iterator); // 0x436a70
extern uint8_t actor_target_update_active_flag(datum_index actor_index, datum_index target_prop_index); // 0x41fc60, EAX actor, EDI prop (0x42b9f3 mov edi,ebp)
extern float actor_rate_potential_target(datum_index actor_index, datum_index target_prop_index); // 0x41fd50

// blam-cc: stack -> zone_a, zone_b, status, force_update
// For every actor whose team matches zone_a or zone_b, walks its prop list and, for each
// prop belonging to the OTHER of the two zones, refreshes combat-status flags and (unless
// force_update suppresses it while status is nonzero) restamps the prop's status byte,
// engaged flag and desirability score.
void ai_notify_actors_of_encounter_state_change(int16_t zone_a, int16_t zone_b, uint8_t status,
                                                 uint8_t force_update)
{
    actor_iterator_state iterator;
    actor *a;
    datum_index actor_index;
    int16_t other_zone;
    datum_index prop_cursor;
    datum_index current_prop_index;
    prop *p;

    if (ai_globals_ptr->actors_valid) {
        iterator.filter_array = encounter_data;
        iterator.unknown_04 = 0;
        iterator.cursor = -1;
        iterator.signature = (uint32_t)(uintptr_t)encounter_data ^ 0x69746572;
        iterator.unknown_10 = 0;
        iterator.active = 1;
        iterator.actor_index = -1;
        iterator.unknown_18 = -1;
        actor_index = (datum_index)k_datum_index_none;
    }

    a = actor_iterator_next(&iterator);
    while (a != 0) {
        other_zone = zone_b;
        if (a->team != zone_a) {
            other_zone = zone_a;
        }
        if ((a->team == zone_a || a->team == zone_b) && other_zone != (int16_t)-1) {
            // Ghidra's "local_8" is the current actor's own index, read back out of the
            // iterator after the call (UNSURE which field actually carries it -- computed
            // here directly from the returned pointer's position in actor_data instead,
            // which is mathematically the same value).
            actor_index = (datum_index)(((uint8_t *)a - (uint8_t *)actor_data->data) / sizeof(actor));

            prop_cursor = a->first_prop;
            while (prop_cursor != (datum_index)k_datum_index_none) {
                current_prop_index = prop_cursor;
                p = &((prop *)prop_data->data)[current_prop_index & 0xffff];
                prop_cursor = p->next_in_actor;
                if (p->object_type == other_zone) {
                    if (force_update == 0) {
                        p->unknown_61 = 1;
                        p->unknown_62 = 1;
                    }
                    if (status == 0 || force_update != 0) {
                        p->is_unit = status;
                        p->engaged = actor_target_update_active_flag(actor_index, current_prop_index);
                        p->desirability = actor_rate_potential_target(actor_index, current_prop_index);
                    }
                }
            }
        }
        a = actor_iterator_next(&iterator);
    }
}

#if 0
Original Ghidra decompilation (0x42b940):

void __cdecl
ai_notify_actors_of_encounter_state_change(short zone_a,short zone_b,char status,char force_update)

{
  uint uVar1;
  undefined1 uVar2;
  int iVar3;
  short sVar4;
  uint uVar5;
  float10 fVar6;
  uint local_8;

  if (*(char *)(DAT_00880354 + 1) != '\0') {
    local_8 = 0xffffffff;
  }
  iVar3 = actor_iterator_next();
  while (iVar3 != 0) {
    sVar4 = zone_b;
    if (((*(short *)(iVar3 + 0x3e) == zone_a) ||
        (sVar4 = zone_a, *(short *)(iVar3 + 0x3e) == zone_b)) && (sVar4 != -1)) {
      uVar1 = *(uint *)((local_8 & 0xffff) * 0x724 + 0x50 + *(int *)(DAT_00880360 + 0x34));
      while (uVar5 = uVar1, uVar5 != 0xffffffff) {
        iVar3 = (uVar5 & 0xffff) * 0x138 + *(int *)(DAT_008802c0 + 0x34);
        uVar1 = *(uint *)(iVar3 + 8);
        if (*(short *)(iVar3 + 0x12) == sVar4) {
          if (force_update == '\0') {
            *(undefined1 *)(iVar3 + 0x61) = 1;
            *(undefined1 *)(iVar3 + 0x62) = 1;
          }
          if ((status == '\0') || (force_update != '\0')) {
            *(char *)(iVar3 + 0x60) = status;
            uVar2 = FUN_0041fc60();
            *(undefined1 *)(iVar3 + 0xa4) = uVar2;
            fVar6 = (float10)actor_rate_potential_target(local_8,uVar5);
            *(float *)(iVar3 + 0x50) = (float)fVar6;
          }
        }
      }
    }
    iVar3 = actor_iterator_next();
  }
  return;
}

Real disassembly confirming actor_iterator_next dropped argument (0x42b940-0x42ba75):

0042b940: mov    eax,ds:0x880354
0042b945: mov    cl,[eax+0x1]
0042b948: sub    esp,0x20
0042b94b: test   cl,cl
0042b94d: je     0x42b983                 ; skip iterator init when !actors_valid
0042b94f: mov    eax,ds:0x8802c8          ; eax = encounter_data
0042b954: mov    ecx,0xffffffff
0042b959: mov    [esp+0x4],eax            ; iterator.filter_array
0042b95d: xor    eax,0x69746572           ; 'iter'
0042b962: mov    word ptr [esp+0x8],0x0   ; iterator.unknown_04
0042b969: mov    [esp+0xc],ecx            ; iterator.cursor
0042b96d: mov    [esp+0x10],eax           ; iterator.signature
0042b971: mov    byte ptr [esp+0x14],0x0  ; iterator.unknown_10
0042b976: mov    [esp+0x1c],ecx           ; iterator.unknown_18
0042b97a: mov    [esp+0x18],ecx           ; iterator.unknown_14
0042b97e: mov    byte ptr [esp+0x15],0x1  ; iterator.active
0042b983: lea    eax,[esp+0x4]
0042b987: call   0x436a70                 ; actor_iterator_next(&iterator)
#endif
