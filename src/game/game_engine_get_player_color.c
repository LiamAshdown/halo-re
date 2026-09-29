// game_engine_get_player_color  (Ghidra: game_engine_get_player_color, already named)
// address 0x463290, size 101 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Returns a 3-float RGB color for a player: either a
// resolved custom palette color or one of two fixed default colors"); types/game.h
// game_variant::teams (+0x34, aliased 0x006f1cbc), player::team (+0x20).
// register convention: a player index in EAX (in_EAX); the output RGB buffer in ESI
// (unaff_ESI). The function also RETURNS out_rgb -- 0x4632ee is "mov eax,esi" before the ret,
// and its caller player_respawn (0x478103) immediately dereferences that returned pointer.
// CORRECTED by review (objdump 0x463290..0x4632f5): the return value was missing, and the
// no-teams branch passes player + 0x60 (a WORD colour index) to 0x539c20 together with a
// 12-byte stack scratch in EAX -- Ghidra rendered that as a plain call taking the player index.
//   // blam-cc: EAX -> player_index, unaff_ESI -> out_rgb

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"

extern data_array *player_data; // 0x0087a480
extern game_variant game_engine_variant; // 0x006f1c88 (teams aliased 0x006f1cbc)
extern real *default_color_a; // 0x00686b10 holds a POINTER to three floats ('mov eax,ds:0x686b10'
                              //   then 'mov ecx,[eax]'), not the floats themselves.
                              //   UNSURE identity (team 0 default color?)
extern real *default_color_b; // 0x00686b18, same indirection. UNSURE identity (team 1?)

extern real *player_color_get_rgb(real *out_rgb, int32_t color_index); // 0x539c20;
    // blam-cc: EAX -> out_rgb, ECX -> color_index. Returns a pointer to three floats.

// blam-cc: EAX -> player_index, unaff_ESI -> out_rgb
real *game_engine_get_player_color(uint32_t player_index, real *out_rgb)
{
    player *p = (player *)((uint8_t *)player_data->data + (player_index & 0xffff) * sizeof(player));
    real scratch[3];
    real *color;

    if (game_engine_variant.teams == 0) {
        color = player_color_get_rgb(scratch, (int32_t)*(int16_t *)&p->color_index);
    } else if (p->team == 0) {
        color = default_color_a;
    } else {
        color = default_color_b;
    }
    out_rgb[0] = color[0];
    out_rgb[1] = color[1];
    out_rgb[2] = color[2];
    return out_rgb;
}

#if 0
Original Ghidra decompilation (0x463290), from tools/pack.py 0x463290:

void game_engine_get_player_color(void)

{
  undefined4 uVar1;
  uint in_EAX;
  undefined4 *puVar2;
  undefined4 uVar3;
  undefined4 uVar4;
  undefined4 *unaff_ESI;

  if (DAT_006f1cbc == '\0') {
    puVar2 = (undefined4 *)player_color_get_rgb();
    uVar3 = *puVar2;
    uVar4 = puVar2[1];
  }
  else if (*(int *)((in_EAX & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 0x20) == 0) {
    uVar3 = *(undefined4 *)PTR_DAT_00686b10;
    uVar4 = *(undefined4 *)(PTR_DAT_00686b10 + 4);
    puVar2 = (undefined4 *)PTR_DAT_00686b10;
  }
  else {
    uVar3 = *(undefined4 *)PTR_DAT_00686b18;
    uVar4 = *(undefined4 *)(PTR_DAT_00686b18 + 4);
    puVar2 = (undefined4 *)PTR_DAT_00686b18;
  }
  uVar1 = puVar2[2];
  *unaff_ESI = uVar3;
  unaff_ESI[1] = uVar4;
  unaff_ESI[2] = uVar1;
  return;
}
#endif
