// player_delete  (Ghidra: player_delete, already named)
// address 0x473ae0, size 204 bytes
// name confidence: 0.5   rewrite confidence: 0.45
// evidence: out/phase4/game_functions.md ("Frees a player's dynamic allocations, unlinks it
//   from the local-player map, and deletes its datum"); types/game.h player::local_player_index
//   (the -1 gate that matches player_new_local's own "build the network update queues only for
//   a non-local player" condition -- this is the matching teardown); types/game.h
//   network_game_mode (0 local, 1 client, 2 host, 3 replay), network_server (0x0071c2d4).
//   objdump -d -M intel --start-address=0x473ae0 --stop-address=0x473bb0 bin/halo.exe pins
//   every register.
// register convention: EAX -> machine_index, EBX -> player_handle.
//   // blam-cc: EAX -> machine_index, EBX -> player_handle
//
// CORRECTED (phase 4 review, against the disassembly): Ghidra shows the very first call,
// "FUN_0045c570();" (game_engine_player_changed_object), with no visible argument. The call
// site pushes only the two callee-saved registers (edi, ebx) before it, so the callback's own
// single stack parameter reads back exactly the just-pushed EBX value -- i.e. this call
// genuinely passes player_handle, via a coincidence of the two adjacent stack slots rather than
// an explicit push. Transcribed here as an explicit argument so the C is portable.
// UNSURE: the queue teardown below frees update_history's storage block (+0x134) directly but
// never frees its records table (+0x128, the pointer-per-record array player_update_queue_create
// allocates); network_queue_destroy is used only for position_updates, and vehicle_updates is
// never touched at all. Both asymmetries are exactly what the disassembly shows -- transcribed
// faithfully, not "fixed".

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

extern data_array *player_data;              // 0x0087a480
extern int16_t network_game_mode;            // 0x00719720
extern network_server_globals *network_server;
extern datum_index machine_to_player[16];    // 0x006b1460

extern void game_engine_player_changed_object(uint32_t param); // 0x45c570, this module (already
    // rewritten, src/game/game_engine_player_changed_object.c)
extern void network_queue_destroy(circular_queue *queue); // 0x47a090, this batch's neighbor,
    // blam-cc: ESI -> queue
extern void datum_delete(data_array *array, datum_index handle); // 0x4d0510, blam-cc: EAX -> array,
    // EDX -> handle
extern uint32_t network_machine_clear_flag_by_id(network_server_globals *server, int32_t machine_id); // 0x4e0b90,
    // EDX server, EDI machine_id

// Notifies the active game engine that this player's object is going away, then -- only while
// this machine is a network client (mode 1) -- tears down the player's incoming update-history
// storage if it is still a non-local (remotely represented) player, or -- while hosting
// (mode 2) -- runs the host-side per-player network cleanup. In either networked mode, clears
// this machine's machine_to_player slot if it still points at this player. Always deletes the
// player datum last.
// The "goto the same tail either way" shape of the original (see #if 0) is folded here into an
// `update_machine_slot` flag; every input reaches the identical final datum_delete call.
void player_delete(uint32_t machine_index, datum_index player_handle)
    // blam-cc: EAX -> machine_index, EBX -> player_handle
{
    player *p;
    int16_t index;
    int16_t requested_salt;
    uint8_t update_machine_slot;

    game_engine_player_changed_object(player_handle); // CORRECTED: real argument is EBX (see header)

    update_machine_slot = 0;
    if (network_game_mode == 1) {
        update_machine_slot = 1;
        if (player_handle != (datum_index)-1) {
            index = (int16_t)player_handle;
            if (index >= 0 && index < player_data->maximum_count) {
                p = (player *)((uint8_t *)player_data->data + (uint32_t)(uint16_t)index * player_data->size);
                requested_salt = (int16_t)(player_handle >> 16);
                if (p->identifier != 0 &&
                    (requested_salt == 0 || p->identifier == requested_salt) &&
                    p->local_player_index == -1) {
                    GlobalFree(p->update_history.queue.storage); // UNSURE: leaves .queue.records unfreed
                    p->update_history.queue.storage = (void *)0;
                    network_queue_destroy(&p->position_updates);
                }
            }
        }
    } else if (network_game_mode == 2) {
        update_machine_slot = 1;
        // 0x473afd: EDX = [0x71c2d4], EDI = machine_index (mov edi,eax at entry, 0x473ae2)
        network_machine_clear_flag_by_id((network_server_globals *)network_server, (int32_t)machine_index);
    }

    if (update_machine_slot && machine_to_player[machine_index & 0xffff] == player_handle) {
        machine_to_player[machine_index & 0xffff] = (datum_index)-1;
    }
    datum_delete(player_data, player_handle);
}

#if 0
Original Ghidra decompilation (0x473ae0), from tools/pack.py 0x473ae0:

void player_delete(void)

{
  uint in_EAX;
  int iVar1;
  short sVar2;
  short sVar3;
  int unaff_EBX;

  FUN_0045c570();
  if (DAT_00719720 == 1) {
    if (((unaff_EBX != -1) && (sVar3 = (short)unaff_EBX, -1 < sVar3)) &&
       (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
      iVar1 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
      sVar3 = *(short *)(iVar1 + *(int *)(DAT_0087a480 + 0x34));
      iVar1 = iVar1 + *(int *)(DAT_0087a480 + 0x34);
      if (((sVar3 != 0) &&
          ((sVar2 = (short)((uint)unaff_EBX >> 0x10), sVar2 == 0 || (sVar3 == sVar2)))) &&
         (*(short *)(iVar1 + 2) == -1)) {
        GlobalFree(*(HGLOBAL *)(iVar1 + 0x134));
        *(undefined4 *)(iVar1 + 0x134) = 0;
        network_queue_destroy();
      }
    }
  }
  else {
    if (DAT_00719720 != 2) goto LAB_00473b9f;
    FUN_004e0b90();
  }
  iVar1 = 0;
  while ((&DAT_006b1460)[(in_EAX & 0xffff) + iVar1] != unaff_EBX) {
    iVar1 = iVar1 + 1;
    if (0 < iVar1) {
      datum_delete();
      return;
    }
  }
  (&DAT_006b1460)[(in_EAX & 0xffff) + iVar1] = 0xffffffff;
LAB_00473b9f:
  datum_delete();
  return;
}
#endif
