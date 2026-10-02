// game_engine_build_end_game_result_text  (Ghidra: FUN_0045cf30; named per
// out/phase4/game_functions.md)
// address 0x45cf30, size 1286 bytes
// VERIFIED against disassembly 0x45cf30..0x45d436 (2026-09-30)
// name confidence: 0.45   rewrite confidence: 0.9
// evidence: out/phase4/game_functions.md ("Builds the localized end-of-game result text (e.g.
// who is leading or tied) by comparing player or team scores through the active game engine's
// callbacks"); types/game.h game_variant (lives_per_round +0x50, teams +0x34),
// game_engine_state, game_engine_definition (is_winner +0x8c, get_team_score +0x50, build_player_text +0x54,
// build_team_score_text +0x5c); scoreboard_entry (place +0x18, tie bit 0x80000000).
// register convention: none; both parameters are stack args (player handle, out wchar_t[0x50]).
// REWRITTEN 2026-09-30 from the disassembly. Every string index is now pinned (the draft guessed indices 0..11 through
// a mis-modelled helper): every lookup is `tag_lookup('ustr', "ui\\multiplayer_game_text")` + text_string_list_get_string(tag, N)
// with N = 0x34 / 0x35 / 0x36 (no lives / one life / N lives, the last one a format string), 0x37 (no clear leader, read
// straight from the tag data), 0x38 / 0x39 (result 0 with / without teams), 0x3a / 0x3b (result 1 with / without teams),
// 0x3c / 0x3d / 0x3e (team game: team 0 ahead / team 1 ahead / tied), 0x3f / 0x40 (player line, tied / not tied). Other
// corrections: game_engine_is_object_winning takes the player handle in EAX; string_format_wide_va_bounded takes its
// character count in EDX (0x80 for the lives text, 0x50 otherwise); the player line has FOUR arguments (place string from
// game_engine_get_default_multiplayer_string(&entry) [0x45ce90 is really get_place_string], the player text, the lives text) and
// the team lines list the leading team first (team 1 first when it is ahead); the team text buffers are only 0x1c bytes.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>

extern data_array *player_data;                     // 0x0087a480
extern tag_instance *tag_instances;                  // 0x0087bc14
extern game_engine_definition *current_game_engine;  // 0x006f1d20
extern game_engine_state game_engine_state_value;    // 0x0087aa10
extern game_variant game_engine_variant;             // 0x006f1c88
extern wchar_t empty_string;                          // 0x00660c34
// 0x00671fac is not an empty string and not a pointer: it holds the characters of
// L"<missing string>", the engine-wide placeholder a failed unicode_string_list lookup
// falls back to. Ghidra prints it as `&PTR_DAT_00671fac` only because its first four
// bytes ("<m") happen to look like a pointer value.
extern wchar_t missing_string_text[];          // 0x00671fac, L"<missing string>"

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0, blam-cc: ECX tag_id, DX index
extern void string_format_wide_va_bounded(uint32_t count, wchar_t *dest, const wchar_t *format, ...); // 0x557910, EDX count
extern uint32_t game_engine_is_object_winning(uint32_t handle); // 0x463660, blam-cc: EAX handle
extern wchar_t *game_engine_get_default_multiplayer_string(const scoreboard_entry *entry); // 0x45ce90 (really get_place_string), EAX entry
extern void game_engine_get_player_scoreboard_entry(datum_index player_handle, scoreboard_entry *out); // 0x45cee0, blam-cc: EAX player, EBX out

// tag_lookup("ui\\multiplayer_game_text") + text_string_list_get_string(tag, index), or the empty string when the tag is missing.
static wchar_t *multiplayer_game_text_string(int16_t index)
{
    datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr'

    if (tag_id == k_datum_index_none) {
        return &empty_string;
    }
    return text_string_list_get_string(tag_id, index);
}

