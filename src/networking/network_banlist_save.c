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

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include <stdio.h>
#include <time.h>

extern growable_array ban_list; // 0x006b859c, element_size 0x38; see UNSURE note above
extern char network_ban_file_mode_string[]; // 0x0065fd30, UNSURE: exact text unresolved (fopen mode)

extern char *network_log_path_resolve(char *requested_path); // this batch, 0x4e40a0
extern char network_banlist_full_path[]; // 0x0071c308, built by sv_banlist_file
// fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper

// Writes the whole in-memory ban list back out to banned<suffix>.txt as CSV: name, cd key hash,
// ban count, then either "--" for an indefinite ban or a "YYYY-MM-DD HH:MM:SS" expiry date.
void network_banlist_save(void)
{
    FILE *file;
    ban_list_entry *entries;
    int32_t i;
    int32_t row;
    struct tm zero_tm;
    struct tm *tm_now;
    char time_buf[31];
    char date_buf[31];
    time_t expiry;

    file = (FILE *)fopen(network_log_path_resolve(network_banlist_full_path),
                                 network_ban_file_mode_string);
    if (file != 0) {
        fprintf(file, "# Name, CD key hash, ban count, ban end date\r\n");
        entries = (ban_list_entry *)ban_list.data;
        row = 0;
        if (0 < ban_list.count) {
            i = 0;
            do {
                ban_list_entry *entry = &entries[i];
                fprintf(file, "%s,%s,%d,", entry->name, entry->cd_key_hash, entry->ban_count);
                if (entry->indefinite == 0) {
                    expiry = entry->expiry_time;
                    zero_tm.tm_min = 0;
                    zero_tm.tm_hour = 0;
                    zero_tm.tm_mday = 0;
                    zero_tm.tm_mon = 0;
                    zero_tm.tm_year = 0;
                    zero_tm.tm_wday = 0;
                    zero_tm.tm_yday = 0;
                    zero_tm.tm_sec = 0;
                    zero_tm.tm_isdst = 0;
                    tm_now = localtime(&expiry);
                    if (tm_now == 0) {
                        tm_now = &zero_tm;
                    }
                    snprintf(time_buf, 0x1f, "%02d:%02d:%02d", tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
                    time_buf[0x1f] = 0;
                    snprintf(date_buf, 0x1f, "%04d-%02d-%02d", tm_now->tm_year + 0x76c, tm_now->tm_mon + 1, tm_now->tm_mday);
                    date_buf[0x1f] = 0;
                    fprintf(file, "%s %s\r\n", date_buf, time_buf);
                } else {
                    fprintf(file, "--\r\n");
                }
                row = row + 1;
                i = i + 1;
            } while (row < ban_list.count);
        }
        fclose(file);
    }
}

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
