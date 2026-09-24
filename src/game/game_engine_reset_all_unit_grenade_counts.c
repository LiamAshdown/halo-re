// game_engine_reset_all_unit_grenade_counts  (Ghidra: FUN_00467de0; named per this rewrite --
// see evidence below, which sharpens out/phase4/game_functions.md's own guess: "Clears two
// per-unit flag bytes for every player's current unit, likely as part of a new-round/new-life
// reset pass.")
// address 0x467de0, size 119 bytes
// name confidence: 0.5   rewrite confidence: 0.65
// evidence: types/units.h unit_data::grenade_counts[2] (0x31e and 0x31f == 799, "unit_get_
//   grenade_count indexes it") -- the exact two bytes this function zeroes, which is why it is
//   renamed from the phase-2 guess ("per-unit flag bytes") to what they actually are.
//   VERIFIED against the disassembly (objdump -d -M intel --start-address=0x467de0
//   --stop-address=0x467e60): Ghidra's decompile drops the local data_iterator this function
//   builds on its own stack and passes in EDI (src/memory/data_iterator_next.c documents that
//   real convention), not the "()" it shows. Each iterator is initialized fresh
//   (data=player_data, next_index=0, index=-1) plus a fourth dword (player_data XOR 0x69746572,
//   the ASCII bytes "iter") that data_iterator_next's own rewritten body never reads back --
//   kept here for fidelity even though it is provably dead.
// register convention: no parameters.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "units.h"
#include "game.h"

extern data_array *player_data;    // 0x0087a480
extern data_array *object_headers; // 0x008603b0

extern void *data_iterator_next(data_iterator *iterator); // 0x4d05d0, blam-cc: EDI -> iterator

// Zeroes both grenade-type counts (unit_data::grenade_counts[0] and [1]) on every player's
// current unit, e.g. as part of a new-round reset.
void game_engine_reset_all_unit_grenade_counts(void)
{
    data_iterator iterator;
    uint32_t unused_checksum; // UNSURE: written, never read back (see header)
    player *p;

    iterator.data = player_data;
    iterator.next_index = 0;
    iterator.index = (datum_index)0xffffffff;
    unused_checksum = (uint32_t)player_data ^ 0x69746572;

    p = (player *)data_iterator_next(&iterator);
    while (p != (player *)0) {
        if (p->unit != (datum_index)0xffffffff) {
            object *unit_obj = ((object_header *)object_headers->data)[p->unit & 0xffff].data;
            unit_data *unit = (unit_data *)((uint8_t *)unit_obj + k_unit_data_offset);
            unit->grenade_counts[0] = 0;
            unit->grenade_counts[1] = 0;
        }
        p = (player *)data_iterator_next(&iterator);
    }

    (void)unused_checksum;
}

#if 0
Original Ghidra decompilation (0x467de0), from tools/pack.py 0x467de0:

void FUN_00467de0(void)

{
  int iVar1;
  int iVar2;

  iVar2 = data_iterator_next();
  iVar1 = DAT_008603b0;
  while (iVar2 != 0) {
    if (*(uint *)(iVar2 + 0x34) != 0xffffffff) {
      iVar2 = *(int *)(*(int *)(iVar1 + 0x34) + 8 + (*(uint *)(iVar2 + 0x34) & 0xffff) * 0xc);
      *(undefined1 *)(iVar2 + 0x31e) = 0;
      *(undefined1 *)(iVar2 + 799) = 0;
    }
    iVar2 = data_iterator_next();
  }
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x467de0 --stop-address=0x467e60):

00467de0:  sub esp,0x10
00467de3:  mov eax,ds:0x87a480
00467de8:  push ebx
00467de9:  mov [esp+0x4],eax        ; iterator.data = player_data
00467ded:  push edi
00467dee:  xor eax,0x69746572
00467df3:  xor ebx,ebx
00467df5:  lea edi,[esp+0x8]        ; edi = &iterator
00467df9:  mov word [esp+0xc],bx    ; iterator.next_index = 0
00467dfe:  mov dword [esp+0x10],0xffffffff  ; iterator.index = -1
00467e06:  mov [esp+0x14],eax       ; unused_checksum
00467e0a:  call 0x4d05d0            ; data_iterator_next(EDI=&iterator)
...
#endif
