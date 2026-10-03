# Original notes: networking `net1_banlist`

Author notes and decompile blocks moved verbatim from the C files converted into the net1_banlist sources.

## network_banlist_add_ban.c

```
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
```

```
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
```

## network_banlist_load.c

```
// network_banlist_load  (Ghidra: network_banlist_load, already named)
// address 0x4e3160, size 538 bytes
// name confidence: 0.8   rewrite confidence: 0.75
// evidence: out/phase4/networking_types_notes.md "ban_list_entry" section (0x4e3160 is the
// section's own source for the CSV parse rules); types/networking.h ban_list_entry
// (name/cd_key_hash/ban_count/indefinite/expiry_time); disassembly (objdump -d -M intel,
// bin/halo.exe) for every register-carried argument below, since Ghidra's own decompile drops
// or misattributes most of them (see UNSURE notes).
// register convention: __cdecl, no arguments.
// UNSURE: Ghidra's decompile shows `FUN_004e40a0(&DAT_0066d81c)` for the path-resolve call, but
// disassembly proves this is the same misattribution documented in network_banlist_save.c:
//   4e3167: push 0x66d81c      ; fopen mode "rt", staged early for the later fopen call
//   4e316c: mov esi,0x71c308   ; ESI = &network_banlist_full_path, this function's real argument
//   4e3171: call 0x4e40a0
//   4e3177: call 0x624186      ; fopen(path=eax, mode=0x66d81c)
// so the real call is network_log_path_resolve(network_banlist_full_path) followed by
// FUN_00624186(path, "rt"), and 0x0066d81c is the two-character mode string "rt" (confirmed by
// reading bin/halo.exe's .rdata at that address), not a path.
// UNSURE: Ghidra's decompile shows both `string_trim_whitespace();` calls with no visible
// arguments (`unaff_EDI`/lost register tracking) and then passes the pre-trim `local_200` /
// `puVar3 + 1` expressions straight into ban_list_get_or_add_entry. Disassembly shows the real
// shape: two spilled locals (name_ptr, hash_ptr) are initialized to the name and cd-key-hash
// field starts, `string_trim_whitespace` is called on each spilled slot in place (EDI -> the
// slot, matching string_trim_whitespace's own `char **` convention), and it is the *trimmed*
// values that are read back out of those same slots and passed to ban_list_get_or_add_entry:
//   4e31ca: mov [esp+0x18],ecx      ; name_ptr = &line[0]
//   4e31e5: mov [esp+0x1c],eax      ; hash_ptr = (first comma) + 1
//   4e31fb: lea edi,[esp+0x10] / call 0x4e4040   ; string_trim_whitespace(&name_ptr)
//   4e3204: lea edi,[esp+0x14] / call 0x4e4040   ; string_trim_whitespace(&hash_ptr)
//   4e320d: mov eax,[esp+0x14] / mov ecx,[esp+0x10]   ; reload the *trimmed* pointers
//   4e3217: call ban_list_get_or_add_entry(name=ecx, hash=eax)
// (name_ptr/hash_ptr land at esp+0x10/esp+0x14 by the time of the trim calls and the reload
// because two bytes are pushed as call arguments in between the stores at esp+0x18/esp+0x1c and
// the trim calls, shifting the visible displacement by 8 -- the slots are the same memory).
// UNSURE: Ghidra's pseudo-C has no case for "no third field at all" (no comma after the cd-key
// hash); disassembly shows this is handled explicitly and differently from "third field present
// but not comma-terminated":
//   4e3229: test esi,esi / je 0x4e3343     ; rest == NULL -> straight to entry->indefinite = 1
//   4e323e: je 0x4e3347 (after the count-field strchr fails)  ; rest != NULL but has no comma
//                                                                -> leaves the entry untouched
// preserved exactly below as two distinct outcomes rather than collapsed into one.
// UNSURE: the literal compared against the date/dash field is `network_ban_indefinite_marker`,
// 0x0066b038, read from bin/halo.exe's .rdata as the bytes "--\0"; the comparison is a 3-byte
// `repz cmpsb` (matches "--" followed by a NUL, i.e. the field is exactly "--"), reproduced here
// with strncmp(field, "--", 3) which has the identical short-circuit-on-mismatch behaviour.
// UNSURE: entry->expiry_time is written unconditionally with the (possibly -1) mktime result
// and then overwritten with 0 on the failure path (4e3332 then 4e333c); this rewrite skips the
// redundant intermediate store since it is not observable.
```

