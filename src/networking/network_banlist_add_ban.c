// network_banlist_add_ban  (Ghidra: network_banlist_add_ban, already named)
// address 0x4e35c0, size 276 bytes
// name confidence: 0.5   rewrite confidence: 0.6
// evidence: out/phase4/networking_types_notes.md "ban_list_entry" and "network_channel"
// (rcon/console connection id 0x0069fdfc) sections; types/networking.h sv_ban_penalty_seconds.
// Disassembly (objdump -d -M intel) pins the register convention: `mov ebx,ecx` at entry, then
// `push eax; push ecx=[0x69fdfc]; call gcd_getkeyhash` (cdecl pushes reversed: gcd_getkeyhash's
// first argument is the connection id global, second is the caller's own incoming EAX), and the
// caller (sv_ban.c, this batch) does `mov eax,[edi+0x5c]` (a network_machine field this batch
// does not otherwise resolve) then `push ebx` (the found network_player_entry*) immediately
// before `mov ecx,ebp` (parsed duration) and the call -- so the stack argument is the player
// record, not a bare index.
// register convention: EAX -> identity_lookup_key, ECX -> duration_override_seconds (0 = look up
// the escalating penalty table), stack -> target_player.
//   // blam-cc: EAX -> identity_lookup_key, ECX -> duration_override_seconds, stack -> target_player
// UNSURE: identity_lookup_key's real meaning -- it is network_machine->unknown_5c (not resolved
// by this batch) forwarded straight through to the foreign gcd_getkeyhash (< this module's start),
// which this rewrite treats as "resolve a CD-key hash string for this connection". UNSURE:
// FUN_00557950's exact semantics (this batch does not resolve it); disassembly shows it takes
// the destination buffer in ESI, the player record in EDI (the stack argument here, forwarded
// unchanged) and a byte count of 0x18 on the stack, consistent with a wide-to-ANSI copy of
// network_player_entry::name.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <time.h>

extern int32_t network_console_connection_id; // 0x0069fdfc, "the rcon/console connection id" (networking.h)
extern int32_t sv_ban_penalty_seconds[4]; // 0x00699574

extern char *gcd_getkeyhash(int32_t connection_id, int32_t identity_lookup_key); // foreign (< this module), CD-key hash lookup
extern void string_convert_unicode_to_ascii(char *dest, network_player_entry *player, int32_t dest_size);
    // foreign (this module, later batch), blam-cc: ESI -> dest, EDI -> player, stack -> dest_size
extern void network_banlist_load(void);  // this module, 0x4e3160 (excluded from this batch)
extern ban_list_entry *ban_list_get_or_add_entry(char *name, char *cd_key_hash); // this batch, 0x4e3890
extern void network_banlist_save(void); // this batch, 0x4e3380
extern void format_local_time_and_date(char *date_dest, int32_t max_len, int32_t time_value,
    char *time_dest); // this batch, 0x4e52c0
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Bans target_player: resolves their CD-key hash and display name, adds or extends their ban
// list entry (escalating through sv_ban_penalty_seconds when duration_override_seconds is 0,
// or indefinitely once the tier table runs out at 4), and saves the list.
uint8_t network_banlist_add_ban(int32_t identity_lookup_key, int32_t duration_override_seconds,
    network_player_entry *target_player)
    // blam-cc: EAX -> identity_lookup_key, ECX -> duration_override_seconds, stack -> target_player
{
    char *cd_key_hash;
    char player_name[24]; // FUN_00557950 writes up to 0x18 bytes
    ban_list_entry *entry;
    time_t now;
    time_t expiry;
    char time_buf[32];
    char date_buf[32];

    cd_key_hash = gcd_getkeyhash(network_console_connection_id, identity_lookup_key);
    if (cd_key_hash == 0 || *cd_key_hash == 0) {
        return 1;
    }
    string_convert_unicode_to_ascii(player_name, target_player, 0x18);
    player_name[0xc] = 0;
    network_banlist_load();
    entry = ban_list_get_or_add_entry(player_name, cd_key_hash);
    if (entry != 0) {
        if (duration_override_seconds == 0) {
            if (entry->ban_count < 4) {
                duration_override_seconds = sv_ban_penalty_seconds[entry->ban_count];
            } else {
                duration_override_seconds = -1;
            }
        }
        entry->ban_count = entry->ban_count + 1;
        if (duration_override_seconds == -1) {
            entry->indefinite = 1;
            entry->expiry_time = 0;
            chimera__console_out((ColorARGB *)0, "Banning %s (%s) indefinitely.", player_name, cd_key_hash);
            network_banlist_save();
            return 1;
        }
        time(&now);
        expiry = now + duration_override_seconds;
        entry->expiry_time = expiry;
        entry->indefinite = 0;
        format_local_time_and_date(date_buf, 0x20, expiry, time_buf);
        chimera__console_out((ColorARGB *)0, "Banning %s (%s) until %s %s.", player_name, cd_key_hash, date_buf, time_buf);
        network_banlist_save();
    }
    return 1;
}

