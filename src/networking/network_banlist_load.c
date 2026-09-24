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

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

extern char network_banlist_full_path[0x104];        // 0x0071c308, see sv_banlist_file.c
extern char network_ban_file_read_mode_string[];      // 0x0066d81c, "rt"
extern char network_ban_indefinite_marker[];          // 0x0066b038, "--"

extern char *network_log_path_resolve(char *requested_path); // this module, 0x4e40a0
extern void *FUN_00624186(const char *path, char *mode);     // 0x624186, fopen-shaped CRT wrapper
extern char *FUN_006257e0(char *s, int ch);                  // 0x6257e0, foreign, strchr-shaped
extern void string_trim_whitespace(char **string_ptr);       // this module, 0x4e4040, blam-cc: EDI -> string_ptr
extern ban_list_entry *ban_list_get_or_add_entry(char *name, char *cd_key_hash); // this module, 0x4e3890

// Reloads the server's ban list from banned<suffix>.txt (see sv_banlist_file / network_banlist_save
// for the companion writer). Each non-comment line is "name,cd_key_hash,ban_count,expiry", where
// the expiry is either the literal "--" (indefinite) or "YYYY-MM-DD HH:MM:SS".
void network_banlist_load(void)
{
    FILE *file;
    char line[0x200];
    char *comma;
    char *name_ptr;
    char *hash_ptr;
    char *rest;
    char *count_end;
    char *date_or_dash;
    char date_token[32];
    char time_token[32];
    struct tm parsed_time;
    time_t expiry;
    long ban_count;
    ban_list_entry *entry;

    file = (FILE *)FUN_00624186(network_log_path_resolve(network_banlist_full_path),
                                 network_ban_file_read_mode_string);
    if (file == 0) {
        return;
    }
    while (fgets(line, 0x200, file) != 0) {
        if (line[0] == '#') {
            continue;
        }
        comma = FUN_006257e0(line, ',');
        if (comma == 0) {
            continue;
        }
        *comma = 0;
        name_ptr = line;
        hash_ptr = comma + 1;
        rest = FUN_006257e0(hash_ptr, ',');
        if (rest != 0) {
            *rest = 0;
            rest = rest + 1;
        }
        string_trim_whitespace(&name_ptr);
        string_trim_whitespace(&hash_ptr);
        entry = ban_list_get_or_add_entry(name_ptr, hash_ptr);
        if (entry == 0) {
            continue;
        }
        if (rest == 0) {
            entry->indefinite = 1;
            continue;
        }
        count_end = FUN_006257e0(rest, ',');
        if (count_end == 0) {
            continue;
        }
        *count_end = 0;
        ban_count = atol(rest);
        entry->ban_count = (int16_t)ban_count;
        date_or_dash = count_end + 1;
        if (strncmp(date_or_dash, network_ban_indefinite_marker, 3) == 0) {
            entry->indefinite = 1;
            continue;
        }
        if (sscanf(date_or_dash, "%32s %32s", date_token, time_token) == 2) {
            memset(&parsed_time, 0, sizeof(parsed_time));
            if (sscanf(time_token, "%02d:%02d:%02d", &parsed_time.tm_hour, &parsed_time.tm_min, &parsed_time.tm_sec) == 3 &&
                sscanf(date_token, "%04d-%02d-%02d", &parsed_time.tm_year, &parsed_time.tm_mon, &parsed_time.tm_mday) == 3) {
                parsed_time.tm_year -= 0x76c;
                parsed_time.tm_mon -= 1;
                parsed_time.tm_isdst = -1;
                expiry = mktime(&parsed_time);
                if (expiry != -1) {
                    entry->expiry_time = (int32_t)expiry;
                    continue;
                }
            }
        }
        entry->expiry_time = 0;
        entry->indefinite = 1;
    }
    fclose(file);
}

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
