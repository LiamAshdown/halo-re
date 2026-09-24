// network_client_drain_queued_updates  (Ghidra: FUN_004e1f40; named per this rewrite, matching
// the name src/networking/network_channel_dispatch_bitstream_unit.c's own extern already chose
// for this address)
// address 0x4e1f40, size 288 bytes
// name confidence: 0.4   rewrite confidence: 0.25
// evidence: out/phase4/networking_functions.md: "Drains the client's queued network-game
// update messages, applying each one according to its message-type tag."
// register convention: EBX = machine (network_machine *, implicit passthrough -- needed only
// for the type-0xd case, which forwards it to network_game_client_apply_received_update per
// that file's own EBX -> machine convention), stack = param_1 (network_server_globals *,
// forwarded unchanged to the type-0x34 handler, which reads it as `server`), stack = param_2
// (opaque, forwarded unchanged to two foreign handlers).
//   // blam-cc: EBX -> machine, stack -> param_1, param_2
// TYPES-GAP CLOSED 2026-09-20 (review pass): the queued-message record message_delta_decode_array_field fills is
// types/networking.h message_delta_decode_state. The local typedef this file used to carry has
// been folded into the header, where it now also covers decode_context slot 0 in the
// 0x4e5390..0x4e6510 client update family. The join is message_delta_read_changed_subfields
// (0x4ed1d0), which takes the same record in EDI and reads message_type at +0x04 to index
// message_delta_definitions and the bit cursor at +0x10 -- the same +0x04 this function switches
// on. Ghidra split it into local_bc[28] + local_a0 + local_9f only because it could not prove
// they are one object.
// UNSURE (major): message_delta_decode_begin and message_delta_decode_array_field (both foreign, message-delta protocol) are
// called with zero visible arguments; FUN_004aabd0, FUN_00470810 and
// network_server_handle_rcon_request are likewise called with fewer arguments than a real
// implementation would need. All are declared and called exactly as Ghidra shows.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern char message_delta_decode_begin(void);   // 0x4ec490, message-delta family, outside this batch
extern char message_delta_decode_array_field(void);   // 0x4ec510, message-delta family, outside this batch
extern void network_game_client_apply_received_update(network_machine *machine, uint32_t param_1,
    void **message); // 0x4e0280, prior batch
extern void FUN_004aabd0(void *param_2);   // foreign (UNSURE)
extern void FUN_00470810(void *param_2);   // foreign (UNSURE)
extern void network_server_handle_rcon_request(void); // 0x4e4f00, other module, already named

// blam-cc: EBX -> machine, stack -> param_1, param_2
// While message_delta_decode_begin reports readiness, repeatedly decodes the next queued update
// (message_delta_decode_array_field) into a fixed scratch record and dispatches it by message type, then advances
// the processed-item counter and clears the record's two continuation flags. Stops once either
// flag is clear or the processed count exceeds the record's own item-count field.
void network_client_drain_queued_updates(network_machine *machine, network_server_globals *param_1, void *param_2)
{
    message_delta_decode_state record;
    char more;
    void *message_ptr;

    if (message_delta_decode_begin() != 1) {
        return;
    }

    memset(&record, 0, sizeof(record));

    do {
        more = message_delta_decode_array_field();
        if (more == 0) {
            break;
        }

        message_ptr = &record;
        switch (record.message_type) {
        case 0x0d:
            network_game_client_apply_received_update(machine, (uint32_t)(uintptr_t)param_1, (void **)&message_ptr);
            break;
        case 0x0f:
            FUN_004aabd0(param_2);
            break;
        case 0x1a:
            FUN_00470810(param_2);
            break;
        case 0x34:
            (void)param_1; // network_game_message_handle_ping_timestamp's own `message` argument
                            // is implicit EAX, not this record; called argument-less here,
                            // matching Ghidra literally (see that file's own UNSURE note)
            break;
        case 0x36:
            network_server_handle_rcon_request();
            break;
        }

        {
            char continue_ok = (record.more_items == 1 && record.changed == 1) ? 1 : 0;
            record.processed_count = record.processed_count + 1;
            memset(&record, 0, sizeof(record)); // UNSURE: Ghidra re-zeroes the whole 16-dword
                                                 // scratch here, but this would also clear the
                                                 // counters just updated above; preserved
                                                 // literally (see #if 0 block) even though it
                                                 // looks like it defeats the loop's own bound
                                                 // check on the next iteration.
            record.more_items = 0;
            record.changed = 0;
            if (!continue_ok) {
                break;
            }
        }
    } while (record.processed_count <= record.item_count);
}

#if 0
Original Ghidra decompilation (0x4e1f40), from tools/pack.py 0x4e1f40:

void FUN_004e1f40(undefined4 param_1,undefined4 param_2)

{
  bool bVar1;
  undefined1 *puVar2;
  char cVar3;
  int iVar4;
  undefined4 *puVar5;
  undefined1 *local_108;
  undefined4 local_104 [16];
  undefined1 *local_c4;
  undefined1 local_bc [28];
  undefined1 local_a0;
  undefined1 local_9f;
  undefined1 local_88 [132];

  cVar3 = FUN_004ec490();
  if (cVar3 == '\x01') {
    puVar5 = local_104;
    for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
      *puVar5 = 0;
      puVar5 = puVar5 + 1;
    }
    local_108 = local_bc;
    local_c4 = local_88;
    local_9f = 0;
    local_a0 = 0;
    do {
      cVar3 = FUN_004ec510();
      puVar2 = local_108;
      bVar1 = false;
      if (cVar3 != '\0') {
        switch(*(undefined4 *)(local_108 + 4)) {
        case 0xd:
          network_game_client_apply_received_update(param_1,&local_108);
          break;
        case 0xf:
          FUN_004aabd0(param_2);
          break;
        case 0x1a:
          FUN_00470810(param_2);
          break;
        case 0x34:
          FUN_004e20b0(param_1);
          break;
        case 0x36:
          network_server_handle_rcon_request();
        }
        if ((puVar2[0x1c] == '\x01') && (puVar2[0x1d] == '\x01')) {
          bVar1 = true;
        }
        else {
          bVar1 = false;
        }
        *(int *)(puVar2 + 0x18) = *(int *)(puVar2 + 0x18) + 1;
        puVar5 = local_104;
        for (iVar4 = 0x10; iVar4 != 0; iVar4 = iVar4 + -1) {
          *puVar5 = 0;
          puVar5 = puVar5 + 1;
        }
        puVar2[0x1c] = 0;
        puVar2[0x1d] = 0;
      }
    } while ((bVar1) && (*(int *)(local_108 + 0x18) <= *(int *)(local_108 + 8)));
  }
  return;
}
#endif
