// observer_update_location  (no Ghidra function; new entry)
// address 0x447a60, size 80 bytes (0x447a60..0x447aaf)
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: out/phase4/camera_types_notes.md "0x447a60: observer update location". It is a slot
//   of the .data procedure table at 0x0069e8dc..0x0069e934 (entry 0x0069e900), next to
//   objects / structure procedures that recompute BSP membership. When a local player exists it
//   recomputes observers[0].camera leaf_index / cluster_index for the current position (the
//   same lookup observer_commit does, without the predicted resource touch).
// register convention: none; cdecl, no arguments.
// No Ghidra decompilation exists for this address; the #if 0 block carries the objdump.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "structures.h"
#include "camera.h"

extern player_globals *local_player_globals;      // 0x0087a478
extern ModelCollisionGeometryBSP *global_globals; // 0x00746f90, the structure collision BSP (types/structures.h)
extern ScenarioStructureBSP *structure_bsp;       // 0x00746f9c
extern observer observers[1];                     // 0x006ac65c

// blam-cc: EAX -> node_index, ECX -> bsp, EDX -> point
extern uint32_t bsp3d_node_find_leaf(int32_t node_index, ModelCollisionGeometryBSP *bsp,
    real_point3d *point);                         // 0x5013a0, physics module

void observer_update_location(void)
{
    int32_t leaf_index;

    if (local_player_globals->local_players[0] == k_datum_index_none) {
        return;
    }
    leaf_index = (int32_t)bsp3d_node_find_leaf(0, global_globals,
        (real_point3d *)&observers[0].camera.position);
    observers[0].camera.leaf_index = leaf_index;
    if (leaf_index == -1) {
        observers[0].camera.cluster_index = -1;
    } else {
        observers[0].camera.cluster_index = (int16_t)((ScenarioStructureBSPLeaf *)
            structure_bsp->leaves.pointer)[leaf_index & 0x7fffffff].cluster;
    }
}

#if 0
No Ghidra function exists at 0x447a60. objdump -d -M intel:

  447a60: mov eax,ds:0x87a478
  447a65: cmp DWORD PTR [eax+0x4],0xffffffff
  447a69: je 0x447aaf
  447a6b: mov ecx,DWORD PTR ds:0x746f90
  447a71: mov edx,0x6ac6d0                ; &observers[0].camera.position
  447a76: xor eax,eax
  447a78: call 0x5013a0
  447a7d: cmp eax,0xffffffff
  447a80: mov ds:0x6ac6dc,eax             ; leaf_index
  447a85: jne 0x447a90
  447a87: or eax,eax
  447a89: mov ds:0x6ac6e0,ax              ; cluster_index = -1
  447a8f: ret
  447a90: mov ecx,DWORD PTR ds:0x746f9c
  447a96: mov edx,DWORD PTR [ecx+0xe4]    ; leaves.pointer
  447a9c: and eax,0x7fffffff
  447aa1: shl eax,0x4
  447aa4: movsx eax,WORD PTR [eax+edx*1+0x8]
  447aa9: mov ds:0x6ac6e0,ax
  447aaf: ret
#endif
