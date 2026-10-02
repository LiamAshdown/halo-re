// player_help_screen_select_by_name  (Ghidra: player_help_screen_select_by_name, already named)
// address 0x4991f0, size 496 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: matches the given name; lowercases the current profile's name (tag_instances[...]
// .path, per types/cache.h) and matches it against ten hardcoded substrings (a bespoke "which
// player-help screen matches this player's name" easter egg/test hook), opening the
// corresponding ui\shell\solo_game\player_help\player_help_screen_* tag and writing the given
// value into its first text_box child's selection_index.
// register convention: cdecl, the recognized stack parameter.
// UNSURE: the lowercase loop's Ghidra pseudo-C (`while (local_100[0] != 0) { ...; local_100[0] =
// *pbVar1; }`) reads as re-testing the buffer's first byte every iteration; disassembly
// (0x499240..0x499254) shows the real loop peeks at `*(p+1)` before advancing p, i.e. an
// ordinary in-place lowercase of the whole NUL-terminated string -- written as the latter.
// TYPES-GAP: strstr (0x625430) is not otherwise named in this session; modeled as a
// case-sensitive substring search (haystack, needle) returning nonzero on a match, matching how
// its result is used here (the input is already lowercased, so it functions as case-insensitive
// substring matching against the lowercase needles).

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "cache.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index global_scenario_index; // 0x0069e8d4, UNSURE name
extern tag_instance *tag_instances; // 0x0087bc14
extern int16_t profile_slot_id[]; // 0x00714dde, per types/interface.h globals list (profile_slot_id[])

extern char player_help_name_a10[]; // 0x00669a5c FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_a30[]; // 0x00669a58 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_a50[]; // 0x00669a54 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_b30[]; // 0x00669a50 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_b40[]; // 0x00669a4c FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_c10[]; // 0x00669a48 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_c20[]; // 0x00669a44 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_c40[]; // 0x00669a40 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_d20[]; // 0x00669a3c FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)
extern char player_help_name_d40[]; // 0x00669a38 FIXED: an array (the binary pushes the ADDRESS as an immediate; a pointer declaration loaded the string bytes)

extern widget_instance *chimera__load_ui_widget(char *tag_path, datum_index tag_index,
    widget_instance *parent, uint16_t controller_index, datum_index history_definition,
    datum_index history_list_definition, int16_t history_selection); // 0x497a70, 7 stack args (objdump)

// Selects and opens a specific player-help screen tag based on matching the current profile's
// name against hardcoded name lists, then stashes `value` into the opened widget's first
// text_box child (selection_index).
void player_help_screen_select_by_name(int16_t value)
{
    char name[256];
    char *p;
    char *tag_path;
    widget_instance *dialog;

    if (global_scenario_index == (datum_index)-1) {
        return;
    }
    strncpy(name, tag_instances[(int16_t)global_scenario_index].path, 0xff);

    for (p = name; *p != 0; p++) {
        *p = (char)tolower((uint8_t)*p);
    }

    if (strstr(name, player_help_name_a10) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_a10";
    } else if (strstr(name, player_help_name_a30) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_a30";
    } else if (strstr(name, player_help_name_a50) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_a50";
    } else if (strstr(name, player_help_name_b30) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_b30";
    } else if (strstr(name, player_help_name_b40) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_b40";
    } else if (strstr(name, player_help_name_c10) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_c10";
    } else if (strstr(name, player_help_name_c20) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_c20";
    } else if (strstr(name, player_help_name_c40) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_c40";
    } else if (strstr(name, player_help_name_d20) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_d20";
    } else if (strstr(name, player_help_name_d40) != 0) {
        tag_path = (char *)"ui\\shell\\solo_game\\player_help\\player_help_screen_d40";
    } else {
        return;
    }

    dialog = chimera__load_ui_widget(tag_path, (datum_index)-1, (widget_instance *)0,
                                     (uint16_t)profile_slot_id[0], (datum_index)-1, (datum_index)-1, -1);
    if (dialog != (widget_instance *)0) {
        widget_instance *child;

        for (child = dialog->first_child; child != (widget_instance *)0 && child->widget_type != 1;
             child = child->next_sibling) {
        }
        if (child != (widget_instance *)0) {
            child->selection_index = value;
        }
    }
}

#if 0
Original Ghidra decompilation (0x4991f0):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void player_help_screen_select_by_name(undefined2 param_1)

{
  byte *pbVar1;
  int iVar2;
  char *pcVar3;
  byte *pbVar4;
  byte local_100 [255];
  undefined1 local_1;

  if (DAT_0069e8d4 != -1) {
    _strncpy((char *)local_100,*(char **)((short)DAT_0069e8d4 * 0x20 + 0x10 + DAT_0087bc14),0xff);
    local_1 = 0;
    pbVar4 = local_100;
    while (local_100[0] != 0) {
      iVar2 = _tolower((uint)*pbVar4);
      *pbVar4 = (byte)iVar2;
      pbVar1 = pbVar4 + 1;
      pbVar4 = pbVar4 + 1;
      local_100[0] = *pbVar1;
    }
    iVar2 = FUN_00625430(local_100,&DAT_00669a5c);
    if (iVar2 == 0) {
      iVar2 = FUN_00625430(local_100,&DAT_00669a58);
      if (iVar2 == 0) {
        iVar2 = FUN_00625430(local_100,&DAT_00669a54);
        if (iVar2 == 0) {
          iVar2 = FUN_00625430(local_100,&DAT_00669a50);
          if (iVar2 == 0) {
            iVar2 = FUN_00625430(local_100,&DAT_00669a4c);
            if (iVar2 == 0) {
              iVar2 = FUN_00625430(local_100,&DAT_00669a48);
              if (iVar2 == 0) {
                iVar2 = FUN_00625430(local_100,&DAT_00669a44);
                if (iVar2 == 0) {
                  iVar2 = FUN_00625430(local_100,&DAT_00669a40);
                  if (iVar2 == 0) {
                    iVar2 = FUN_00625430(local_100,&DAT_00669a3c);
                    if (iVar2 == 0) {
                      iVar2 = FUN_00625430(local_100,&DAT_00669a38);
                      if (iVar2 == 0) {
                        return;
                      }
                      pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_d40";
                    }
                    else {
                      pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_d20";
                    }
                  }
                  else {
                    pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c40";
                  }
                }
                else {
                  pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c20";
                }
              }
              else {
                pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_c10";
              }
            }
            else {
              pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_b40";
            }
          }
          else {
            pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_b30";
          }
        }
        else {
          pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a50";
        }
      }
      else {
        pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a30";
      }
    }
    else {
      pcVar3 = "ui\\shell\\solo_game\\player_help\\player_help_screen_a10";
    }
    iVar2 = chimera__load_ui_widget
                      (pcVar3,0xffffffff,0,_DAT_00714dde,0xffffffff,0xffffffff,0xffffffff);
    if (iVar2 != 0) {
      for (iVar2 = *(int *)(iVar2 + 0x34); (iVar2 != 0 && (*(short *)(iVar2 + 0xe) != 1));
          iVar2 = *(int *)(iVar2 + 0x2c)) {
      }
      *(undefined2 *)(iVar2 + 0x40) = param_1;
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
