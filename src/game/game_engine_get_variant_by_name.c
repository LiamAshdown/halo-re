// game_engine_get_variant_by_name  (Ghidra: game_engine_get_variant_by_name, already named)
// address 0x4622d0, size 1968 bytes
// name confidence: 0.9   rewrite confidence: 0.35
// evidence: CEA/PDB string match; the 35 `game_engine_variant_defaults_*` callees (this batch
// through 0x464430, later batches beyond it); types/game.h k_game_variant_size (0x98, matching
// the 0x26-dword copy at this function's tail) and k_maximum_variant_history_entries (99,
// matching the "uVar4 < 100" scan below).
// register convention: the requested name (narrow char*) in ECX (in_ECX). `out` (NULL to just
// test existence) is this function's own stack parameter.
//   // blam-cc: ECX -> name, stack -> out
// UNSURE: this rewrite restructures Ghidra's ~35-deep nested if/else chain of identical
// `_stricmp(name, "literal") -> call defaults -> break` arms into an equivalent table scan; the
// comparison order (and therefore which name wins on any ambiguity) is preserved exactly.
// CORRECTED (phase 4 review, objdump --start-address=0x4622d0 --stop-address=0x462a80):
//   1. `dynamic_variant_name` does not exist. Ghidra prints the 36th comparison's operand as
//      PTR_s_g_objects_equipment_00667450_0x13_00660888; 0x00660888 is not a pointer, it is the
//      string literal "ctf" (objdump -s -j .rdata: 63 74 66 00). The arm is therefore
//      _stricmp(name, "ctf") and FUN_00467450 is game_engine_variant_defaults_stalker.
//   2. `requested_name_wide` is not a parameter. It is the 24-wchar local buffer immediately
//      after the staging variant (Ghidra's local_1c0), and string_convert_ascii_to_unicode fills it -- that call
//      takes EAX = &buffer and EDI = 0x30 (objdump 0x4629bc..0x4629d0), both of which Ghidra
//      drops, which is why the buffer looked unwritten.
//   3. The custom-variant scan is bounded by the COUNT saved_game_enumerate_by_type writes back
//      through EBX (objdump 0x4629f6 / 0x4629ff / 0x462a52: `cmp si,di; jb`), not by a fixed 100
//      iterations. 100 is only the capacity passed in.
//   4. Every arm's copy source is the EAX the defaults function returns, and each of those
//      returns its own `out` argument unchanged (e.g. objdump 0x463c8e `mov eax,[ebp+8]` with
//      nothing clobbering EAX afterwards), so `&staging` is the copy source on every path.
// The custom-variant scan's "-1 means call game_engine_apply_current_custom_variant and keep
// scanning" behavior (rather than stopping) is transcribed exactly as decompiled, odd as it looks.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

typedef void (*game_engine_variant_defaults_fn)(game_variant *out);

extern uint8_t playlist_profiles_need_defaults; // 0x0069e8d0


