// game_engine_is_object_winning  (Ghidra: FUN_00463660; renamed per its summary)
// address 0x463660, size 195 bytes
// name confidence: 0.3   rewrite confidence: 0.2
// evidence: out/phase4/game_functions.md ("Determines whether a given object belongs to the
// currently leading/winning team"); types/game.h scoreboard_entry::place (+0x18, "bit
// 0x80000000 marks a tie with the entry above"), game_variant::teams (+0x34, aliased
// 0x006f1cbc), game_engine_definition::get_team_score (+0x50); this batch's
// game_engine_get_player_scoreboard_entry (0x45cee0), game_engine_players_ready_for_bsp_switch
// (0x45c750) and game_engine_find_first_eligible_player_on_team (0x45c9e0), all already written.
// register convention: a player/object handle in EAX (in_EAX).
//   // blam-cc: EAX -> handle
// UNSURE: in non-team mode, a rank tied exactly for first place (place == 0x80000000) falls
// through to the shared "return 0xffffffff" at the bottom, distinct from an outright rank-0 win
// (returns 1) or any other rank (returns 0). In team mode, `bVar7 != 0xffffffff` is always true
// (bVar7 only ever holds 0 or 1) and is transcribed as dead code rather than removed.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern game_engine_definition *current_game_engine; // 0x006f1d20
extern data_array *player_data;                     // 0x0087a480
extern game_variant game_engine_variant;            // 0x006f1c88 (teams aliased 0x006f1cbc)

extern void game_engine_get_player_scoreboard_entry(datum_index player_handle, scoreboard_entry *out); // 0x45cee0
extern uint8_t game_engine_players_ready_for_bsp_switch(void); // 0x45c750
extern uint8_t game_engine_find_first_eligible_player_on_team(int32_t team); // 0x45c9e0

// blam-cc: EAX -> handle
uint32_t game_engine_is_object_winning(uint32_t handle)
{
    if (game_engine_variant.teams == 0) {
        scoreboard_entry entry;
        game_engine_get_player_scoreboard_entry((datum_index)handle, &entry);
        if ((entry.place & 0x80000000) == 0 || (entry.place & 0x7fffffff) != 0) {
            return (entry.place & 0x7fffffff) == 0;
        }
    } else {
        int32_t team0_score = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(0);
        int32_t team1_score = ((int32_t (*)(int32_t))current_game_engine->get_team_score)(1);
        uint8_t winning_team;

        if (game_engine_players_ready_for_bsp_switch() == 0) {
            winning_team = (game_engine_find_first_eligible_player_on_team(0) == 0);
        } else {
            if (team0_score == team1_score) {
                return 0xffffffff;
            }
            winning_team = team0_score <= team1_score;
        }
        if (winning_team != 0xffffffff) { // always true; see file header
            player *p = (player *)((uint8_t *)player_data->data + (handle & 0xffff) * sizeof(player));
            return (uint32_t)(p->team == (int32_t)winning_team);
        }
    }
    return 0xffffffff;
}

#if 0
Original Ghidra decompilation (0x463660), from tools/pack.py 0x463660:

uint FUN_00463660(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  undefined4 *puVar4;
  int iVar5;
  undefined4 *puVar6;
  bool bVar7;
  undefined4 local_38 [6];
  uint local_20;

  if (DAT_006f1cbc == '\0') {
    puVar4 = (undefined4 *)FUN_0045cee0();
    puVar6 = local_38;
    for (iVar5 = 7; iVar5 != 0; iVar5 = iVar5 + -1) {
      *puVar6 = *puVar4;
      puVar4 = puVar4 + 1;
      puVar6 = puVar6 + 1;
    }
    if (((local_20 & 0x80000000) == 0) || ((local_20 & 0x7fffffff) != 0)) {
      return (uint)((local_20 & 0x7fffffff) == 0);
    }
  }
  else {
    iVar2 = (**(code **)(DAT_006f1d20 + 0x50))(0);
    iVar3 = (**(code **)(DAT_006f1d20 + 0x50))(1);
    iVar5 = *(int *)(DAT_0087a480 + 0x34);
    cVar1 = FUN_0045c750();
    if (cVar1 == '\0') {
      cVar1 = FUN_0045c9e0(0);
      bVar7 = cVar1 == '\0';
    }
    else {
      if (iVar2 == iVar3) {
        return 0xffffffff;
      }
      bVar7 = iVar2 <= iVar3;
    }
    if (bVar7 != 0xffffffff) {
      return (uint)(*(uint *)((in_EAX & 0xffff) * 0x200 + iVar5 + 0x20) == (uint)bVar7);
    }
  }
  return 0xffffffff;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
