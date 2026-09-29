// game_engine_build_end_game_result_text  (Ghidra: FUN_0045cf30; named per
// out/phase4/game_functions.md)
// address 0x45cf30, size 1286 bytes
// name confidence: 0.45   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Builds the localized end-of-game result text (e.g.
// who is leading or tied) by comparing player or team scores through the active game engine's
// callbacks"); types/game.h game_variant (lives_per_round +0x50, teams +0x34),
// game_engine_state, game_engine_definition (is_winner, build_score_header_text,
// build_team_score_text, get_team_score); scoreboard_entry (place, tie bit 0x80000000);
// game_engine_get_default_multiplayer_string.c / game_engine_get_player_scoreboard_entry.c
// (this batch).
// register convention: none recognized by Ghidra; both parameters are its own explicit stack
// args.
// UNSURE (pervasive through this file): almost every text_string_list_get_string() /
// tag_lookup() pair in the original decompilation is called with no visible string-table index
// -- the index lives in a register Ghidra could not trace across this function's many branches.
// Two of them were pinned by disassembly (indices 0x3f and 0x40, see
// game_engine_get_default_multiplayer_string.c's own header) but the rest could not be, and are
// modeled here as calls to that same helper with a placeholder index per branch, each flagged
// individually. The reflexive-style direct-offset string read for the "no result" / tie case
// (offsets +0x44c/+0x458 on the tag data, count gated on > 0x37) mirrors the same idiom used
// throughout game_engine_post_rasterize_post_game.c and is transcribed just as literally.

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

extern datum_index tag_lookup(tag_group group, char *path); // 0x442550
extern wchar_t *text_string_list_get_string(datum_index tag_id, int16_t index); // 0x5578c0
extern wchar_t *string_format_wide_va_bounded(wchar_t *dest, const wchar_t *format, ...); // 0x557910
extern char game_engine_is_object_winning(void); // 0x463660, not in this batch; UNSURE exact meaning
extern wchar_t *game_engine_get_default_multiplayer_string(int16_t string_index); // 0x45ce90, this batch
extern void game_engine_get_player_scoreboard_entry(datum_index player_handle, scoreboard_entry *out); // 0x45cee0, this batch

// blam-cc: none recognized; param_1 (player) and param_2 (out, wchar_t[0x50]) are Ghidra's own stack args
// Builds the localized "who is leading / tied / how many lives left" line for the end-of-game
// or in-progress scoreboard, using the active game engine's score-comparison callbacks.
void game_engine_build_end_game_result_text(datum_index player_handle, wchar_t *out)
{
    wchar_t *lives_text = &empty_string;

    if (0 < game_engine_variant.lives_per_round) {
        player *p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
        int32_t lives_left = game_engine_variant.lives_per_round - p->deaths;

        if (lives_left == 0) {
            lives_text = game_engine_get_default_multiplayer_string(0); // UNSURE: index not visible
        } else if (lives_left == 1) {
            lives_text = game_engine_get_default_multiplayer_string(1); // UNSURE: index not visible
        } else {
            wchar_t fmt_buffer[128]; // matches Ghidra's local_300 (254 bytes) + terminator
            wchar_t *fmt = game_engine_get_default_multiplayer_string(2); // UNSURE: index not visible
            string_format_wide_va_bounded(fmt_buffer, fmt, lives_left);
            fmt_buffer[0x7f] = 0;
            lives_text = fmt_buffer; // NOTE: Ghidra returns a pointer to a function-local buffer
                                      // here too (`local_300`), used immediately below before the
                                      // function returns, so lifetime is not actually a problem.
        }
    }

    if (game_engine_state_value == _game_engine_state_ending) {
        char result; // -1 no clear leader/tie, 0 or 1 select which of two phrasings
        uint8_t teams = 0;

        result = 0;
        if (current_game_engine != 0) {
            if (current_game_engine->is_winner == 0) {
                result = game_engine_is_object_winning();
            } else {
                result = ((char (*)(datum_index))current_game_engine->is_winner)(player_handle);
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
                    uint32_t len = *(uint32_t *)(entry + 0x44c);
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
            wchar_t *text = teams
                ? game_engine_get_default_multiplayer_string(3)  // UNSURE: index not visible
                : game_engine_get_default_multiplayer_string(4); // UNSURE: index not visible
            wcsncpy(out, text, 0x50);
            goto done;
        } else if (result == 1) {
            wchar_t *text = !teams
                ? game_engine_get_default_multiplayer_string(5)  // UNSURE: index not visible
                : game_engine_get_default_multiplayer_string(6); // UNSURE: index not visible
            wcsncpy(out, text, 0x50);
            goto done;
        } else {
            wcsncpy(out, L"", 0x50);
        }
    } else if (current_game_engine == 0 || game_engine_variant.teams == 0) {
        scoreboard_entry entry;
        wchar_t header[128]; // matches Ghidra's local_200 (0x1c0 == 7 dwords, but used here as
                              // build_score_header_text's own output buffer -- see UNSURE below
        wchar_t *fmt;

        game_engine_get_player_scoreboard_entry(player_handle, &entry);
        ((void (*)(datum_index, wchar_t *))current_game_engine->build_player_text)(
            player_handle, header);

        if ((entry.place & 0x80000000) == 0) {
            fmt = game_engine_get_default_multiplayer_string(7); // UNSURE: index not visible
        } else {
            fmt = game_engine_get_default_multiplayer_string(8); // UNSURE: index not visible
        }
        string_format_wide_va_bounded(out, fmt, header, lives_text);
    } else {
        wchar_t team0_text[128];
        wchar_t team1_text[128];
        int32_t score0, score1;
        wchar_t *fmt;

        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(0, team0_text);
        ((void (*)(int32_t, wchar_t *))current_game_engine->build_team_score_text)(1, team1_text);
        score0 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        score1 = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(1);

        if (score1 < score0) {
            fmt = game_engine_get_default_multiplayer_string(9); // UNSURE: index not visible
            string_format_wide_va_bounded(out, fmt, team1_text, team0_text, lives_text);
        } else if (score1 <= score0) {
            fmt = game_engine_get_default_multiplayer_string(10); // UNSURE: index not visible
            string_format_wide_va_bounded(out, fmt, team1_text, lives_text);
            goto done;
        } else {
            fmt = game_engine_get_default_multiplayer_string(11); // UNSURE: index not visible
            string_format_wide_va_bounded(out, fmt, team0_text, team1_text, lives_text);
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