extern void game_engine_apply_current_custom_variant(void); // 0x463b90, this batch
extern void game_engine_variant_defaults_classic_slayer(game_variant *out); // 0x463c40, this batch
extern void game_engine_variant_defaults_classic_slayer_pro(game_variant *out); // 0x463d20, this batch
extern void game_engine_variant_defaults_classic_elimination(game_variant *out); // 0x463e00, this batch
extern void game_engine_variant_defaults_classic_phantoms(game_variant *out); // 0x463ee0, this batch
extern void game_engine_variant_defaults_classic_endurance(game_variant *out); // 0x463fc0, this batch
extern void game_engine_variant_defaults_classic_rockets(game_variant *out); // 0x4640a0, this batch
extern void game_engine_variant_defaults_classic_snipers(game_variant *out); // 0x464180, this batch
extern void game_engine_variant_defaults_classic_team_slayer(game_variant *out); // 0x464260, this batch
extern void game_engine_variant_defaults_classic_oddball(game_variant *out); // 0x464340, this batch
extern void game_engine_variant_defaults_classic_team_oddball(game_variant *out); // 0x464430, this batch
extern void game_engine_variant_defaults_classic_reverse_tag(game_variant *out); // 0x464510, not in this batch
extern void game_engine_variant_defaults_classic_accumulation(game_variant *out); // 0x464600, not in this batch
extern void game_engine_variant_defaults_classic_juggernaut(game_variant *out); // 0x464700, not in this batch
extern void game_engine_variant_defaults_classic_stalker(game_variant *out); // 0x464800, not in this batch
extern void game_engine_variant_defaults_classic_king(game_variant *out); // 0x464900, not in this batch
extern void game_engine_variant_defaults_classic_king_pro(game_variant *out); // 0x4649d0, not in this batch
extern void game_engine_variant_defaults_classic_crazy_king(game_variant *out); // 0x464aa0, not in this batch
extern void game_engine_variant_defaults_classic_team_king(game_variant *out); // 0x464b70, not in this batch
extern void game_engine_variant_defaults_classic_ctf(game_variant *out); // 0x464c40, not in this batch
extern void game_engine_variant_defaults_classic_ctf_pro(game_variant *out); // 0x464d30, not in this batch
extern void game_engine_variant_defaults_classic_invasion(game_variant *out); // 0x464e20, not in this batch
extern void game_engine_variant_defaults_classic_iron_ctf(game_variant *out); // 0x464f10, not in this batch
extern void game_engine_variant_defaults_classic_race(game_variant *out); // 0x465000, not in this batch
extern void game_engine_variant_defaults_classic_rally(game_variant *out); // 0x4650e0, not in this batch
extern void game_engine_variant_defaults_classic_team_race(game_variant *out); // 0x4651c0, not in this batch
extern void game_engine_variant_defaults_classic_team_rally(game_variant *out); // 0x4652a0, not in this batch
extern void game_engine_variant_defaults_team_slayer(game_variant *out); // 0x467cf0, not in this batch
extern void game_engine_variant_defaults_team_race(game_variant *out); // 0x467c00, not in this batch
extern void game_engine_variant_defaults_team_oddball(game_variant *out); // 0x467af0, not in this batch
extern void game_engine_variant_defaults_team_king(game_variant *out); // 0x467a10, not in this batch
extern void game_engine_variant_defaults_slayer(game_variant *out); // 0x467920, not in this batch
extern void game_engine_variant_defaults_race(game_variant *out); // 0x467830, not in this batch
extern void game_engine_variant_defaults_oddball(game_variant *out); // 0x467730, not in this batch
extern void game_engine_variant_defaults_king(game_variant *out); // 0x467650, not in this batch
extern void game_engine_variant_defaults_juggernaut(game_variant *out); // 0x467540, not in this batch
extern game_variant *game_engine_variant_defaults_stalker(game_variant *out); // 0x467450, not in
    // this batch. RENAMED from FUN_00467450: its string arm is the literal "ctf" (see above), and
    // it ends in the same rep movs out of a local staging buffer as its 37 siblings.
extern void game_engine_variant_defaults_crazy_king(game_variant *out); // 0x467370, not in this batch
extern void game_engine_variant_defaults_assault(game_variant *out); // 0x467270, not in this batch

extern void string_convert_ascii_to_unicode(wchar_t *out_name, int32_t max_chars); // 0x557990, not in this batch;
    // blam-cc: EAX -> out_name, EDI -> max_chars. UNSURE identity, but it is what fills the
    // 24-wchar buffer the custom-variant scan then compares against (objdump 0x4629bc: EDI = 0x30).