```
#if 0
Original Ghidra decompilation (0x4e3160), from tools/pack.py 0x4e3160:

void network_banlist_load(void)

{
  undefined4 uVar1;
  FILE *_File;
  char *pcVar2;
  undefined1 *puVar3;
  char *pcVar4;
  long lVar5;
  int iVar6;
  char *pcVar7;
  bool bVar8;
  time_t tVar9;
  tm local_268;
  FILE *local_244;
  char local_240 [32];
  char local_220 [32];
  char local_200 [512];

  uVar1 = FUN_004e40a0(&DAT_0066d81c);
  _File = (FILE *)FUN_00624186(uVar1);
  if (_File != (FILE *)0x0) {
    local_244 = _File;
    pcVar2 = _fgets(local_200,0x200,_File);
    while (pcVar2 != (char *)0x0) {
      if (local_200[0] != '#') {
        puVar3 = (undefined1 *)FUN_006257e0(local_200,0x2c);
        if (puVar3 != (undefined1 *)0x0) {
          *puVar3 = 0;
          pcVar2 = (char *)FUN_006257e0(puVar3 + 1,0x2c);
          if (pcVar2 != (char *)0x0) {
            *pcVar2 = '\0';
            pcVar2 = pcVar2 + 1;
          }
          string_trim_whitespace();
          string_trim_whitespace();
          pcVar4 = ban_list_get_or_add_entry(local_200,puVar3 + 1);
          if (pcVar4 != (char *)0x0) {
            if (pcVar2 != (char *)0x0) {
              puVar3 = (undefined1 *)FUN_006257e0(pcVar2,0x2c);
              if (puVar3 == (undefined1 *)0x0) goto LAB_004e3347;
              *puVar3 = 0;
              lVar5 = _atol(pcVar2);
              iVar6 = 3;
              bVar8 = true;
              *(short *)(pcVar4 + 0x2e) = (short)lVar5;
              pcVar2 = puVar3 + 1;
              pcVar7 = "--";
              do {
                if (iVar6 == 0) break;
                iVar6 = iVar6 + -1;
                bVar8 = *pcVar2 == *pcVar7;
                pcVar2 = pcVar2 + 1;
                pcVar7 = pcVar7 + 1;
              } while (bVar8);
              if (!bVar8) {
                iVar6 = _sscanf(puVar3 + 1,"%32s %32s",local_220,local_240);
                if (iVar6 == 2) {
                  local_268.tm_min = 0;
                  local_268.tm_hour = 0;
                  local_268.tm_mday = 0;
                  local_268.tm_mon = 0;
                  local_268.tm_year = 0;
                  local_268.tm_wday = 0;
                  local_268.tm_yday = 0;
                  local_268.tm_isdst = 0;
                  local_268.tm_sec = 0;
                  iVar6 = _sscanf(local_240,"%02d:%02d:%02d",&local_268.tm_hour,&local_268.tm_min,
                                  &local_268);
                  if ((iVar6 == 3) &&
                     (iVar6 = _sscanf(local_220,"%04d-%02d-%02d",&local_268.tm_year,
                                      &local_268.tm_mon,&local_268.tm_mday), iVar6 == 3)) {
                    local_268.tm_year = local_268.tm_year + -0x76c;
                    local_268.tm_mon = local_268.tm_mon + -1;
                    local_268.tm_isdst = -1;
                    tVar9 = _mktime(&local_268);
                    *(int *)(pcVar4 + 0x34) = (int)tVar9;
                    if ((int)tVar9 != -1) goto LAB_004e3347;
                  }
                }
                pcVar4[0x34] = '\0';
                pcVar4[0x35] = '\0';
                pcVar4[0x36] = '\0';
                pcVar4[0x37] = '\0';
              }
            }
            pcVar4[0x30] = '\x01';
          }
        }
      }
LAB_004e3347:
      _File = local_244;
      pcVar2 = _fgets(local_200,0x200,local_244);
    }
    _fclose(_File);
  }
  return;
}

Disassembly (objdump -d -M intel, bin/halo.exe) for the register-carried arguments and the real
control flow through the ban-count/date section (esp deltas tracked relative to the loop-top
value at 0x4e31b0, ESP_LOOP):
  4e3167: push 0x66d81c            ; fopen mode "rt", staged early
  4e316c: mov esi,0x71c308         ; ESI = &network_banlist_full_path
  4e3171: call 0x4e40a0            ; network_log_path_resolve(esi)
  4e3177: call 0x624186            ; fopen(path=eax, mode=[0x66d81c])
  4e31be: lea ecx,[esp+0x80]       ; &line[0]
  4e31ca: mov [esp+0x18],ecx       ; -> ESP_LOOP+0x10 once the two pending pushes are counted:
                                   ;    name_ptr = &line[0]
  4e31e5: mov [esp+0x1c],eax       ; -> ESP_LOOP+0x14: hash_ptr = (first comma)+1
  4e31fb: lea edi,[esp+0x10]       ; edi = ESP_LOOP+0x10 = &name_ptr
  4e31ff: call 0x4e4040            ; string_trim_whitespace(&name_ptr)
  4e3204: lea edi,[esp+0x14]       ; edi = ESP_LOOP+0x14 = &hash_ptr
  4e3208: call 0x4e4040            ; string_trim_whitespace(&hash_ptr)
  4e320d: mov eax,[esp+0x14]       ; reload trimmed hash_ptr
  4e3211: mov ecx,[esp+0x10]       ; reload trimmed name_ptr
  4e3217: call 0x4e3890            ; ban_list_get_or_add_entry(name=ecx, hash=eax)
  4e3229: test esi,esi             ; esi = rest (second-comma+1), or 0 if no second comma
  4e322b: je 0x4e3343              ; rest == 0 -> entry->indefinite = 1 directly
  4e323e: je 0x4e3347              ; rest != 0 but no third comma -> leave entry untouched
  4e3253: mov edi,0x66b038         ; "--\0" literal
  4e3265: repz cmps ...            ; compare date_or_dash[0..2] against "--\0"
  4e3267: je 0x4e3343              ; match -> entry->indefinite = 1
  (sscanf/mktime section confirms struct tm base = ESP_LOOP+0x18: tm_sec=+0x00, tm_min=+0x04,
   tm_hour=+0x08, tm_mday=+0x0c, tm_mon=+0x10, tm_year=+0x14, tm_wday=+0x18, tm_yday=+0x1c,
   tm_isdst=+0x20, matching the standard MSVCRT struct tm layout exactly)
  4e332f: cmp eax,0xffffffff / 4e333a: jne 0x4e3347   ; mktime succeeded -> skip indefinite=1
#endif
```

