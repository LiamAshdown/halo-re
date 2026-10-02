// network_game_session_finalize_and_add_player  (Ghidra: FUN_004df840, unnamed)
// address 0x4df840, size 185 bytes
// name confidence: 0.4   rewrite confidence: 0.45
// evidence: out/phase4/networking_functions.md: "Sanitises and finalises a newly-joining
// player's name and colour (rejecting reserved characters, resolving collisions), then
// registers the player in the channel table via FUN_004de4e0." The reserved characters are
// literally '%' and '|' (the `local_8` scratch), matching the join-name escaping used
// elsewhere in this module. Field reads against `entry` (in_EAX) match
// types/networking.h's network_player_entry exactly: word 0xe -> byte 0x1c
// (machine_index, sign-extended), word 0xf -> byte 0x1e (unknown_1e), word 0xc -> 0x18
// (color_index). `in_ECX + 8` lands on network_server_globals::session (offset 0x008).
// register convention: EAX = entry (network_player_entry *), ECX = server
// (network_server_globals *), EDX = machine (network_machine *).
//   // blam-cc: EAX -> entry, ECX -> server, EDX -> machine
// FIXED (register inputs, objdump): EDX carries machine (read at 0x4df850,
// cmp WORD PTR [edx+0xc],cx); the prose note above already named it correctly but had no
// machine-readable "// blam-cc:" line and its own continuation wrapped without the file's usual
// two-space "//" indent, so the checker's parser cut the note off before reaching EDX.
// UNSURE: `in_EDX` is inferred to be the joining machine's network_machine record purely
// from `*(short *)(in_EDX + 0xc)` matching machine_id's offset; no caller in this batch
// shows the argument being loaded, so the pointer's exact source is not re-derived here.
// UNSURE: the final `return (uint)in_EAX & 0xffffff00;` on the mismatch path is transcribed
// literally; it looks like leftover register reuse from the compiler rather than a
// meaningful result, but semantics are preserved as-is.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <wchar.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t game_engine_team_is_leading(uint32_t requested_team); // 0x470720, other module;
    // UNSURE: parameter/return purpose inferred only from "-1 means auto-assign" usage here
extern void network_game_generate_unique_random_name(void); // 0x4df730, this batch;
    // UNSURE: real signature takes an output buffer + entry; called here with no visible args
    // because it operates on the same `entry` this function already holds in EAX
extern char network_player_name_collision_check(void); // 0x4df6f0, this batch;
    // UNSURE: takes the candidate name in EBX per its own header; not re-derivable here
extern void network_player_assign_random_color(void); // 0x4df790, this batch;
    // UNSURE: takes `entry` in EAX, matching its own documented "stores at param_1+0x18"
extern uint32_t network_player_entry_add(network_player_entry *entry, network_game_session *session); // 0x4de4e0, this batch;
    // blam-cc: EAX = entry (unaffected pass-through), ECX = session

// Rejects the '%' and '|' escape characters from a candidate player name, regenerates a
// fresh random name on any collision or reserved character, and assigns a random colour to
// a still-unassigned slot, before handing the finished entry to network_player_entry_add.
// Only runs when the entry's machine_index already matches the caller's machine.
uint32_t network_game_session_finalize_and_add_player(network_player_entry *entry, network_server_globals *server, network_machine *machine)
{
    wchar_t reserved[4];
    wchar_t *hit;

    if (machine->machine_id != (int16_t)entry->machine_index) {
        return (uint32_t)entry & 0xffffff00;
    }
    reserved[0] = L'%';
    reserved[1] = L'\0';
    reserved[2] = L'|';
    reserved[3] = L'\0';
    if (entry->team_index == -1) {
        entry->team_index = (int8_t)game_engine_team_is_leading(0xffffffff);
    }
    if (entry->name[0] == L'\0') {
        network_game_generate_unique_random_name();
    }
    hit = wcsstr((wchar_t *)entry->name, reserved);
    if (hit != 0 || (hit = wcsstr((wchar_t *)entry->name, reserved + 2), hit != 0)) {
        network_game_generate_unique_random_name();
    }
    if (network_player_name_collision_check() == 0) {
        network_game_generate_unique_random_name();
    }
    if (entry->color_index == -1) {
        network_player_assign_random_color();
    }
    return network_player_entry_add(entry, &server->session);
}

#if 0
Original Ghidra decompilation (0x4df840):

uint FUN_004df840(void)

{
  undefined1 uVar1;
  char cVar2;
  wchar_t *in_EAX;
  wchar_t *pwVar3;
  uint uVar4;
  int in_ECX;
  int in_EDX;
  wchar_t local_8 [4];

  if (*(short *)(in_EDX + 0xc) == (short)(char)in_EAX[0xe]) {
    local_8[0] = L'%';
    local_8[1] = L'\0';
    local_8[2] = L'|';
    local_8[3] = L'\0';
    if ((char)in_EAX[0xf] == -1) {
      uVar1 = FUN_00470720(0xffffffff);
      *(undefined1 *)(in_EAX + 0xf) = uVar1;
    }
    if (*in_EAX == L'\0') {
      network_game_generate_unique_random_name();
    }
    pwVar3 = _wcsstr(in_EAX,local_8);
    if ((pwVar3 != (wchar_t *)0x0) ||
       (pwVar3 = _wcsstr(in_EAX,local_8 + 2), pwVar3 != (wchar_t *)0x0)) {
      network_game_generate_unique_random_name();
    }
    cVar2 = FUN_004df6f0();
    if (cVar2 == '\0') {
      network_game_generate_unique_random_name();
    }
    if (in_EAX[0xc] == L'\xffff') {
      FUN_004df790();
    }
    uVar4 = FUN_004de4e0(in_ECX + 8);
    return uVar4;
  }
  return (uint)in_EAX & 0xffffff00;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