extern void playlist_profile_create_default_profiles_on_disk(void); // 0x53bc70
extern void saved_game_enumerate_by_type(uint16_t type, int32_t *out_handles, uint8_t builtin_only,
    uint16_t *capacity_and_count); // 0x53c4e0, stack (type, out, builtin_only), EBX &count
    // 0x53c4e0; blam-cc: EBX -> an in/out capacity-then-count int32 (100 in, the number of
    // enumerated slots out), then the three stack arguments
extern uint8_t saved_game_get_variant(int32_t slot, game_variant *out); // 0x53bee0, not in this batch

// blam-cc: ECX -> name, stack -> out
uint8_t game_engine_get_variant_by_name(const char *name, game_variant *out)
{
    static const struct { const char *name; game_engine_variant_defaults_fn fn; } k_builtin_variants[] = {
        {"classic_slayer", game_engine_variant_defaults_classic_slayer},
        {"classic_slayer_pro", game_engine_variant_defaults_classic_slayer_pro},
        {"classic_elimination", game_engine_variant_defaults_classic_elimination},
        {"classic_phantoms", game_engine_variant_defaults_classic_phantoms},
        {"classic_endurance", game_engine_variant_defaults_classic_endurance},
        {"classic_rockets", game_engine_variant_defaults_classic_rockets},
        {"classic_snipers", game_engine_variant_defaults_classic_snipers},
        {"classic_team_slayer", game_engine_variant_defaults_classic_team_slayer},
        {"classic_oddball", game_engine_variant_defaults_classic_oddball},
        {"classic_team_oddball", game_engine_variant_defaults_classic_team_oddball},
        {"classic_reverse_tag", game_engine_variant_defaults_classic_reverse_tag},
        {"classic_accumulation", game_engine_variant_defaults_classic_accumulation},
        {"classic_juggernaut", game_engine_variant_defaults_classic_juggernaut},
        {"classic_stalker", game_engine_variant_defaults_classic_stalker},
        {"classic_king", game_engine_variant_defaults_classic_king},
        {"classic_king_pro", game_engine_variant_defaults_classic_king_pro},
        {"classic_crazy_king", game_engine_variant_defaults_classic_crazy_king},
        {"classic_team_king", game_engine_variant_defaults_classic_team_king},
        {"classic_ctf", game_engine_variant_defaults_classic_ctf},
        {"classic_ctf_pro", game_engine_variant_defaults_classic_ctf_pro},
        {"classic_invasion", game_engine_variant_defaults_classic_invasion},
        {"classic_iron_ctf", game_engine_variant_defaults_classic_iron_ctf},
        {"classic_race", game_engine_variant_defaults_classic_race},
        {"classic_rally", game_engine_variant_defaults_classic_rally},
        {"classic_team_race", game_engine_variant_defaults_classic_team_race},
        {"classic_team_rally", game_engine_variant_defaults_classic_team_rally},
        {"team_slayer", game_engine_variant_defaults_team_slayer},
        {"team_race", game_engine_variant_defaults_team_race},
        {"team_oddball", game_engine_variant_defaults_team_oddball},
        {"team_king", game_engine_variant_defaults_team_king},
        {"slayer", game_engine_variant_defaults_slayer},
        {"race", game_engine_variant_defaults_race},
        {"oddball", game_engine_variant_defaults_oddball},
        {"king", game_engine_variant_defaults_king},
        {"juggernaut", game_engine_variant_defaults_juggernaut},
    };
    game_variant staging;             // Ghidra's local_258, 76 uint16 == 0x98
    wchar_t requested_name_wide[24];  // Ghidra's local_1c0, immediately after `staging`
    uint8_t matched = 0;
    size_t i;

    for (i = 0; i < sizeof(k_builtin_variants) / sizeof(k_builtin_variants[0]); i++) {
        if (_stricmp(name, k_builtin_variants[i].name) == 0) {
            if (out == 0) {
                return 1;
            }
            k_builtin_variants[i].fn(&staging);
            matched = 1;
            break;
        }
    }

    if (matched == 0 && _stricmp(name, "ctf") == 0) {
        if (out == 0) {
            return 1;
        }
        game_engine_variant_defaults_stalker(&staging);
        matched = 1;
    } else if (matched == 0 && _stricmp(name, "crazy_king") == 0) {
        if (out == 0) {
            return 1;
        }
        game_engine_variant_defaults_crazy_king(&staging);
        matched = 1;
    } else if (matched == 0 && _stricmp(name, "assault") != 0) {
        // Not any built-in name at all: search the saved/custom game variant list.
        int32_t slots[100];       // matches Ghidra's local_190 [100]
        int32_t slot_count = 100; // in: capacity; out: how many slots were enumerated (EBX)
        uint16_t slot_index;

        string_convert_ascii_to_unicode(requested_name_wide, 0x30); // objdump 0x4629bc: EAX = &buffer, EDI = 0x30
        if (playlist_profiles_need_defaults == 1) {
            playlist_profile_create_default_profiles_on_disk();
            playlist_profiles_need_defaults = 0;
        }
        // blam-cc: EBX = &slot_count
        saved_game_enumerate_by_type(1, slots, 1, (uint16_t *)&slot_count); // 0x4629f6

        for (slot_index = 0; (int32_t)(uint32_t)slot_index < slot_count; slot_index++) {
            if (slots[slot_index] == -1) {
                game_engine_apply_current_custom_variant();
                continue;
            }
            if (saved_game_get_variant(slots[slot_index], &staging) != 0 &&
                _wcsicmp((const wchar_t *)staging.name, requested_name_wide) == 0) {
                if (out == 0) {
                    return 1;
                }
                matched = 1;
                break;
            }
        }
        if (matched == 0) {
            return 0; // objdump 0x462a57: the exhausted scan returns the flag byte, still 0
        }
    } else if (matched == 0) {
        // name == "assault"
        if (out == 0) {
            return 1;
        }
        game_engine_variant_defaults_assault(&staging);
    }

    memcpy(out, &staging, sizeof(game_variant)); // objdump 0x462a70: rep movs, ecx = 0x26
    return 1;
}