#if 0
Original Ghidra decompilation (0x4e35c0), from tools/pack.py 0x4e35c0:

undefined4 network_banlist_add_ban(void)

{
  char *pcVar1;
  char *pcVar2;
  int in_ECX;
  int local_54;
  char local_50 [12];
  undefined1 local_44;
  undefined1 local_40 [32];
  undefined1 local_20 [32];

  pcVar1 = (char *)FUN_0061aa50(DAT_0069fdfc);
  if ((pcVar1 == (char *)0x0) || (*pcVar1 == '\0')) {
    return 1;
  }
  FUN_00557950(0x18);
  local_44 = 0;
  network_banlist_load();
  pcVar2 = ban_list_get_or_add_entry(local_50,pcVar1);
  if (pcVar2 != (char *)0x0) {
    if (in_ECX == 0) {
      if (*(short *)(pcVar2 + 0x2e) < 4) {
        in_ECX = (&DAT_00699574)[*(short *)(pcVar2 + 0x2e)];
      }
      else {
        in_ECX = -1;
      }
    }
    *(short *)(pcVar2 + 0x2e) = *(short *)(pcVar2 + 0x2e) + 1;
    if (in_ECX == -1) {
      pcVar2[0x30] = '\x01';
      pcVar2[0x34] = '\0';
      pcVar2[0x35] = '\0';
      pcVar2[0x36] = '\0';
      pcVar2[0x37] = '\0';
      chimera__console_out("Banning %s (%s) indefinitely.",local_50,pcVar1);
      network_banlist_save();
      return 1;
    }
    FID_conflict___time32(&local_54);
    local_54 = local_54 + in_ECX;
    *(int *)(pcVar2 + 0x34) = local_54;
    pcVar2[0x30] = '\0';
    format_local_time_and_date(local_54,local_40);
    chimera__console_out("Banning %s (%s) until %s %s.",local_50,pcVar1,local_20,local_40);
    network_banlist_save();
  }
  return 1;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the register convention:
  4e35c0: mov ebx,ecx                 ; ebx = duration_override_seconds (ECX)
  4e35c7: mov ecx,[0x69fdfc]          ; ecx = rcon_connection_id
  4e35ce: push eax                    ; FUN_0061aa50's 2nd arg = identity_lookup_key (EAX)
  4e35cf: push ecx                    ; FUN_0061aa50's 1st arg = rcon_connection_id
  4e35d1: call FUN_0061aa50
  ...
  4e35ed: mov edi,[esp+0x68]          ; edi = target_player, this function's stack argument
  4e35f1: push 0x18
  4e35f3: lea esi,[esp+0x18]          ; esi = &player_name
  4e35f7: call FUN_00557950
And sv_ban.c's call site:
  4e3a1a: mov eax,[edi+0x5c]          ; edi = network_machine* from FUN_004e0810
  4e3a1d: push ebx                    ; ebx = target_player (network_player_entry*)
  4e3a1e: mov ecx,ebp                 ; ebp = parsed duration, or 0
  4e3a20: call network_banlist_add_ban
#endif