// blam-cc: none; param_1 (player) and param_2 (out, wchar_t[0x50]) are stack args
// Builds the localized "who is leading / tied / how many lives left" line for the end-of-game
// or in-progress scoreboard, using the active game engine's score-comparison callbacks.
void game_engine_build_end_game_result_text(datum_index player_handle, wchar_t *out)
{
    wchar_t *lives_text = &empty_string;
    wchar_t lives_buffer[0x80];

    if (0 < game_engine_variant.lives_per_round) {
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        int32_t lives_left = game_engine_variant.lives_per_round - (int32_t)(int16_t)p->deaths;

        if (lives_left == 0) {
            lives_text = multiplayer_game_text_string(0x34);
        } else if (lives_left == 1) {
            lives_text = multiplayer_game_text_string(0x35);
        } else {
            string_format_wide_va_bounded(0x80, lives_buffer, multiplayer_game_text_string(0x36), lives_left);
            lives_buffer[0x7f] = 0;
            lives_text = lives_buffer;
        }
    }

    if (game_engine_state_value == _game_engine_state_ending) {
        int32_t result = 0; // -1 no clear leader, 0 / 1 the two result phrasings; anything else writes nothing
        uint8_t teams = 0;

        if (current_game_engine != 0) {
            if (current_game_engine->is_winner == 0) {
                result = (int32_t)game_engine_is_object_winning(player_handle);
            } else {
                result = ((int32_t (*)(datum_index))current_game_engine->is_winner)(player_handle);
            }
            teams = (uint8_t)game_engine_variant.teams;
        }

        if (result == -1) {
            datum_index tag_id = tag_lookup(0x75737472, "ui\\multiplayer_game_text"); // 'ustr'
            wchar_t *text = &empty_string;

            if (tag_id != k_datum_index_none) {
                int32_t *tag_data = (int32_t *)tag_instances[tag_id & 0xffff].data;
                text = missing_string_text;
                if (0x37 < *tag_data) {
                    uint8_t *entry = (uint8_t *)tag_data[1];
                    uint32_t len = *(uint32_t *)(entry + 0x44c); // string 0x37: entries are 0x14 bytes
                    if (0 < (int32_t)len) {
                        wchar_t *string_data = *(wchar_t **)(entry + 0x458);
                        *(uint16_t *)((uint8_t *)string_data + ((len & 0xfffffffe) - 2)) = 0;
                        wcsncpy(out, string_data, 0x50);
                        goto done;
                    }
                }
            }
            wcsncpy(out, text, 0x50);
        } else if (result == 0) {
            wcsncpy(out, multiplayer_game_text_string(teams ? 0x38 : 0x39), 0x50);
        } else if (result == 1) {
            wcsncpy(out, multiplayer_game_text_string(teams ? 0x3a : 0x3b), 0x50);
        }
    } else if (current_game_engine == 0 || game_engine_variant.teams == 0) {
        scoreboard_entry entry;
        wchar_t header[0x80];
        wchar_t *fmt;

        game_engine_get_player_scoreboard_entry(player_handle, &entry);
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(player_handle, header);

        fmt = multiplayer_game_text_string((entry.place & 0x80000000) != 0 ? 0x3f : 0x40);
        string_format_wide_va_bounded(0x50, out, fmt, game_engine_get_default_multiplayer_string(&entry), header, lives_text);
    } else {
        wchar_t team0_text[14];
        wchar_t team1_text[14];
        int32_t score0, score1;

        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(0, team0_text);
        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(1, team1_text);
        score0 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        score1 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(1);

        if (score0 > score1) {
            string_format_wide_va_bounded(0x50, out, multiplayer_game_text_string(0x3c), team0_text, team1_text, lives_text);
        } else if (score0 < score1) {
            string_format_wide_va_bounded(0x50, out, multiplayer_game_text_string(0x3d), team1_text, team0_text, lives_text);
        } else {
            string_format_wide_va_bounded(0x50, out, multiplayer_game_text_string(0x3e), team1_text, lives_text);
        }
    }

done:
    out[0x4f] = 0;
}

#if 0
Original Ghidra decompilation (0x45cf30), from tools/pack.py 0x45cf30:

void FUN_0045cf30(uint param_1,wchar_t *param_2)

