// custom_waypoint_matches_filter  (Ghidra: FUN_004620c0; renamed per its summary)
// address 0x4620c0, size 204 bytes
// name confidence: 0.4   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Register-argument helper that tests whether one entry
// of the 32-slot custom-waypoint table applies to a given object/player, honoring -1 wildcard
// sentinels for player, team-type and owner filters"); types/game.h custom_waypoint (active
// +0x0c, player +0x10, team +0x14, owner +0x18), game_engine_index (_game_engine_ctf == 1),
// game_engine_definition::waypoint_filter (+0x80).
// register convention: a waypoint table slot index in EDI (unaff_EDI); a pointer to a
// player-shaped record (only its team field at +0x20 is read) in EAX (in_EAX); param_1 is this
// function's own stack parameter (a player or object handle compared against the waypoint's
// player/owner fields).
//   // blam-cc: EAX -> reference, unaff_EDI -> slot, stack -> candidate
// UNSURE: the exact intent of the CTF-specific branch (including the extra game_engine_ctf_unit_is_flag_holder call)
// is not established anywhere in this batch's evidence; transcribed literally. The final
// "candidate == owner -> not a match" comparisons in both branches read backwards from a naive
// "does this waypoint apply" test, and are plausibly a "hide a flag's own waypoint from its own
// carrier" rule, but that reading is not confirmed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern uint8_t game_engine_ctf_unit_is_flag_holder(void); // 0x469780, not in this batch; UNSURE signature

// blam-cc: EAX -> reference, unaff_EDI -> slot, stack -> candidate
uint8_t custom_waypoint_matches_filter(int32_t candidate, custom_waypoint *slot, int32_t reference_team)
{
    uint8_t not_matching;

    if (current_game_engine == 0) {
        return 0;
    }

    if (current_game_engine->index == _game_engine_ctf && slot->active != 0) {
        if ((slot->player == (datum_index)0xffffffff || candidate == (int32_t)slot->player) &&
            (slot->team == -1 || reference_team == slot->team)) {
            if (slot->owner == (datum_index)0xffffffff) {
                return 1;
            }
            if (candidate != (int32_t)slot->owner) {
                return 1;
            }
        }
        if (reference_team == slot->team) {
            return 0;
        }
        if (slot->owner == (datum_index)0xffffffff) {
            return 0;
        }
        not_matching = (game_engine_ctf_unit_is_flag_holder() == 0);
    } else {
        if (current_game_engine->waypoint_filter != 0) {
            if (slot->active != 0) {
                return ((uint8_t (*)(int32_t))current_game_engine->waypoint_filter)(candidate);
            }
            return 0;
        }
        if (slot->active == 0) {
            return 0;
        }
        if (slot->player != (datum_index)0xffffffff && candidate != (int32_t)slot->player) {
            return 0;
        }
        if (slot->team != -1 && reference_team != slot->team) {
            return 0;
        }
        if (slot->owner == (datum_index)0xffffffff) {
            return 1;
        }
        not_matching = (candidate == (int32_t)slot->owner);
    }

    return not_matching ? 0 : 1;
}

#if 0
Original Ghidra decompilation (0x4620c0), from tools/pack.py 0x4620c0:

undefined1 FUN_004620c0(int param_1)

{
  char cVar1;
  undefined1 uVar2;
  int in_EAX;
  int unaff_EDI;
  bool bVar3;

  if (DAT_006f1d20 == 0) {
    return 0;
  }
  if ((*(int *)(DAT_006f1d20 + 4) == 1) && (*(char *)(&DAT_006f1894 + unaff_EDI * 8) != '\0')) {
    if ((((&DAT_006f1898)[unaff_EDI * 8] == -1) || (param_1 == (&DAT_006f1898)[unaff_EDI * 8])) &&
       ((*(short *)(&DAT_006f189c + unaff_EDI * 8) == -1 ||
        (*(int *)(in_EAX + 0x20) == (int)*(short *)(&DAT_006f189c + unaff_EDI * 8))))) {
      if ((&DAT_006f18a0)[unaff_EDI * 8] == -1) {
        return 1;
      }
      if (param_1 != (&DAT_006f18a0)[unaff_EDI * 8]) {
        return 1;
      }
    }
    if (*(int *)(in_EAX + 0x20) == (int)*(short *)(&DAT_006f189c + unaff_EDI * 8)) {
      return 0;
    }
    if ((&DAT_006f18a0)[unaff_EDI * 8] == -1) {
      return 0;
    }
    cVar1 = FUN_00469780();
    bVar3 = cVar1 == '\0';
  }
  else {
    if (*(code **)(DAT_006f1d20 + 0x80) != (code *)0x0) {
      if (*(char *)(&DAT_006f1894 + unaff_EDI * 8) != '\0') {
        uVar2 = (**(code **)(DAT_006f1d20 + 0x80))(param_1);
        return uVar2;
      }
      return 0;
    }
    if (*(char *)(&DAT_006f1894 + unaff_EDI * 8) == '\0') {
      return 0;
    }
    if (((&DAT_006f1898)[unaff_EDI * 8] != -1) && (param_1 != (&DAT_006f1898)[unaff_EDI * 8])) {
      return 0;
    }
    if ((*(short *)(&DAT_006f189c + unaff_EDI * 8) != -1) &&
       (*(int *)(in_EAX + 0x20) != (int)*(short *)(&DAT_006f189c + unaff_EDI * 8))) {
      return 0;
    }
    if ((&DAT_006f18a0)[unaff_EDI * 8] == -1) {
      return 1;
    }
    bVar3 = param_1 == (&DAT_006f18a0)[unaff_EDI * 8];
  }
  if (!bVar3) {
    return 1;
  }
  return 0;
}
#endif
