// objects_set_ambient_cluster_override  (named by types/objects.h's own struct comment:
// "object_globals.ambient_cluster_mode, set by objects_set_ambient_cluster_override and read
// by objects_get_ambient_cluster")
// address 0x4f79d0, size 115 bytes
// name confidence: 0.85 (fixed by types/objects.h's own citation of this address by this name)
// rewrite confidence: 0.55 (raised from 0.4 by the phase-4 review pass: the leaf/cluster
// lookup was corrected against the disassembly)
// evidence: types/objects.h object_globals (ambient_cluster_mode 0x90, ambient_cluster_index
//   0x94, object_ambient_cluster_mode enum); global 0x006b8cbc object_globals_pointer.
// register convention: a local-player index in AX. Confirmed against objdump -d -M intel
//   bin/halo.exe: 0x4f79d0 cmp ax,0xffff at entry, no stack access.
//   // blam-cc: AX -> local_player_index
// UNSURE: bsp3d_node_find_leaf is called here as (globals=ECX, point=EDX, index=EAX), which differs
//   from src/objects/object_set_cluster_and_parent.c's plain-C call shape for the same callee
//   (that file does not claim a specific register mapping beyond its own call site); resolved
//   directly from this function's own disassembly (0x4f79fb mov ecx,DAT_00746f90 /
//   0x4f7a01 xor eax,eax / 0x4f7a03 call 0x5013a0). UNSURE: the player-record field at
//   DAT_00746f8c[player]+0x28 (stride 0x68) fed in as the probe point is not otherwise named in
//   this module; likely an eye/camera position on a foreign player struct.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern object_globals *object_globals_pointer; // 0x006b8cbc
extern void *global_globals; // 0x00746f90
extern uint8_t *player_globals_table; // 0x00746f8c, UNSURE: see file header, stride 0x68
extern uint8_t *structure_bsp_globals; // 0x00746f9c

extern int32_t bsp3d_node_find_leaf(void *globals, real_point3d *point, int32_t index); // 0x5013a0, this call site: ECX -> globals, EDX -> point, EAX -> index

void objects_set_ambient_cluster_override(int16_t local_player_index) // blam-cc: AX -> local_player_index
{
    if (local_player_index != -1) {
        uint8_t *player_base = *(uint8_t **)(player_globals_table + 0x4f4);
        real_point3d *point = (real_point3d *)(player_base + local_player_index * 0x68 + 0x28);
        int32_t leaf = bsp3d_node_find_leaf(global_globals, point, 0);

        if (leaf != -1) {
            // PHASE-4 REVIEW: 0x4f7a13 `mov edx,[ecx+0xe4]` / `and eax,0x7fffffff` /
            // `shl eax,4` -- the +0xe4 slot is a pointer and the index is masked; the
            // earlier rewrite did neither.
            int16_t cluster = *(int16_t *)(*(uint8_t **)(structure_bsp_globals + 0xe4) +
                                           (uint32_t)(leaf & 0x7fffffff) * 0x10 + 8);
            if (cluster != -1) {
                object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_override;
                object_globals_pointer->ambient_cluster_index = cluster;
                return;
            }
        }
    }

    object_globals_pointer->ambient_cluster_mode = _object_ambient_cluster_none;
}

#if 0
Original Ghidra decompilation (0x4f79d0):

void FUN_004f79d0(void)

{
  short sVar1;
  int iVar2;
  short in_AX;
  int iVar3;

  if (in_AX != -1) {
    iVar3 = FUN_005013a0();
    iVar2 = DAT_006b8cbc;
    if ((iVar3 != -1) &&
       (sVar1 = *(short *)(iVar3 * 0x10 + 8 + *(int *)(DAT_00746f9c + 0xe4)), sVar1 != -1)) {
      *(undefined2 *)(DAT_006b8cbc + 0x90) = 2;
      *(short *)(iVar2 + 0x94) = sVar1;
      return;
    }
  }
  *(undefined2 *)(DAT_006b8cbc + 0x90) = 0;
  return;
}
#endif
