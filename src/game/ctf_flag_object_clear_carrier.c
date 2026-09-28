// ctf_flag_object_clear_carrier  (Ghidra: ctf_flag_object_clear_carrier, already named)
// address 0x4666c0, size 99 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x4666c0
//   --stop-address=0x466730): both callees are already rewritten with known signatures
//   (object_set_position_and_orientation, object_reset_velocity_and_wake, src/objects/), and
//   their argument registers pin this function's own two implicit parameters -- EBX is pushed
//   straight through as their object_index and is never itself assigned, and EDI (never
//   assigned either) is exactly the position pointer object_set_position_and_orientation reads
//   from its own documented EDI slot. types/units.h's already-declared global_forward3d_pointer
//   / global_up3d_pointer (0x696718 / 0x696720) are the (1,0,0) / (0,0,1) identity axes read out
//   of bin/halo.exe. object+0x200/+0x204 are types/items.h item_data::ignore_object_index /
//   held_game_time; object+0x22c is equipment_data's first dword, which types/items.h leaves
//   fully unresolved.
// register convention: object handle in EBX, target position in EDI.
//   // blam-cc: EBX -> flag_object_index, EDI -> position
// UNSURE: the bit cleared at object+0x22c (mask 0xffffffdf, i.e. bit 5) is inside
//   equipment_data, which types/items.h has not resolved; kept as a raw offset.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "items.h"
#include "game.h"

extern data_array *object_data;                    // 0x008603b0
extern game_time_globals *game_time;                  // 0x006f1d6c
extern real_vector3d *global_forward3d_pointer;        // 0x00696718, (1,0,0)
extern real_vector3d *global_up3d_pointer;             // 0x00696720, (0,0,1)

extern void object_set_position_and_orientation(datum_index object_index, real_vector3d *forward,
                                                  real_vector3d *up, real_point3d *position); // 0x4f51c0
extern void object_reset_velocity_and_wake(uint32_t object_index); // 0x4f5160

// blam-cc: EBX -> flag_object_index, EDI -> position
// Drops a CTF flag object at `position` facing the world identity axes, wakes it and resets its
// velocity, clears an equipment-runtime flag bit (UNSURE which), and resets its
// held-by/held-since bookkeeping (item_data::ignore_object_index and held_game_time) as if it
// had just been returned to the ground.
void ctf_flag_object_clear_carrier(datum_index flag_object_index, real_point3d *position)
{
    object *flag_obj;
    item_data *item;
    uint32_t *unknown_22c;

    if (flag_object_index == (datum_index)0xffffffff) {
        return;
    }

    flag_obj = ((object_header *)object_data->data)[flag_object_index & 0xffff].data;

    object_set_position_and_orientation(flag_object_index, global_forward3d_pointer,
                                         global_up3d_pointer, position);
    object_reset_velocity_and_wake(flag_object_index);

    unknown_22c = (uint32_t *)((uint8_t *)flag_obj + 0x22c); // UNSURE: equipment_data+0x00
    *unknown_22c = *unknown_22c & 0xffffffdf;

    item = (item_data *)((uint8_t *)flag_obj + k_item_data_offset);
    item->held_game_time = game_time->game_time;
    item->ignore_object_index = (datum_index)0xffffffff;
}

#if 0
Original Ghidra decompilation (0x4666c0), from tools/pack.py 0x4666c0:

void ctf_flag_object_clear_carrier(void)

{
  int iVar1;
  int iVar2;
  uint unaff_EBX;

  if (unaff_EBX != 0xffffffff) {
    iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (unaff_EBX & 0xffff) * 0xc);
    object_set_position_and_orientation();
    object_reset_velocity_and_wake();
    iVar2 = DAT_006f1d6c;
    *(uint *)(iVar1 + 0x22c) = *(uint *)(iVar1 + 0x22c) & 0xffffffdf;
    *(undefined4 *)(iVar1 + 0x204) = *(undefined4 *)(iVar2 + 0xc);
    *(undefined4 *)(iVar1 + 0x200) = 0xffffffff;
  }
  return;
}

Raw disassembly (objdump -d -M intel --start-address=0x4666c0 --stop-address=0x466730):

004666c0:  cmp ebx,0xffffffff
004666c3:  je 0x466722
004666c5:  mov ecx,ds:0x8603b0
004666cb:  mov edx,[ecx+0x34]
004666ce:  mov ecx,ds:0x696718
004666d4:  mov eax,ebx
004666d6:  and eax,0xffff
004666db:  push esi
004666dc:  lea eax,[eax+eax*2]
004666df:  mov esi,[edx+eax*4+0x8]
004666e3:  mov eax,ds:0x696720
004666e8:  push eax
004666e9:  push ecx
004666ea:  push ebx
004666eb:  call 0x4f51c0
004666f0:  push ebx
004666f1:  call 0x4f5160
004666f6:  mov eax,[esi+0x22c]
004666fc:  mov edx,ds:0x6f1d6c
00466702:  add esp,0x10
00466705:  and eax,0xffffffdf
00466708:  mov [esi+0x22c],eax
0046670e:  mov eax,[edx+0xc]
00466711:  mov [esi+0x204],eax
00466717:  mov dword ptr [esi+0x200],0xffffffff
0046671e:  pop esi
00466722:  ret
#endif
