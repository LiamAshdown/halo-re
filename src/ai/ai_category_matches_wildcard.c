// ai_category_matches_wildcard  (Ghidra: FUN_00433ba0; really the ai_allegiance worker)
// address 0x433ba0, size 199 bytes
// name confidence: 0.3   rewrite confidence: 0.9
// REWRITTEN from objdump 0x433ba0..0x433c66 (the draft dropped both teams and called team_pair_override_add with a
//   single flag). Makes two teams allies (hs ai_allegiance). When one side is the player team (1) and the other is
//   human (2) or sentinel (5), the alliance can be broken by betrayal: threshold 5, a difficulty-scaled forgiveness
//   time (300/450/1200/2700 from game globals +0x0e) and, for humans, the betrayal-by-player flag; each side's
//   "is that team" flag says which of the two is the non-player team.
// blam-cc: stack -> team_a, AX -> team_b
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern game_main_globals *main_game_globals; // 0x006b0b80

extern void team_pair_override_add(int16_t index_a, uint8_t unknown_08, int16_t index_b, uint8_t unknown_09,
    int16_t threshold, int16_t timer_reset, uint8_t unknown_0c); // 0x45be50, EAX, stack

void ai_category_matches_wildcard(int16_t category, int16_t other_category)
{
    static const int16_t k_forgiveness_ticks[4] = {300, 450, 1200, 2700};
    int16_t team_a = category;
    int16_t team_b = other_category;
    int16_t other = -1;
    int16_t threshold = -1;
    int16_t timer = -1;
    uint8_t betrayable = 0;
    uint8_t human = 0;
    uint8_t b_is_other = 0;
    uint8_t a_is_other = 0;

    if (team_a == -1 || team_b == -1) {
        return;
    }
    if (team_a == 1) {
        other = team_b;
    } else if (team_b == 1) {
        other = team_a;
    }
    if (other == 2 || other == 5) {
        timer = k_forgiveness_ticks[main_game_globals->difficulty & 3];
        human = other == 2;
        betrayable = 1;
        threshold = 5;
        b_is_other = team_b == other;
    }
    if (betrayable && team_a == other) {
        a_is_other = 1;
    }
    team_pair_override_add(team_a, a_is_other, team_b, b_is_other, threshold, timer, human);
}

#if 0
Original Ghidra decompilation (0x433ba0):

void FUN_00433ba0(short param_1)

{
  bool bVar1;
  short in_AX;
  short sVar2;
  undefined4 uVar3;

  if ((param_1 != -1) && (in_AX != -1)) {
    bVar1 = false;
    sVar2 = in_AX;
    if ((param_1 != 1) && (sVar2 = -1, in_AX == 1)) {
      sVar2 = param_1;
    }
    if (((((sVar2 == 2) || (sVar2 == 5)) && (bVar1 = true, in_AX == sVar2)) || (bVar1)) &&
       (param_1 == sVar2)) {
      uVar3 = 1;
    }
    else {
      uVar3 = 0;
    }
    FUN_0045be50(uVar3);
  }
  return;
}
#endif