{
  int *piVar1;
  int iVar2;
  int iVar3;
  wchar_t *pwVar4;
  uint uVar5;
  undefined *puVar6;
  undefined4 *puVar7;
  undefined4 *puVar8;
  char cVar9;
  undefined **_Source;
  undefined *puVar10;
  undefined4 local_338 [6];
  uint uStack_320;
  undefined4 local_31c [7];
  undefined1 local_300 [254];
  undefined2 local_202;
  undefined4 local_200 [128];

  puVar10 = &DAT_00660c34;
  if (0 < DAT_006f1cd8) {
    iVar2 = DAT_006f1cd8 -
            *(short *)((param_1 & 0xffff) * 0x200 + 0xae + *(int *)(DAT_0087a480 + 0x34));
    if (iVar2 == 0) {
      iVar2 = tag_lookup("ui\\multiplayer_game_text");
      if (iVar2 == -1) {
LAB_0045d003:
        puVar10 = &DAT_00660c34;
      }
      else {
        puVar10 = (undefined *)text_string_list_get_string();
      }
    }
    else if (iVar2 == 1) {
      iVar2 = tag_lookup("ui\\multiplayer_game_text");
      if (iVar2 == -1) goto LAB_0045d003;
      puVar10 = (undefined *)text_string_list_get_string();
    }
    else {
      iVar3 = tag_lookup("ui\\multiplayer_game_text");
      if (iVar3 == -1) {
        puVar10 = &DAT_00660c34;
      }
      else {
        puVar10 = (undefined *)text_string_list_get_string();
      }
      string_format_wide_va_bounded(local_300,puVar10,iVar2);
      local_202 = 0;
      puVar10 = local_300;
    }
  }
  if (DAT_0087aa10 == 1) {
    iVar2 = 0;
    if (DAT_006f1d20 != 0) {
      if (*(code **)(DAT_006f1d20 + 0x8c) == (code *)0x0) {
        iVar2 = FUN_00463660();
      }
      else {
        iVar2 = (**(code **)(DAT_006f1d20 + 0x8c))(param_1);
      }
    }
    cVar9 = '\0';
    if (DAT_006f1d20 != 0) {
      cVar9 = DAT_006f1cbc;
    }
    if (iVar2 == -1) {
      uVar5 = tag_lookup("ui\\multiplayer_game_text");
      if (uVar5 == 0xffffffff) {
        _Source = (undefined **)&DAT_00660c34;
      }
      else {
        piVar1 = *(int **)((uVar5 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
        _Source = &PTR_DAT_00671fac;
        if (0x37 < *piVar1) {
          iVar2 = piVar1[1];
          uVar5 = *(uint *)(iVar2 + 0x44c);
          if (0 < (int)uVar5) {
            pwVar4 = *(wchar_t **)(iVar2 + 0x458);
            *(undefined2 *)((int)pwVar4 + ((uVar5 & 0xfffffffe) - 2)) = 0;
            _wcsncpy(param_2,pwVar4,0x50);
            goto LAB_0045d41b;
          }
        }
      }
      _wcsncpy(param_2,(wchar_t *)_Source,0x50);
    }
    else {
      if (iVar2 == 0) {
        if (cVar9 != '\0') {
          iVar2 = tag_lookup("ui\\multiplayer_game_text");
          if (iVar2 == -1) {
            _wcsncpy(param_2,L"",0x50);
          }
          else {
            pwVar4 = (wchar_t *)text_string_list_get_string();
            _wcsncpy(param_2,pwVar4,0x50);
          }
          goto LAB_0045d41b;
        }
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 != -1) {
          pwVar4 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(param_2,pwVar4,0x50);
          goto LAB_0045d41b;
        }
      }
      else {
        if (iVar2 != 1) goto LAB_0045d41b;
        if (cVar9 == '\0') {
          iVar2 = tag_lookup("ui\\multiplayer_game_text");
          if (iVar2 == -1) {
            _wcsncpy(param_2,L"",0x50);
          }
          else {
            pwVar4 = (wchar_t *)text_string_list_get_string();
            _wcsncpy(param_2,pwVar4,0x50);
          }
          goto LAB_0045d41b;
        }
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 != -1) {
          pwVar4 = (wchar_t *)text_string_list_get_string();
          _wcsncpy(param_2,pwVar4,0x50);
          goto LAB_0045d41b;
        }
      }
      _wcsncpy(param_2,L"",0x50);
    }
  }
  else {
    if ((DAT_006f1d20 == 0) || (DAT_006f1cbc == '\0')) {
      puVar7 = (undefined4 *)FUN_0045cee0();
      puVar8 = local_338;
      for (iVar2 = 7; iVar2 != 0; iVar2 = iVar2 + -1) {
        *puVar8 = *puVar7;
        puVar7 = puVar7 + 1;
        puVar8 = puVar8 + 1;
      }
      (**(code **)(DAT_006f1d20 + 0x54))(param_1,local_200);
      if ((uStack_320 & 0x80000000) == 0) {
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 == -1) {
          puVar6 = &DAT_00660c34;
        }
        else {
          puVar6 = (undefined *)text_string_list_get_string();
        }
        puVar7 = local_200;
        puVar8 = (undefined4 *)FUN_0045ce90(puVar7,puVar10);
      }
      else {
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 == -1) {
          puVar6 = &DAT_00660c34;
        }
        else {
          puVar6 = (undefined *)text_string_list_get_string();
        }
        puVar7 = local_200;
        puVar8 = (undefined4 *)FUN_0045ce90(puVar7,puVar10);
      }
    }
    else {
      (**(code **)(DAT_006f1d20 + 0x5c))(0,local_338);
      (**(code **)(DAT_006f1d20 + 0x5c))(1,local_31c);
      iVar2 = (**(code **)(DAT_006f1d20 + 0x50))(0);
      iVar3 = (**(code **)(DAT_006f1d20 + 0x50))(1);
      if (iVar3 < iVar2) {
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 == -1) {
          puVar7 = local_31c;
          puVar8 = local_338;
          puVar6 = &DAT_00660c34;
        }
        else {
          puVar6 = (undefined *)text_string_list_get_string();
          puVar7 = local_31c;
          puVar8 = local_338;
        }
      }
      else {
        if (iVar3 <= iVar2) {
          iVar2 = tag_lookup("ui\\multiplayer_game_text");
          if (iVar2 == -1) {
            puVar6 = &DAT_00660c34;
          }
          else {
            puVar6 = (undefined *)text_string_list_get_string();
          }
          string_format_wide_va_bounded(param_2,puVar6,local_31c,puVar10);
          goto LAB_0045d41b;
        }
        iVar2 = tag_lookup("ui\\multiplayer_game_text");
        if (iVar2 == -1) {
          puVar7 = local_338;
          puVar8 = local_31c;
          puVar6 = &DAT_00660c34;
        }
        else {
          puVar6 = (undefined *)text_string_list_get_string();
          puVar7 = local_338;
          puVar8 = local_31c;
        }
      }
    }
    string_format_wide_va_bounded(param_2,puVar6,puVar8,puVar7,puVar10);
  }
LAB_0045d41b:
  param_2[0x4f] = L'\0';
  return;
}
#endif
