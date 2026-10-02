// network_session_player_table_index_apply  (Ghidra: FUN_004d9190; renamed, no prior name)
// address 0x4d9190, size 139 bytes
// name confidence: 0.35   rewrite confidence: 0.3
// evidence: out/phase4/networking_functions.md summary ("Finds the player slot matching a given
// machine id and records its assigned table index (e.g. team or score-table slot) for that
// player"); the loop cursor starts at param_1+0xcd2 and steps by 0x20 (network_player_entry's
// stride) comparing its first two bytes against candidate+0x1c/+0x1d, which are exactly
// network_player_entry::machine_index/machine_player_index (0x1c/0x1d) -- so
// param_1+0xcd2-0x1c = param_1+0xcb6 = &client->session.players[0], confirming param_1 is
// network_client_globals* (client->session.players sits at client+0xb14+0x1a2 = client+0xcb6).
// register convention: Ghidra's own recovered stack parameters (param_1 = client, param_2 = the
// table index to record); `unaff_EBX` (the candidate machine_index/machine_player_index source)
// is an elided register argument, reconstructed as a third parameter.
// blam-cc: stack -> client, table_index; EBX -> candidate
// UNSURE: `candidate`'s type is unknown -- it is read only at byte offsets +0x1c/+0x1d, the same
// two offsets network_player_entry uses for the same two fields, but nothing pins its overall
// layout or size; kept as a raw `const uint8_t *` rather than invented as a named struct.
// UNSURE: `network_player_entry_validate` (types/networking.h's own validator for network_player_entry, per
// out/phase4/networking_types_notes.md) is called with no visible argument in Ghidra's output
// (its result only appears via `extraout_AL`, a classic sign the argument-setup instruction was
// elided); reconstructed as taking the current loop candidate entry
// (`&client->session.players[i]`), the only pointer in scope that matches its documented
// "requires +0x1d in 0..0, +0x1c in 0..15" signature.
// UNSURE: player_data's growable-array base pointer is read as `*(int *)(player_data + 0x34)`;
// types/memory.h's data_array does not (yet) name an offset-0x34 field, so it is read via a raw
// offset on `(uint8_t *)player_data` rather than guessed into a new header field.
// `param_1 + 0xec0` is client->session.unknown_3ac (0xb14 + 0x3ac session offset = 0xec0).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *player_data; // 0x0087a480
extern char network_player_entry_validate(network_player_entry *entry); // 0x4de9f0, UNSURE argument; not in this batch
extern int32_t player_data_iterator_advance(int16_t step_count); // 0x4d98f0

// blam-cc: stack -> client, table_index; EBX -> candidate
// FIXED (objdump): every ret sets only AL; the upper bits of EAX are left as they were
uint8_t network_session_player_table_index_apply(network_client_globals *client, int32_t table_index,
                                                   const uint8_t *candidate)
{
    int32_t i;
    network_player_entry *entry;
    int32_t player_slot;
    uint8_t *player_base;

    i = 0;
    entry = &client->session.players[0];
    while (1) {
        if (network_player_entry_validate(entry) != 0 && entry->machine_index == (int8_t)candidate[0x1c] &&
            entry->machine_player_index == (int8_t)candidate[0x1d]) {
            break;
        }
        i = i + 1;
        entry = &client->session.players[i];
        if (i > 15) {
            return 0;
        }
    }

    player_slot = player_data_iterator_advance((int8_t)client->session.players[i].slot_index);
    if (client->session.map_loaded != 0 && player_slot != 0 && (uint32_t)player_slot != 0xffffffff &&
        table_index != -1) {
        player_base = *(uint8_t **)((uint8_t *)player_data + 0x34);
        *(int32_t *)(player_base + ((uint32_t)player_slot & 0xffff) * 0x200 + 0xd0) = table_index;
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4d9190):

undefined4 FUN_004d9190(int param_1,int param_2)

{
  char extraout_AL;
  uint uVar1;
  int unaff_EBX;
  int iVar2;
  char *pcVar3;

  iVar2 = 0;
  pcVar3 = (char *)(param_1 + 0xcd2);
  while( true ) {
    FUN_004de9f0();
    if (((extraout_AL != '\0') && (*pcVar3 == *(char *)(unaff_EBX + 0x1c))) &&
       (pcVar3[1] == *(char *)(unaff_EBX + 0x1d))) break;
    iVar2 = iVar2 + 1;
    pcVar3 = pcVar3 + 0x20;
    if (0xf < iVar2) {
      return 0;
    }
  }
  uVar1 = FUN_004d98f0((int)*(char *)(iVar2 * 0x20 + 0xcd5 + param_1));
  if (((*(char *)(param_1 + 0xec0) != '\0') && (uVar1 != 0)) &&
     ((uVar1 != 0xffffffff && (param_2 != -1)))) {
    *(int *)((uVar1 & 0xffff) * 0x200 + 0xd0 + *(int *)(DAT_0087a480 + 0x34)) = param_2;
  }
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
