// hud_draw_teammate_nameplate  (Ghidra: FUN_0045e520; named per out/phase4/game_functions.md)
// address 0x45e520, size 343 bytes
// name confidence: 0.4   rewrite confidence: 0.35
// evidence: out/phase4/game_functions.md ("Tracks a hysteresis-stabilized nameplate target found
// by hud_find_nearby_teammate_for_nameplate and draws its name and a scaled value onscreen");
// types/game.h player (local_player_index +0x02, unit +0x34, unknown_7c/unknown_80 hysteresis
// pair, name +0x04). FUN_00474db0 (called from hud_find_nearby_teammate_for_nameplate) must
// return a *player* handle, not an object/unit one, because player+0x7c is validated here with
// the exact datum_get pattern against the player data_array.
// register convention: player handle in EAX (in_EAX).
//   // blam-cc: EAX -> player
// UNSURE: hud_draw_teammate_nameplate_text and FUN_006283c0 are outside this batch's range; FUN_006283c0's return is
// treated as a plain float (a per-frame time delta or fade weight) multiplied by 0.5.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include <string.h>
#include <wchar.h>
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// wcsncpy (0x00627a94 wcsncpy) comes from <wchar.h>; memset is inlined by the compiler.
extern data_array *player_data; // 0x0087a480

extern datum_index hud_find_nearby_teammate_for_nameplate(datum_index player_handle); // 0x45e340, this batch
extern void hud_draw_teammate_nameplate_text(wchar_t *text, int32_t value); // 0x461f20, not in this batch
extern double pow(double base, double exponent); // C runtime (the retail copy is the CRT _CIpow at 0x6283c0)

// blam-cc: EAX -> player
// Advances `player`'s nameplate-target hysteresis counter (player+0x80, 0..15) toward whatever
// hud_find_nearby_teammate_for_nameplate currently reports (ignoring a self-match), latching a
// new tracked target (player+0x7c) only once the counter reaches either end. Draws the tracked
// target's name when one is latched.
void hud_draw_teammate_nameplate(datum_index player_handle)
{
    player *p;
    datum_index found;
    player *tracked;
    wchar_t name[12];

    p = (player *)((uint8_t *)player_data->data + (player_handle & 0xffff) * sizeof(player));
    found = (datum_index)0xffffffff;

    if (p->local_player_index != -1 && p->unit != (datum_index)0xffffffff) {
        found = hud_find_nearby_teammate_for_nameplate(player_handle);
        if (found == player_handle) {
            found = (datum_index)0xffffffff;
        }
    }

    if ((datum_index)p->nameplate_target_player == found) {
        if (p->nameplate_fade_ticks < 0xf) {
            p->nameplate_fade_ticks = p->nameplate_fade_ticks + 1;
        }
    } else {
        if (0 < p->nameplate_fade_ticks) {
            p->nameplate_fade_ticks = p->nameplate_fade_ticks - 1;
        }
        if (p->nameplate_fade_ticks == 0) {
            p->nameplate_target_player = found;
        }
    }

    if (p->nameplate_target_player != (datum_index)0xffffffff) {
        int16_t index = (int16_t)p->nameplate_target_player;
        if (-1 < index && index < player_data->maximum_count) {
            tracked = (player *)((uint8_t *)player_data->data + player_data->size * index);
            if (tracked->identifier != 0 &&
                ((int16_t)((uint32_t)p->nameplate_target_player >> 16) == 0 ||
                 tracked->identifier == (int16_t)((uint32_t)p->nameplate_target_player >> 16))) {
                memset(name, 0, sizeof(name));
                wcsncpy(name, (const wchar_t *)tracked->name, 0x0b);
                name[0x0b] = 0;
                // FIXED 2026-09-28: 0x45e5e8..0x45e649: the scale is pow(min(+0x80, 10) * 0.1, 1.9) * 0.5
                //   (0x00672c30 is the double 1.9f).
                hud_draw_teammate_nameplate_text(name,
                    (float)pow((double)((float)(p->nameplate_fade_ticks < 10 ? p->nameplate_fade_ticks : 10) * 0.1f), (double)1.9f) * 0.5f);
                return;
            }
        }
        p->nameplate_target_player = (datum_index)0xffffffff;
    }
}

#if 0
Original Ghidra decompilation (0x45e520), from tools/pack.py 0x45e520:

void FUN_0045e520(void)

{
  int iVar1;
  int iVar2;
  uint in_EAX;
  uint uVar3;
  short *psVar4;
  short sVar5;
  short sVar6;
  int iVar7;
  float10 fVar8;
  wchar_t local_18 [12];

  iVar7 = (in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34);
  uVar3 = 0xffffffff;
  if (((*(short *)(iVar7 + 2) != -1) && (*(int *)(iVar7 + 0x34) != -1)) &&
     (uVar3 = FUN_0045e340(), uVar3 == in_EAX)) {
    uVar3 = 0xffffffff;
  }
  iVar2 = DAT_0087a480;
  if (*(uint *)(iVar7 + 0x7c) == uVar3) {
    if (*(int *)(iVar7 + 0x80) < 0xf) {
      *(int *)(iVar7 + 0x80) = *(int *)(iVar7 + 0x80) + 1;
    }
  }
  else {
    if (0 < *(int *)(iVar7 + 0x80)) {
      *(int *)(iVar7 + 0x80) = *(int *)(iVar7 + 0x80) + -1;
    }
    if (*(int *)(iVar7 + 0x80) == 0) {
      *(uint *)(iVar7 + 0x7c) = uVar3;
    }
  }
  iVar1 = *(int *)(iVar7 + 0x7c);
  if (iVar1 != -1) {
    sVar5 = (short)iVar1;
    if ((-1 < sVar5) && (sVar5 < *(short *)(iVar2 + 0x20))) {
      psVar4 = (short *)((int)*(short *)(iVar2 + 0x22) * (int)sVar5 + *(int *)(iVar2 + 0x34));
      sVar5 = *psVar4;
      if ((sVar5 != 0) && ((sVar6 = (short)((uint)iVar1 >> 0x10), sVar6 == 0 || (sVar5 == sVar6))))
      {
        local_18[1] = L'\0';
        local_18[2] = L'\0';
        local_18[3] = L'\0';
        local_18[4] = L'\0';
        local_18[5] = L'\0';
        local_18[6] = L'\0';
        local_18[7] = L'\0';
        local_18[8] = L'\0';
        local_18[9] = L'\0';
        local_18[10] = L'\0';
        local_18[0] = L'\0';
        local_18[0xb] = 0;
        _wcsncpy(local_18,psVar4 + 2,0xb);
        local_18[0xb] = 0;
        fVar8 = (float10)FUN_006283c0();
        FUN_00461f20(local_18,(float)(fVar8 * (float10)0.5));
        return;
      }
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
