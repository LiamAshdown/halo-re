// render_object_is_camera_unit  (Ghidra: FUN_0050ea50; new name, evidence below. Disagrees with
// out/phase4/render_functions.md's phase-2 summary of a cluster test -- see the UNSURE note.)
// address 0x50ea50, size 112 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: disassembly (objdump -d -M intel, 0x50ea50..0x50eabf) traces every field this
//   function actually touches: player_globals.local_players[0] (types/game.h, +0x04, bound
//   checked to exactly local_player_index in [0,1) or -1) indexes the players data_array
//   (0x0087a480) to read player.unit (+0x34, a datum_index -- NOT a cluster), which is compared
//   against the function's own object argument. camera_get_type_for_player is called with CX
//   still holding current_local_player_index from the first load (a genuine reused register, not
//   a fresh argument). types/game.h's player.unit field pins this: the phase-2 summary ("tests
//   whether a cluster matches the camera's current cluster or media cluster") does not match any
//   field actually read here and is not used for this rewrite.
// register convention: ESI = object (datum_index to test).
//   // blam-cc: ESI=object
// UNSURE: 0x006869d0/0x006869d2/0x00686a04 (a bool, an int16 compared against 2, and a datum_index)
//   are not documented anywhere in types/; named generically for a director/theater-style camera
//   that can be watching a different object than the local player's own unit. Also UNSURE:
//   camera_get_type_for_player's return value of 0 is assumed to mean "normal" here too, matching
//   src/render/render_local_player_gunner_seat_visible.c's own note for the same callee. The
//   local player's own bound check compares current_local_player_index directly (0x007c3108, the
//   window loop's own copy), not player_globals::local_player_index.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_globals *local_player_globals; // 0x0087a478, game module
extern data_array *player_data;              // 0x0087a480, game module
extern int16_t current_local_player_index;    // 0x007c3108, this module

extern int16_t camera_get_type_for_player(int16_t player_index); // 0x445ac0, camera module;
                                                                  // blam-cc: CX=player_index

extern uint8_t camera_script;      // 0x006869d0, UNSURE
extern int16_t director_camera_mode;        // 0x006869d2, UNSURE
extern datum_index director_camera_target;  // 0x00686a04, UNSURE

// Tests whether `object` is the object the camera is currently effectively looking through: the
// local player's own driven unit (when there is exactly one local player and the camera is in its
// normal, non-cinematic mode), or, failing that, the director/theater camera's own tracked target
// object (when that mode is active and set to mode 2). Used to exclude that object from work that
// should not apply to whatever the viewer is currently inside of (e.g. the fake object shadow
// candidate list built by render_objects_collect, 0x50eac0).
uint8_t render_object_is_camera_unit(datum_index object) // blam-cc: ESI=object
{
    int16_t local_player_index = current_local_player_index;
    datum_index local_unit = k_datum_index_none;

    if (local_player_index != -1 && local_player_index < 1) {
        datum_index local_player = local_player_globals->local_players[local_player_index];
        if (local_player != k_datum_index_none) {
            player *p = &((player *)player_data->data)[(uint16_t)local_player];
            local_unit = p->unit;
        }
    }

    if (local_unit == object && camera_get_type_for_player(local_player_index) == 0) {
        return 1;
    }
    if (camera_script != 0 && director_camera_mode == 2 && director_camera_target == object) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x50ea50):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

undefined4 FUN_0050ea50(void)

{
  uint uVar1;
  short sVar2;
  int iVar3;
  int unaff_ESI;

  sVar2 = (short)_DAT_007c3108;
  if (((sVar2 == -1) || (0 < sVar2)) ||
     (uVar1 = *(uint *)(DAT_0087a478 + 4 + sVar2 * 4), uVar1 == 0xffffffff)) {
    iVar3 = -1;
  }
  else {
    iVar3 = *(int *)((uVar1 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
  }
  if ((iVar3 == unaff_ESI) && (sVar2 = camera_get_type_for_player(), sVar2 == 0)) {
    return 1;
  }
  if (((DAT_006869d0 != '\0') && (DAT_006869d2 == 2)) && (DAT_00686a04 == unaff_ESI)) {
    return 1;
  }
  return 0;
}

Disassembly (objdump -d -M intel, 0x50ea50..0x50eabf):

0050ea50:
  mov    ecx,DWORD PTR ds:0x7c3108      ; render_local_player_index
  cmp    cx,0xffff
  je     0x50ea74                       ; -1 -> no local unit
  cmp    cx,0x1
  jge    0x50ea74                       ; >= 1 -> no local unit (single local player slot)
  mov    edx,DWORD PTR ds:0x87a478      ; local_player_globals
  movsx  eax,cx
  mov    eax,DWORD PTR [edx+eax*4+0x4]  ; local_player_globals->local_players[cx]
  cmp    eax,0xffffffff
  jne    0x50ea79
0050ea74:
  or     eax,0xffffffff                 ; local_unit = -1
  jmp    0x50ea8e
0050ea79:
  mov    edx,DWORD PTR ds:0x87a480      ; player_data
  mov    edx,DWORD PTR [edx+0x34]
  and    eax,0xffff
  shl    eax,0x9                        ; * 0x200 (player stride)
  mov    eax,DWORD PTR [eax+edx*1+0x34] ; player.unit
0050ea8e:
  cmp    eax,esi                        ; local_unit == object
  jne    0x50ea9c
  call   0x445ac0                       ; camera_get_type_for_player(CX still = render_local_player_index)
  test   ax,ax
  je     0x50eaba
0050ea9c:
  mov    al,ds:0x6869d0
  ...
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
