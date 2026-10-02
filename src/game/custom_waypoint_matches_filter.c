// custom_waypoint_matches_filter  (Ghidra: FUN_004620c0; renamed per its summary)
// address 0x4620c0, size 204 bytes
// VERIFIED against disassembly 0x4620c0..0x46218c (2026-09-30)
// name confidence: 0.4   rewrite confidence: 0.85
// evidence: out/phase4/game_functions.md ("Register-argument helper that tests whether one entry
// of the 32-slot custom-waypoint table applies to a given object/player, honoring -1 wildcard
// sentinels for player, team-type and owner filters"); types/game.h custom_waypoint (active
// +0x0c, player +0x10, team +0x14, owner +0x18), game_engine_index (_game_engine_ctf == 1),
// game_engine_definition::waypoint_filter (+0x80).
// register convention: EAX = a pointer to the reference player record (only its team, +0x20, is read; it is also
// forwarded to game_engine_ctf_unit_is_flag_holder in EAX); EDI = the waypoint table slot INDEX (the slot address is
// 0x6f1888 + index*0x20); the stack parameter is the candidate player/object handle.
//   // blam-cc: EAX -> reference_player, EDI -> slot_index, stack -> candidate
// FIXED 2026-09-30 (disassembly): the draft took EAX as an int "reference_team" and EDI as a slot POINTER; the binary
// dereferences EAX+0x20 and computes the slot address from the EDI index. The engine's waypoint_filter hook is
// called cdecl with (candidate, slot_index).
// The CTF branch: a slot matches when its player/team filters pass and the candidate is not its owner; otherwise a
// slot on the reference player's own team never matches, an ownerless one never matches, and the rest match only when
// game_engine_ctf_unit_is_flag_holder(reference_player) is nonzero (its real meaning is not established).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern custom_waypoint custom_waypoints[k_maximum_custom_waypoints]; // 0x006f1888

extern uint8_t game_engine_ctf_unit_is_flag_holder(player *p); // 0x469780, EAX player

// blam-cc: EAX -> reference_player, EDI -> slot_index, stack -> candidate
uint8_t custom_waypoint_matches_filter(int32_t candidate, player *reference_player, int32_t slot_index)
{
    custom_waypoint *slot = &custom_waypoints[slot_index];

    if (current_game_engine == 0) {
        return 0;
    }

    if (current_game_engine->index == _game_engine_ctf && slot->active != 0) {
        if ((slot->player == (datum_index)0xffffffff || candidate == (int32_t)slot->player) &&
            (slot->team == -1 || reference_player->team == (int32_t)slot->team)) {
            if (slot->owner == (datum_index)0xffffffff || candidate != (int32_t)slot->owner) {
                return 1;
            }
        }
        if (reference_player->team == (int32_t)slot->team) {
            return 0;
        }
        if (slot->owner == (datum_index)0xffffffff) {
            return 0;
        }
        return game_engine_ctf_unit_is_flag_holder(reference_player) != 0;
    }

    if (current_game_engine->waypoint_filter != 0) {
        if (slot->active == 0) {
            return 0;
        }
        return ((uint8_t (*)(int32_t, int32_t))current_game_engine->waypoint_filter)(candidate, slot_index);
    }
    if (slot->active == 0) {
        return 0;
    }
    if (slot->player != (datum_index)0xffffffff && candidate != (int32_t)slot->player) {
        return 0;
    }
    if (slot->team != -1 && reference_player->team != (int32_t)slot->team) {
        return 0;
    }
    if (slot->owner == (datum_index)0xffffffff) {
        return 1;
    }
    return candidate != (int32_t)slot->owner;
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
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
