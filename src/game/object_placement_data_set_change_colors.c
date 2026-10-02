// object_placement_data_set_change_colors  (Ghidra: FUN_00477670; renamed by this review. The
// first pass called it position_history_reset_all_slots against a locally invented
// `position_history` struct; the block is object_placement_data and the four 3-float slots it
// fills are that struct's four change colors.)
// address 0x477670, size 87 bytes
// name confidence: 0.7   rewrite confidence: 0.9
// evidence: VERIFIED against the disassembly of its only caller, player_respawn (0x477ea0),
//   at 0x4780a0..0x478124:
//     lea eax,[esp+0x30] ; call 0x4f53a0        -> object_placement_data_initialize(esp+0x30)
//     ... position -> [esp+0x48] (= +0x18), forward -> [esp+0x64] (= +0x34),
//         up -> [esp+0x70] (= +0x40)            -> confirms esp+0x30 is the placement block
//     lea esi,[esp+0xb8] ; mov eax,ebx ; call 0x463290   -> game_engine_get_player_color
//     mov ecx,[eax] / [eax+4] / [eax+8] -> [esp+0x24..0x2c]   (a 3-float RGB copy)
//     lea eax,[esp+0x24] ; lea ecx,[esp+0x30] ; call 0x477670
//   so EAX is an RGB colour and ECX is the placement block, and the four slots this function
//   writes at +0x58/+0x64/+0x70/+0x7c are object_placement_data + 0x58.
// types/objects.h calls that array `network_vectors` and types it real_vector3d; it is used
//   here as real_rgb_color[4], and 0x4f8b70 (the consumer object_new_with_datum_role_control
//   hands it to) copies it into object + 0x188 with count 4 immediately before
//   object_update_change_colors -- i.e. these are Blam's four change colors, not network
//   vectors. The field name is left alone so src/objects keeps compiling; see src/game/README.md
//   "Known gaps" for the cross-module correction this implies.
// register convention: EAX -> color, ECX -> placement.
//   // blam-cc: EAX -> color, ECX -> placement

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

// Writes the same RGB triple into all four of `placement`'s change-color slots, so the object
// player_respawn is about to create is tinted entirely in that player's colour.
void object_placement_data_set_change_colors(real *color, object_placement_data *placement)
{
    int32_t i;
    for (i = 0; i < 4; i = i + 1) {
        placement->network_vectors[i].i = color[0];  // red
        placement->network_vectors[i].j = color[1];  // green
        placement->network_vectors[i].k = color[2];  // blue
    }
}

#if 0
Original Ghidra decompilation (0x477670), from tools/pack.py 0x477670:

void FUN_00477670(void)

{
  undefined4 *in_EAX;
  int in_ECX;

  *(undefined4 *)(in_ECX + 0x58) = *in_EAX;
  *(undefined4 *)(in_ECX + 0x5c) = in_EAX[1];
  *(undefined4 *)(in_ECX + 0x60) = in_EAX[2];
  *(undefined4 *)(in_ECX + 100) = *in_EAX;
  *(undefined4 *)(in_ECX + 0x68) = in_EAX[1];
  *(undefined4 *)(in_ECX + 0x6c) = in_EAX[2];
  *(undefined4 *)(in_ECX + 0x70) = *in_EAX;
  *(undefined4 *)(in_ECX + 0x74) = in_EAX[1];
  *(undefined4 *)(in_ECX + 0x78) = in_EAX[2];
  *(undefined4 *)(in_ECX + 0x7c) = *in_EAX;
  *(undefined4 *)(in_ECX + 0x80) = in_EAX[1];
  *(undefined4 *)(in_ECX + 0x84) = in_EAX[2];
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