## network_banlist_print.c

```
// network_banlist_print  (Ghidra: FUN_004e34e0; renamed -- console command that lists bans)
// address 0x4e34e0, size 210 bytes
// name confidence: 0.6   rewrite confidence: 0.65
// evidence: out/phase4/networking_functions.md ("Console command that lists the currently
// banned players with their ban counts"); the literal header string "[Num Bans Name]" and row
// format "[%-4d %2d %*s]" (index, ban count, 12-wide name); types/networking.h ban_list_entry.
// register convention: __cdecl, no arguments.
// UNSURE: the destination line buffer is built one byte past its start in the original
// decompile (acStack_101[1] = 0, then every append walks from acStack_101 to find that NUL and
// the whole buffer is printed from +1 on), which only matters because byte 0 is left
// uninitialized and never printed; this rewrite uses an ordinary NUL-terminated-at-0 buffer,
// which produces byte-for-byte identical console output.
```

```
#if 0
Original Ghidra decompilation (0x4e34e0), from tools/pack.py 0x4e34e0:

void FUN_004e34e0(void)

{
  char cVar1;
  char *pcVar2;
  uint uVar3;
  uint uVar4;
  int iVar5;
  int iVar6;
  char *pcVar7;
  int local_144;
  char local_140 [63];
  char acStack_101 [257];

  iVar6 = 0;
  chimera__console_out("[Num Bans Name]");
  if (0 < DAT_006b85a0) {
    do {
      acStack_101[1] = 0;
      iVar5 = iVar6 * 0x38;
      local_144 = 0;
      do {
        if (DAT_006b85a0 <= iVar6) break;
        _sprintf(local_140,"[%-4d %2d %*s]",iVar6,(int)*(short *)(DAT_006b85a4 + 0x2e + iVar5),0xc,
                 DAT_006b85a4 + iVar5);
        pcVar2 = local_140;
        do {
          cVar1 = *pcVar2;
          pcVar2 = pcVar2 + 1;
        } while (cVar1 != '\0');
        uVar3 = (int)pcVar2 - (int)local_140;
        pcVar2 = acStack_101;
        do {
          pcVar7 = pcVar2 + 1;
          pcVar2 = pcVar2 + 1;
        } while (*pcVar7 != '\0');
        pcVar7 = local_140;
        for (uVar4 = uVar3 >> 2; uVar4 != 0; uVar4 = uVar4 - 1) {
          *(undefined4 *)pcVar2 = *(undefined4 *)pcVar7;
          pcVar7 = pcVar7 + 4;
          pcVar2 = pcVar2 + 4;
        }
        local_144 = local_144 + 1;
        iVar6 = iVar6 + 1;
        iVar5 = iVar5 + 0x38;
        for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
          *pcVar2 = *pcVar7;
          pcVar7 = pcVar7 + 1;
          pcVar2 = pcVar2 + 1;
        }
      } while (local_144 < 3);
      chimera__console_out(acStack_101 + 1);
    } while (iVar6 < DAT_006b85a0);
  }
  return;
}
#endif
```