#if 0
Original Ghidra decompilation (0x4622d0), from tools/pack.py 0x4622d0:

undefined1 game_engine_get_variant_by_name(undefined4 *param_1)

{
  char cVar1;
  int iVar2;
  wchar_t *extraout_EAX;
  wchar_t *extraout_EAX_00;
  wchar_t *extraout_EAX_01;
  wchar_t *extraout_EAX_02;
  wchar_t *extraout_EAX_03;
  wchar_t *extraout_EAX_04;
  wchar_t *extraout_EAX_05;
  wchar_t *extraout_EAX_06;
  wchar_t *extraout_EAX_07;
  wchar_t *extraout_EAX_08;
  wchar_t *extraout_EAX_09;
  wchar_t *extraout_EAX_10;
  wchar_t *extraout_EAX_11;
  wchar_t *extraout_EAX_12;
  wchar_t *extraout_EAX_13;
  wchar_t *extraout_EAX_14;
  wchar_t *extraout_EAX_15;
  wchar_t *extraout_EAX_16;
  wchar_t *extraout_EAX_17;
  wchar_t *extraout_EAX_18;
  wchar_t *extraout_EAX_19;
  wchar_t *extraout_EAX_20;
  wchar_t *extraout_EAX_21;
  wchar_t *extraout_EAX_22;
  wchar_t *extraout_EAX_23;
  wchar_t *extraout_EAX_24;
  wchar_t *extraout_EAX_25;
  wchar_t *extraout_EAX_26;
  wchar_t *extraout_EAX_27;
  wchar_t *extraout_EAX_28;
  wchar_t *extraout_EAX_29;
  wchar_t *extraout_EAX_30;
  wchar_t *extraout_EAX_31;
  wchar_t *extraout_EAX_32;
  wchar_t *extraout_EAX_33;
  wchar_t *pwVar3;
  wchar_t *extraout_EAX_34;
  wchar_t *extraout_EAX_35;
  char *in_ECX;
  ushort uVar4;
  wchar_t local_258 [76];
  wchar_t local_1c0 [24];
  int local_190 [100];

  iVar2 = __stricmp(in_ECX,"classic_slayer");
  if (iVar2 == 0) {
    if (param_1 == (undefined4 *)0x0) {
      return 1;
    }
    game_engine_variant_defaults_classic_slayer(local_258);
    pwVar3 = extraout_EAX;
  }
  else {
    iVar2 = __stricmp(in_ECX,"classic_slayer_pro");
    if (iVar2 == 0) {
      if (param_1 == (undefined4 *)0x0) {
        return 1;
      }
      game_engine_variant_defaults_classic_slayer_pro(local_258);
      pwVar3 = extraout_EAX_00;
    }
    else {
      iVar2 = __stricmp(in_ECX,"classic_elimination");
      if (iVar2 == 0) {
        if (param_1 == (undefined4 *)0x0) {
          return 1;
        }
        game_engine_variant_defaults_classic_elimination(local_258);
        pwVar3 = extraout_EAX_01;
      }
      else {
        iVar2 = __stricmp(in_ECX,"classic_phantoms");
        if (iVar2 == 0) {
          if (param_1 == (undefined4 *)0x0) {
            return 1;
          }
          game_engine_variant_defaults_classic_phantoms(local_258);
          pwVar3 = extraout_EAX_02;
        }
        else {
          iVar2 = __stricmp(in_ECX,"classic_endurance");
          if (iVar2 == 0) {
            if (param_1 == (undefined4 *)0x0) {
              return 1;
            }
            game_engine_variant_defaults_classic_endurance(local_258);
            pwVar3 = extraout_EAX_03;
          }
          else {
            iVar2 = __stricmp(in_ECX,"classic_rockets");
            if (iVar2 == 0) {
              if (param_1 == (undefined4 *)0x0) {
                return 1;
              }
              game_engine_variant_defaults_classic_rockets(local_258);
              pwVar3 = extraout_EAX_04;
            }
            else {
              iVar2 = __stricmp(in_ECX,"classic_snipers");
              if (iVar2 == 0) {
                if (param_1 == (undefined4 *)0x0) {
                  return 1;
                }
                game_engine_variant_defaults_classic_snipers(local_258);
                pwVar3 = extraout_EAX_05;
              }
              else {
                iVar2 = __stricmp(in_ECX,"classic_team_slayer");
                if (iVar2 == 0) {
                  if (param_1 == (undefined4 *)0x0) {
                    return 1;
                  }
                  game_engine_variant_defaults_classic_team_slayer(local_258);
                  pwVar3 = extraout_EAX_06;
                }
                else {
                  iVar2 = __stricmp(in_ECX,"classic_oddball");
                  if (iVar2 == 0) {
                    if (param_1 == (undefined4 *)0x0) {
                      return 1;
                    }
                    game_engine_variant_defaults_classic_oddball(local_258);
                    pwVar3 = extraout_EAX_07;
                  }
                  else {
                    iVar2 = __stricmp(in_ECX,"classic_team_oddball");
                    if (iVar2 == 0) {
                      if (param_1 == (undefined4 *)0x0) {
                        return 1;
                      }
                      game_engine_variant_defaults_classic_team_oddball(local_258);
                      pwVar3 = extraout_EAX_08;
                    }
                    else {
                      iVar2 = __stricmp(in_ECX,"classic_reverse_tag");
                      if (iVar2 == 0) {
                        if (param_1 == (undefined4 *)0x0) {
                          return 1;
                        }
                        game_engine_variant_defaults_classic_reverse_tag(local_258);
                        pwVar3 = extraout_EAX_09;
                      }
                      else {
                        iVar2 = __stricmp(in_ECX,"classic_accumulation");
                        if (iVar2 == 0) {
                          if (param_1 == (undefined4 *)0x0) {
                            return 1;
                          }
                          game_engine_variant_defaults_classic_accumulation(local_258);
                          pwVar3 = extraout_EAX_10;
                        }
                        else {
                          iVar2 = __stricmp(in_ECX,"classic_juggernaut");
                          if (iVar2 == 0) {
                            if (param_1 == (undefined4 *)0x0) {
                              return 1;
                            }
                            game_engine_variant_defaults_classic_juggernaut(local_258);
                            pwVar3 = extraout_EAX_11;
                          }
                          else {
                            iVar2 = __stricmp(in_ECX,"classic_stalker");
                            if (iVar2 == 0) {
                              if (param_1 == (undefined4 *)0x0) {
                                return 1;
                              }
                              game_engine_variant_defaults_classic_stalker(local_258);
                              pwVar3 = extraout_EAX_12;
                            }
                            else {
                              iVar2 = __stricmp(in_ECX,"classic_king");
                              if (iVar2 == 0) {
                                if (param_1 == (undefined4 *)0x0) {
                                  return 1;
                                }
                                game_engine_variant_defaults_classic_king(local_258);
                                pwVar3 = extraout_EAX_13;
                              }
                              else {
                                iVar2 = __stricmp(in_ECX,"classic_king_pro");
                                if (iVar2 == 0) {
                                  if (param_1 == (undefined4 *)0x0) {
                                    return 1;
                                  }
                                  game_engine_variant_defaults_classic_king_pro(local_258);
                                  pwVar3 = extraout_EAX_14;
                                }
                                else {
                                  iVar2 = __stricmp(in_ECX,"classic_crazy_king");
                                  if (iVar2 == 0) {
                                    if (param_1 == (undefined4 *)0x0) {
                                      return 1;
                                    }
                                    game_engine_variant_defaults_classic_crazy_king(local_258);
                                    pwVar3 = extraout_EAX_15;
                                  }
                                  else {
                                    iVar2 = __stricmp(in_ECX,"classic_team_king");
                                    if (iVar2 == 0) {
                                      if (param_1 == (undefined4 *)0x0) {
                                        return 1;
                                      }
                                      game_engine_variant_defaults_classic_team_king(local_258);
                                      pwVar3 = extraout_EAX_16;
                                    }
                                    else {
                                      iVar2 = __stricmp(in_ECX,"classic_ctf");
                                      if (iVar2 == 0) {
                                        if (param_1 == (undefined4 *)0x0) {
                                          return 1;
                                        }
                                        game_engine_variant_defaults_classic_ctf(local_258);
                                        pwVar3 = extraout_EAX_17;
                                      }
                                      else {
                                        iVar2 = __stricmp(in_ECX,"classic_ctf_pro");
                                        if (iVar2 == 0) {
                                          if (param_1 == (undefined4 *)0x0) {
                                            return 1;
                                          }
                                          game_engine_variant_defaults_classic_ctf_pro(local_258);
                                          pwVar3 = extraout_EAX_18;
                                        }
                                        else {
                                          iVar2 = __stricmp(in_ECX,"classic_invasion");
                                          if (iVar2 == 0) {
                                            if (param_1 == (undefined4 *)0x0) {
                                              return 1;
                                            }
                                            game_engine_variant_defaults_classic_invasion(local_258)
                                            ;
                                            pwVar3 = extraout_EAX_19;
                                          }
                                          else {
                                            iVar2 = __stricmp(in_ECX,"classic_iron_ctf");
                                            if (iVar2 == 0) {
                                              if (param_1 == (undefined4 *)0x0) {
                                                return 1;
                                              }
                                              game_engine_variant_defaults_classic_iron_ctf
                                                        (local_258);
                                              pwVar3 = extraout_EAX_20;
                                            }
                                            else {
                                              iVar2 = __stricmp(in_ECX,"classic_race");
                                              if (iVar2 == 0) {
                                                if (param_1 == (undefined4 *)0x0) {
                                                  return 1;
                                                }
                                                game_engine_variant_defaults_classic_race(local_258)
                                                ;
                                                pwVar3 = extraout_EAX_21;
                                              }
                                              else {
                                                iVar2 = __stricmp(in_ECX,"classic_rally");
                                                if (iVar2 == 0) {
                                                  if (param_1 == (undefined4 *)0x0) {
                                                    return 1;
                                                  }
                                                  game_engine_variant_defaults_classic_rally
                                                            (local_258);
                                                  pwVar3 = extraout_EAX_22;
                                                }
                                                else {
                                                  iVar2 = __stricmp(in_ECX,"classic_team_race");
                                                  if (iVar2 == 0) {
                                                    if (param_1 == (undefined4 *)0x0) {
                                                      return 1;
                                                    }
                                                    game_engine_variant_defaults_classic_team_race
                                                              (local_258);
                                                    pwVar3 = extraout_EAX_23;
                                                  }
                                                  else {
                                                    iVar2 = __stricmp(in_ECX,"classic_team_rally");
                                                    if (iVar2 == 0) {
                                                      if (param_1 == (undefined4 *)0x0) {
                                                        return 1;
                                                      }

                                                  game_engine_variant_defaults_classic_team_rally
                                                            (local_258);
                                                  pwVar3 = extraout_EAX_24;
                                                  }
                                                  else {
                                                    iVar2 = __stricmp(in_ECX,"team_slayer");
                                                    if (iVar2 == 0) {
                                                      if (param_1 == (undefined4 *)0x0) {
                                                        return 1;
                                                      }
                                                      game_engine_variant_defaults_team_slayer
                                                                (local_258);
                                                      pwVar3 = extraout_EAX_25;
                                                    }
                                                    else {
                                                      iVar2 = __stricmp(in_ECX,"team_race");
                                                      if (iVar2 == 0) {
                                                        if (param_1 == (undefined4 *)0x0) {
                                                          return 1;
                                                        }
                                                        game_engine_variant_defaults_team_race
                                                                  (local_258);
                                                        pwVar3 = extraout_EAX_26;
                                                      }
                                                      else {
                                                        iVar2 = __stricmp(in_ECX,"team_oddball");
                                                        if (iVar2 == 0) {
                                                          if (param_1 == (undefined4 *)0x0) {
                                                            return 1;
                                                          }
                                                          game_engine_variant_defaults_team_oddball
                                                                    (local_258);
                                                          pwVar3 = extraout_EAX_27;
                                                        }
                                                        else {
                                                          iVar2 = __stricmp(in_ECX,"team_king");
                                                          if (iVar2 == 0) {
                                                            if (param_1 == (undefined4 *)0x0) {
                                                              return 1;
                                                            }
                                                            game_engine_variant_defaults_team_king
                                                                      (local_258);
                                                            pwVar3 = extraout_EAX_28;
                                                          }
                                                          else {
                                                            iVar2 = __stricmp(in_ECX,"slayer");
                                                            if (iVar2 == 0) {
                                                              if (param_1 == (undefined4 *)0x0) {
                                                                return 1;
                                                              }
                                                              game_engine_variant_defaults_slayer
                                                                        (local_258);
                                                              pwVar3 = extraout_EAX_29;
                                                            }
                                                            else {
                                                              iVar2 = __stricmp(in_ECX,"race");
                                                              if (iVar2 == 0) {
                                                                if (param_1 == (undefined4 *)0x0) {
                                                                  return 1;
                                                                }
                                                                game_engine_variant_defaults_race
                                                                          (local_258);
                                                                pwVar3 = extraout_EAX_30;
                                                              }
                                                              else {
                                                                iVar2 = __stricmp(in_ECX,"oddball");
                                                                if (iVar2 == 0) {
                                                                  if (param_1 == (undefined4 *)0x0)
                                                                  {
                                                                    return 1;
                                                                  }

                                                  game_engine_variant_defaults_oddball(local_258);
                                                  pwVar3 = extraout_EAX_31;
                                                  }
                                                  else {
                                                    iVar2 = __stricmp(in_ECX,"king");
                                                    if (iVar2 == 0) {
                                                      if (param_1 == (undefined4 *)0x0) {
                                                        return 1;
                                                      }
                                                      game_engine_variant_defaults_king(local_258);
                                                      pwVar3 = extraout_EAX_32;
                                                    }
                                                    else {
                                                      iVar2 = __stricmp(in_ECX,"juggernaut");
                                                      if (iVar2 == 0) {
                                                        if (param_1 == (undefined4 *)0x0) {
                                                          return 1;
                                                        }
                                                        game_engine_variant_defaults_juggernaut
                                                                  (local_258);
                                                        pwVar3 = extraout_EAX_33;
                                                      }
                                                      else {
                                                        iVar2 = __stricmp(in_ECX,(char *)&
                                                  PTR_s_g_objects_equipment_00667450_0x13_00660888);
                                                  if (iVar2 == 0) {
                                                    if (param_1 == (undefined4 *)0x0) {
                                                      return 1;
                                                    }
                                                    pwVar3 = (wchar_t *)FUN_00467450(local_258);
                                                  }
                                                  else {
                                                    iVar2 = __stricmp(in_ECX,"crazy_king");
                                                    if (iVar2 == 0) {
                                                      if (param_1 == (undefined4 *)0x0) {
                                                        return 1;
                                                      }
                                                      game_engine_variant_defaults_crazy_king
                                                                (local_258);
                                                      pwVar3 = extraout_EAX_34;
                                                    }
                                                    else {
                                                      iVar2 = __stricmp(in_ECX,"assault");
                                                      if (iVar2 != 0) {
                                                        FUN_00557990();
                                                        if (DAT_0069e8d0 == '\x01') {

                                                  playlist_profile_create_default_profiles_on_disk()
                                                  ;
                                                  DAT_0069e8d0 = '\0';
                                                  }
                                                  saved_game_enumerate_by_type(1,local_190,1);
                                                  uVar4 = 0;
                                                  do {
                                                    if (local_190[uVar4] == -1) {
                                                      game_engine_apply_current_custom_variant();
                                                    }
                                                    else {
                                                      cVar1 = FUN_0053bee0(local_190[uVar4],
                                                                           local_258);
                                                      if ((cVar1 != '\0') &&
                                                         (iVar2 = __wcsicmp(local_258,local_1c0),
                                                         iVar2 == 0)) {
                                                        if (param_1 == (undefined4 *)0x0) {
                                                          return 1;
                                                        }
                                                        pwVar3 = local_258;
                                                        goto LAB_00462a6e;
                                                      }
                                                    }
                                                    uVar4 = uVar4 + 1;
                                                    if (99 < uVar4) {
                                                      return 0;
                                                    }
                                                  } while( true );
                                                  }
                                                  if (param_1 == (undefined4 *)0x0) {
                                                    return 1;
                                                  }
                                                  game_engine_variant_defaults_assault(local_258);
                                                  pwVar3 = extraout_EAX_35;
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                  }
                                                }
                                              }
                                            }
                                          }
                                        }
                                      }
                                    }
                                  }
                                }
                              }
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
          }
        }
      }
    }
  }
LAB_00462a6e:
  for (iVar2 = 0x26; iVar2 != 0; iVar2 = iVar2 + -1) {
    *param_1 = *(undefined4 *)pwVar3;
    pwVar3 = pwVar3 + 2;
    param_1 = param_1 + 1;
  }
  return 1;
}
#endif
