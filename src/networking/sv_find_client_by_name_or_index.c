// sv_find_client_by_name_or_index  (Ghidra: sv_find_client_by_name_or_index, already named)
// address 0x4e3f70, size 194 bytes
// name confidence: 0.6   rewrite confidence: 0.6
// evidence: out/phase4/networking_functions.md; types/networking.h network_player_entry
// (machine_index/slot_index) and its note on FUN_004de9f0 ("validate ... requires
// machine_player_index in 0..0, machine_index in 0..15, and a UTF-16 name ... within 12
// characters"); disassembly (objdump -d -M intel) pins the single-string register convention and
// the two lookup paths (by 1-based numeric index matching slot_index, or by name).
// register convention: EAX -> name_or_index.
//   // blam-cc: EAX -> name_or_index
// UNSURE: FUN_00557990 (foreign, < this batch) -- from its shape (destination buffer, one
// string argument) this is read as an ANSI-to-UTF-16 conversion into the 12-wide-char scratch
// this function compares against each player's name.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern network_server_globals *network_server; // 0x0071c2d4

extern uint8_t string_is_numeric(char *string); // this batch, 0x4e3f30
extern uint16_t *string_convert_ascii_to_unicode(uint16_t *dst, uint32_t capacity_bytes, const char *source); // 0x557990, EAX dst, EDI capacity, EBX source
extern uint8_t network_player_entry_validate(network_player_entry *entry); // foreign (< this batch), validates one player row

// Resolves a console-command argument to a player row of the local session: either a 1-based
// client slot number (matched against each row's slot_index) or a player name (matched
// case-sensitively, UTF-16). Returns 0 if nothing matches.
network_player_entry *sv_find_client_by_name_or_index(char *name_or_index) // blam-cc: EAX -> name_or_index
{
    network_game_session *session = &network_server->session;
    int32_t i;

    if (string_is_numeric(name_or_index) == 0) {
        uint16_t wide_name[13];

        string_convert_ascii_to_unicode(wide_name, 0x1a, name_or_index); // 0x4e3f8d: EDI = 0x1a bytes (13 characters)
        for (i = 0; i < 0x10; i = i + 1) {
            network_player_entry *entry = &session->players[i];
            if (network_player_entry_validate(entry) != 0 && wcscmp((const wchar_t *)wide_name, (const wchar_t *)entry->name) == 0) {
                return entry;
            }
        }
        return 0;
    } else {
        int32_t index = atol(name_or_index) - 1;
        if (index < 0 || 0x10 <= index) {
            return 0;
        }
        for (i = 0; i < 0x10; i = i + 1) {
            network_player_entry *entry = &session->players[i];
            if (network_player_entry_validate(entry) != 0 && entry->slot_index == index) {
                return entry;
            }
        }
        return 0;
    }
}

#if 0
Original Ghidra decompilation (0x4e3f70), from tools/pack.py 0x4e3f70:

(pack.py's decompiled-C section for this address was not printed with a body; the control flow
below is reconstructed directly from objdump -d -M intel, bin/halo.exe, 0x4e3f70..0x4e4031:)

undefined4 sv_find_client_by_name_or_index(char *name_or_index)

{
  network_server_globals *server = DAT_0071c2d4;
  bool numeric = string_is_numeric(name_or_index);
  int i;
  if (!numeric) {
    uint16_t wide_name[13];
    FUN_00557990(wide_name, name_or_index);
    for (i = 0; i < 0x10; i++) {
      network_player_entry *entry = &server->session.players[i];
      if (FUN_004de9f0(entry) && _wcscmp(wide_name, entry->name) == 0) return entry;
    }
    return 0;
  } else {
    int index = atol(name_or_index) - 1;
    if (index < 0 || index >= 0x10) return 0;
    for (i = 0; i < 0x10; i++) {
      network_player_entry *entry = &server->session.players[i];
      if (FUN_004de9f0(entry) && entry->slot_index == index) return entry;
    }
    return 0;
  }
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