## network_banlist_save.c

```
// network_banlist_save  (Ghidra: network_banlist_save, already named)
// address 0x4e3380, size 348 bytes
// name confidence: 0.8   rewrite confidence: 0.7
// evidence: out/phase4/networking_types_notes.md "ban list" section; types/networking.h
// ban_list_entry (name/cd_key_hash/ban_count/indefinite/expiry_time); the CSV header string and
// the "%s,%s,%d," / "--\r\n" / "%s %s\r\n" row formats.
// register convention: __cdecl, no arguments.
// UNSURE: the growable_array this function reads is declared in types/networking.h as
// "global 0x006b85a0: growable_array ban_list", but the code here reads a plain count at
// 0x006b85a0 and a plain data pointer at 0x006b85a4 -- which only match memory.h's canonical
// growable_array {element_size, count, data} layout if the struct actually starts 4 bytes
// earlier, at 0x006b859c (count = base+4 = 0x6b85a0, data = base+8 = 0x6b85a4). That base is
// also where ban_list_get_or_add_entry.c's growable_array_add_element(&ban_list) call needs ESI
// to point. Declared that way here rather than redefining the header's comment.
```

```
#if 0
Original Ghidra decompilation (0x4e3380), from tools/pack.py 0x4e3380:

void network_banlist_save(void)

{
  int iVar1;
  undefined4 uVar2;
  FILE *_File;
  tm *ptVar3;
  int iVar4;
  int local_6c;
  undefined4 local_68;
  char local_64 [31];
  undefined1 local_45;
  char local_44 [31];
  undefined1 local_25;
  tm local_24;

  uVar2 = FUN_004e40a0(&DAT_0065fd30);
  _File = (FILE *)FUN_00624186(uVar2);
  if (_File != (FILE *)0x0) {
    _fprintf(_File,"# Name, CD key hash, ban count, ban end date\r\n");
    local_6c = 0;
    if (0 < DAT_006b85a0) {
      iVar4 = 0;
      do {
        iVar1 = DAT_006b85a4 + iVar4;
        _fprintf(_File,"%s,%s,%d,",iVar1,iVar1 + 0xd,(int)*(short *)(DAT_006b85a4 + 0x2e + iVar4));
        if (*(char *)(iVar1 + 0x30) == '\0') {
          local_68 = *(undefined4 *)(iVar1 + 0x34);
          local_24.tm_min = 0;
          local_24.tm_hour = 0;
          local_24.tm_mday = 0;
          local_24.tm_mon = 0;
          local_24.tm_year = 0;
          local_24.tm_wday = 0;
          local_24.tm_yday = 0;
          local_24.tm_sec = 0;
          local_24.tm_isdst = 0;
          ptVar3 = _localtime((time_t *)&local_68);
          if (ptVar3 == (tm *)0x0) {
            ptVar3 = &local_24;
          }
          __snprintf(local_64,0x1f,"%02d:%02d:%02d",ptVar3->tm_hour,ptVar3->tm_min,ptVar3->tm_sec);
          local_45 = 0;
          __snprintf(local_44,0x1f,"%04d-%02d-%02d",ptVar3->tm_year + 0x76c,ptVar3->tm_mon + 1,
                     ptVar3->tm_mday);
          local_25 = 0;
          _fprintf(_File,"%s %s\r\n",local_44,local_64);
        }
        else {
          _fprintf(_File,"--\r\n");
        }
        local_6c = local_6c + 1;
        iVar4 = iVar4 + 0x38;
      } while (local_6c < DAT_006b85a0);
    }
    _fclose(_File);
  }
  return;
}
#endif
```

