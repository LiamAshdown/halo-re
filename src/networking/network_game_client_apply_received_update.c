// network_game_client_apply_received_update  (Ghidra: network_game_client_apply_received_update,
// already named)
// address 0x4e0280, size 308 bytes
// name confidence: 0.5   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md: "Processes one received network update packet
// for the given connection: updates timing/history bookkeeping, checks client latency via
// network_client_check_connection_quality, applies the position delta with FUN_004dff70, and log[s]..." unaff_EBX+0x1c is
// exactly network_machine::connect_state (13 dwords, 0x34 bytes); the log format string
// ("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n") and unaff_EBX+0xc (byte within
// connect_state) match network_player_update_history_log_write's summary.
// register convention: EBX = machine (network_machine *), stack = param_1 (unused),
// message (a pointer to a pointer to the message record).
// blam-cc: EBX -> machine, stack -> (unused, message)
// UNSURE: no caller in this batch shows EBX being loaded before this call (it is presumably
// set up by an outer caller outside the range being rewritten here); modelled as a normal
// parameter per the register convention regardless.
// UNSURE: connect_state is documented as fully opaque in out/phase4/networking_types_notes.md.
// It is staged into a local copy before the type-1/else dispatch and copied back afterward, so
// message_delta_read_changed_subfields and FUN_004ec590 almost certainly take that staged copy
// as an implicit decode-context argument, but neither call shows any visible arguments in the
// decompilation; both are called argument-less here, matching Ghidra literally.
// UNSURE: network_client_check_connection_quality and network_game_client_apply_position_update (FUN_004dff70) are
// likewise called with no visible arguments in the original. The latter's real signature (see
// network_game_client_apply_position_update.c) needs a `state` and a `packet` pointer, which
// are very likely the staged connect_state copy and the resolved message record respectively;
// its `object` argument has no visible source at all and is passed as NULL here rather than
// invented.
// UNSURE: the message record's own layout is not established; accessed only through the two
// offsets this function itself touches (type at +0x00, a counter at +0x0c, a flag byte at
// +0x1d), consistent with out/phase4/networking_types_notes.md's note that the message-delta
// wire record has no declared type in this module.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <string.h>

extern int32_t message_delta_read_changed_subfields(message_delta_decode_state *state,
    void *field_bindings, const void *previous, void *destination);
    // blam-cc: EDI -> state, stack -> field_bindings, previous, destination; 0x4ed1d0
extern uint8_t message_delta_decode_compound_field(void *decode_context, void *destination);
    // blam-cc: EAX -> decode_context, ECX -> destination; 0x4ec590, message-delta stateless
    // (baseline) decode. It forwards to message_delta_read_changed_subfields with a NULL
    // previous-state pointer and the caller destination (0x4ec591..0x4ec59a).
extern char network_client_check_connection_quality(void); // 0x4e0080, this batch (UNSURE args here; see its own file)
extern void network_game_client_apply_position_update(void *state, uint32_t *packet, void *tick_count, void *object); // 0x4dff70, this batch
extern void network_player_update_history_log_write(const char *format, ...); // 0x4e7f90, other module

// Stages `machine`'s connect_state, dispatches the message record by its type (either merges
// changed sub-fields or takes the FUN_004ec590 path), restores connect_state, and -- if the
// staged copy's first byte is set -- checks connection quality and, on success, applies the
// position/orientation update and logs it.
void network_game_client_apply_received_update(network_machine *machine, uint32_t param_1, void **message)
{
    uint8_t staged[0x34];
    uint8_t staged2[0x34];
    int32_t *msg;
    int32_t delta_bits;

    (void)param_1;
    memcpy(staged, machine->connect_state, 0x34);

    msg = (int32_t *)*message;
    if (*msg == 1) {
        // blam-cc: EDI -> the decode state (0x4e029a `mov edi,[eax]`), stack -> message + 1,
        // machine->connect_state (previous), staged (destination)
        delta_bits = message_delta_read_changed_subfields((message_delta_decode_state *)msg,
            message + 1, machine->connect_state, staged);
        msg[3] = msg[3] + delta_bits;
        *((uint8_t *)msg + 0x1d) = 1;
    } else {
        // blam-cc: EAX -> message, ECX -> staged
        message_delta_decode_compound_field(message, staged);
    }

    memcpy(machine->connect_state, staged, 0x34);

    if (staged[0] != 0) {
        char quality_ok;

        memcpy(staged2, staged, 0x34);
        quality_ok = network_client_check_connection_quality();
        if (quality_ok == 1) {
            network_game_client_apply_position_update(staged2, (uint32_t *)msg, 0, 0); // UNSURE: object arg unknown
            if (*(int16_t *)(machine->connect_state + 0xc) != 0) {
                GetTickCount();
                network_player_update_history_log_write("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n");
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x4e0280):

void network_game_client_apply_received_update(undefined4 param_1,int *param_2)

{
  char cVar1;
  int iVar2;
  int unaff_EBX;
  undefined4 *puVar3;
  char *pcVar4;
  undefined4 auStackY_9c [7];
  undefined4 uStackY_80;
  char local_34 [52];

  puVar3 = (undefined4 *)(unaff_EBX + 0x1c);
  pcVar4 = local_34;
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *(undefined4 *)pcVar4 = *puVar3;
    puVar3 = puVar3 + 1;
    pcVar4 = pcVar4 + 4;
  }
  param_2 = (int *)*param_2;
  if (*param_2 == 1) {
    iVar2 = message_delta_read_changed_subfields();
    param_2[3] = param_2[3] + iVar2;
    *(undefined1 *)((int)param_2 + 0x1d) = 1;
  }
  else {
    FUN_004ec590();
  }
  pcVar4 = local_34;
  puVar3 = (undefined4 *)(unaff_EBX + 0x1c);
  for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
    *puVar3 = *(undefined4 *)pcVar4;
    pcVar4 = pcVar4 + 4;
    puVar3 = puVar3 + 1;
  }
  if (local_34[0] != '\0') {
    pcVar4 = local_34;
    puVar3 = auStackY_9c;
    for (iVar2 = 0xd; iVar2 != 0; iVar2 = iVar2 + -1) {
      *puVar3 = *(undefined4 *)pcVar4;
      pcVar4 = pcVar4 + 4;
      puVar3 = puVar3 + 1;
    }
    cVar1 = FUN_004e0080();
    if (cVar1 == '\x01') {
      FUN_004dff70();
      if (*(short *)(unaff_EBX + 0xc) != 0) {
        GetTickCount();
        uStackY_80 = 0x4e03aa;
        network_player_update_history_log_write
                  ("[%d]: [%d]:\t Received update [%d] for [%d] ticks.\n");
      }
    }
  }
  return;
}
#endif
