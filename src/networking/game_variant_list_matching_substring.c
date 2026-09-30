// game_variant_list_matching_substring  (Ghidra: already named)
// address 0x4e4600, size 528 bytes
// name confidence: 0.55   rewrite confidence: 0.4
// evidence: out/phase4/networking_functions.md ("lists available game-variant (gametype) names
// whose lowercased name contains an optional substring argument"); the literal
// "Game types matching substring \"%ls\" :" / "%-36ls " formats.
// register convention: both parameters arrive on the stack, matching map_list_matching_substring.c
// (the same "*_matching_substring" exception documented in sv_maxplayers_evaluate.c).
//   // blam-cc: stack -> argument_count, stack -> arguments
// UNSURE: FUN_0053bee0's exact shape (foreign: given a saved-game id, writes the variant's wide
// name into a caller buffer and returns success); string_format_wide_va_bounded's shape (foreign,
// read here as a bounded ANSI-to-wide vsnprintf-style formatter); saved_game_enumerate_by_type's
// out-array element meaning (-1 sentinel entries are treated as "use the current custom variant").

#include "tags.h"
#include "memory.h"
#include "fn_game.h"
#include "fn_saved_games.h"
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

extern uint8_t playlist_profiles_need_defaults; // 0x0069e8d0

extern void string_format_wide_va_bounded(uint16_t *dest, const char *format, ...); // foreign, UNSURE shape, 0x557910

extern void saved_game_enumerate_by_type(int32_t type, int32_t *out_ids, int32_t flag); // foreign, UNSURE shape, 0x53c4e0
extern uint8_t saved_game_get_variant(int32_t saved_game_id, uint16_t *out_name); // foreign, UNSURE shape

extern void *console_color_00685214; // 0x00685214, a ColorARGB * the original loads into EAX
extern void *actor_mode_default_look_weights; // 0x00686af8, a ColorARGB * the original loads into EAX
extern void chimera__console_out(ColorARGB *color, char *format, ...); // 0x496b50, EAX color (NULL = default)

// Console command: lists installed game-variant names (an optional lowercased filter substring),
// two per output line.
void game_variant_list_matching_substring(uint32_t argument_count, char **arguments) // blam-cc: stack -> argument_count, stack -> arguments
{
    uint16_t filter[32];
    int32_t saved_game_ids[100];
    int32_t i;

    filter[0] = 0;
    if (0 < (int32_t)argument_count) {
        uint16_t *p;
        string_format_wide_va_bounded(filter, "%s", arguments[0]);
        for (p = filter; *p != 0; p = p + 1) {
            *p = towlower(*p);
        }
    }
    chimera__console_out((ColorARGB *)console_color_00685214, "Game types matching substring \"%ls\" :", filter);
    if (playlist_profiles_need_defaults == 1) {
        playlist_profile_create_default_profiles_on_disk();
        playlist_profiles_need_defaults = 0;
    }
    for (i = 0; i < 100; i = i + 1) {
        saved_game_ids[i] = -1;
    }
    saved_game_enumerate_by_type(1, saved_game_ids, 1);
    i = 0;
    do {
        char line[256];
        int32_t on_line = 0;

        line[0] = 0;
        while (i <= 99 && on_line < 2) {
            if (saved_game_ids[i] == -1) {
                game_engine_apply_current_custom_variant();
            } else {
                uint16_t variant_name[64];
                if (saved_game_get_variant(saved_game_ids[i], variant_name) != 0) {
                    uint16_t lowered[64];
                    wcsncpy(lowered, variant_name, 0x3f);
                    lowered[0x3f] = 0;
                    for (uint16_t *p = lowered; *p != 0; p = p + 1) {
                        *p = towlower(*p);
                    }
                    if (filter[0] == 0 || wcsstr(lowered, filter) != 0) {
                        char formatted[64];
                        sprintf(formatted, "%-36ls ", variant_name);
                        strcat(line, formatted);
                        on_line = on_line + 1;
                    }
                }
            }
            i = i + 1;
        }
        if (line[0] != 0) {
            chimera__console_out((ColorARGB *)actor_mode_default_look_weights, line);
        }
        if (99 < i) {
            return;
        }
    } while (1);
}

#if 0
Original Ghidra decompilation (0x4e4600), from tools/pack.py 0x4e4600:

void game_variant_list_matching_substring(int param_1,undefined4 *param_2)

{
  char cVar1;
  wchar_t *pwVar2;
  char *pcVar3;
  uint uVar4;
  int iVar5;
  uint uVar6;
  int iVar7;
  char *pcVar8;
  undefined4 *puVar9;
  int *piVar10;
  wchar_t local_468;
  undefined4 local_466 [31];
  undefined2 local_3ea;
  wchar_t local_3e8;
  undefined4 local_3e6 [31];
  undefined2 local_36a;
  char local_368 [64];
  wchar_t local_328 [75];
  char cStack_291;
  char local_290 [256];
  int local_190 [100];

  local_468 = L'\0';
  ... (zero local_466, local_3e6; fill local_190[100] with -1) ...
  iVar5 = 0;
  if (0 < param_1) {
    string_format_wide_va_bounded(&local_468,&DAT_0066a888,*param_2);
    local_3ea = 0;
    __wcslwr(&local_468);
  }
  chimera__console_out("Game types matching substring \"%ls\" :",&local_468);
  if (DAT_0069e8d0 == '\x01') {
    playlist_profile_create_default_profiles_on_disk();
    DAT_0069e8d0 = '\0';
  }
  saved_game_enumerate_by_type(1,local_190,1);
  do {
    local_290[0] = '\0';
    iVar7 = 0;
    do {
      if (99 < iVar5) break;
      if (local_190[iVar5] == -1) {
        game_engine_apply_current_custom_variant();
      }
      else {
        cVar1 = FUN_0053bee0(local_190[iVar5],local_328);
        if (cVar1 != '\0') {
          _wcsncpy(&local_3e8,local_328,0x3f);
          local_36a = 0;
          __wcslwr(&local_3e8);
          if ((local_468 == L'\0') ||
             (pwVar2 = _wcsstr(&local_3e8,&local_468), pwVar2 != (wchar_t *)0x0)) {
            _sprintf(local_368,"%-36ls ",local_328);
            /* append local_368 to local_290 */
            iVar7 = iVar7 + 1;
          }
        }
      }
      iVar5 = iVar5 + 1;
    } while (iVar7 < 2);
    if (local_290[0] != '\0') {
      chimera__console_out(local_290);
    }
    if (99 < iVar5) {
      return;
    }
  } while( true );
}
#endif