## network_session_autoban_player.c

```
// network_session_autoban_player  (Ghidra: FUN_004e36e0; renamed, suggested by
// out/phase2/results/networking_04.json)
// address 0x4e36e0, size 233 bytes
// name confidence: 0.45   rewrite confidence: 0.65
// evidence: out/phase2/results/networking_04.json 0x4e36e0: "Resolves a player-index parameter
// to a session machine slot, skips already-disconnecting machines, logs 'AUTOBAN: Banning %S.',
// then calls network_banlist_add_ban followed by FUN_004e0af0 to disconnect/kick the machine."
// types/networking.h network_machine (machine_id at +0x0c, channel at +0x00), player (identifier
// at +0x00, name at +0x04, unknown_64 at +0x64); disassembly (objdump -d -M intel, bin/halo.exe)
// for the register convention and for the field this batch had not otherwise resolved.
// register convention: ECX -> player_handle. Confirmed by disassembly: `cmp ecx,0xffffffff`
// against the incoming register at function entry, with no stack args.
//   // blam-cc: ECX -> player_handle
// UNSURE: `movsx bx, BYTE PTR [edx+0x64]` reads only the low BYTE of player::unknown_64 (declared
// int16_t in types/game.h) and sign-extends it; modeled here as a byte read through the address
// of that field rather than redeclaring it, since the field's true width/meaning is not resolved
// by this batch. It is then compared against network_machine::machine_id (also int16_t) to find
// the owning machine slot, which is the closest thing to an established use of it in this module.
// UNSURE (load-bearing, preserved as-is): disassembly shows the "channel is still connected ->
// refuse" guard (`test bl,bl / jne`, reading channel->connected) only runs on the path where a
// matching machine slot with a non-NULL channel was actually found. When the loop exhausts all
// 16 machines without a match, execution still falls through to the AUTOBAN print and to
// `machine->unknown_5c` for the identity_lookup_key -- with `machine` NULL. This looks like a
// genuine latent null-pointer read in the original binary (only survivable if this function is
// never actually called for a player with no matching machine); it is preserved exactly rather
// than guarded, per the task's "no invented behaviour" rule.
// UNSURE: the player object's address 4 bytes in (`&target_player->name[0]`) is passed as the
// `network_player_entry *target_player` argument of network_banlist_add_ban, exactly as
// network_banlist_add_ban.c already declares it; only the leading UTF-16 name field of either
// struct is ever read by that callee's callee (FUN_00557950), and both structs happen to have
// their name array at that argument's offset 0, so this is safe punning rather than a real type
// match. See network_banlist_add_ban.c's own header for the full chain.
// UNSURE: FUN_004e0af0's reason code 6 matches sv_ban.c's own call to the same function with the
// same reason, and its blam-cc parameter shape (ECX -> reason, EDI -> machine, stack -> server)
// is copied from that already-resolved file; EDI is callee-saved under cdecl, so it still holds
// the machine pointer found by the loop above by the time this call executes.
```

```
#if 0
Original Ghidra decompilation (0x4e36e0), from tools/pack.py 0x4e36e0:

undefined4 FUN_004e36e0(void)

{
  int *piVar1;
  char cVar2;
  short sVar3;
  int in_ECX;
  int iVar4;
  int iVar5;
  short *psVar6;
  short sVar7;

  if (((in_ECX != -1) && (sVar3 = (short)in_ECX, -1 < sVar3)) &&
     (sVar3 < *(short *)(DAT_0087a480 + 0x20))) {
    iVar5 = (int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3;
    sVar3 = *(short *)(iVar5 + *(int *)(DAT_0087a480 + 0x34));
    iVar5 = iVar5 + *(int *)(DAT_0087a480 + 0x34);
    if ((sVar3 != 0) && ((sVar7 = (short)((uint)in_ECX >> 0x10), sVar7 == 0 || (sVar3 == sVar7))))
    {
      iVar4 = 0;
      psVar6 = (short *)(DAT_0071c2d4 + 0x3c4);
      do {
        if (*psVar6 == (short)*(char *)(iVar5 + 100)) {
          piVar1 = (int *)(iVar4 * 0x60 + 0x3b8 + DAT_0071c2d4);
          if (((piVar1 != (int *)0x0) && (iVar4 = *piVar1, iVar4 != 0)) &&
             (*(char *)(iVar4 + 0xa98) != '\0')) {
            return 0;
          }
          break;
        }
        iVar4 = iVar4 + 1;
        psVar6 = psVar6 + 0x30;
      } while (iVar4 < 0x10);
      iVar5 = iVar5 + 4;
      chimera__console_out("AUTOBAN: Banning %S.",iVar5);
      cVar2 = network_banlist_add_ban(iVar5);
      if ((cVar2 != '\0') && (cVar2 = FUN_004e0af0(DAT_0071c2d4), cVar2 != '\0')) {
        return 1;
      }
    }
  }
  return 0;
}

Disassembly (objdump -d -M intel, bin/halo.exe), the register-carried argument and the loop over
network_server->machines[]:
  4e36e2: cmp ecx,0xffffffff        ; player_handle == k_datum_index_none
  4e36ee: sar edi,0x10              ; salt = (int16_t)(player_handle >> 16)
  4e36fb: mov esi,[0x87a480]        ; esi = player_data
  4e3719: mov cx,[edx+ebx]          ; target_player->identifier, edx = target_player
  4e3736: movsx bx,byte ptr [edx+0x64]  ; machine_key = low byte of target_player->unknown_64
  4e373c: mov ebp,[0x71c2d4]        ; ebp = network_server
  4e3746: lea esi,[ebp+0x3c4]       ; &network_server->machines[0].machine_id
  4e3750-4e375c: loop, stride 0x60, 16 iterations
  4e3760-4e376d: edi = &network_server->machines[i] on match
  4e3777: mov bl,[ecx+0xa98]        ; machine->channel->connected
  4e3781: lea esi,[edx+0x4]         ; &target_player->name[0]
  4e3791: mov eax,[edi+0x5c]        ; identity_lookup_key = machine->unknown_5c (edi may be NULL
                                    ; here if the loop found no match -- see UNSURE note above)
  4e37aa: mov ecx,0x6 / 4e37af: call 0x4e0af0   ; FUN_004e0af0(6, edi=machine, [0x71c2d4]=server)
#endif
```
