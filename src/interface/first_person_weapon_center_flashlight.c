// first_person_weapon_center_flashlight  (Ghidra: already named)
// address 0x492b80, size 172 bytes
// name confidence: 0.75   rewrite confidence: 0.6
// evidence: out/phase4/interface_functions.md "Computes the centered world-space origin,
// extents, and direction of the local player's first-person weapon flashlight marker."
// Disassembled directly (objdump bin/halo.exe 0x492b80..0x492c2b) to resolve the stack-allocated
// object_marker buffer's field offsets, which Ghidra split into disconnected "local_XX" floats:
// the extents/origin source is marker.node_transform.forward, the direction source is
// marker.node_transform.up, and the origin subtracts forward * 0.5 (the .rdata constant at
// 0x00672abc, per types/devices.h) from marker.node_transform.position.
// register convention: out_origin is the one Ghidra-recognized stack parameter (param_1);
//   unit_index (EDI), out_extents (EBX) and out_direction (ESI) are all unrecognized register
//   arguments Ghidra shows as unaff_*.
//   // blam-cc: EDI -> unit_index, EBX -> out_extents, ESI -> out_direction, stack -> out_origin
// FIXED (register inputs, objdump): ESI carries out_direction (read at 0x492c1f, mov [esi],eax);
// it was already named in the prose above but the note's line-wrap broke the checker's parser
// (continuation lines need the file's usual two-space "//" indent), so ESI was silently dropped.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern int32_t local_player_index_for_unit(datum_index unit_index); // 0x4940a0, this module
extern uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
                                                      object_marker *out, uint32_t name_arg); // 0x492ad0, this module

// If unit_index belongs to a local player with an attached first-person weapon, looks up the
// weapon model's "flashlight" marker and derives a centered origin (marker position offset back
// half the marker's forward extent), the raw forward vector as an extents triple, and the
// marker's up vector as a direction triple.
void first_person_weapon_center_flashlight(datum_index unit_index, real_point3d *out_origin,
                                            real_vector3d *out_extents, real_vector3d *out_direction)
{
    int32_t local_player;
    first_person_weapon_interface *fp;
    object_marker marker;
    int16_t result;

    local_player = local_player_index_for_unit(unit_index);
    if ((int16_t)local_player == -1) {
        return;
    }

    fp = &first_person_weapon_interfaces[local_player];
    if (fp->attached == 0) {
        return;
    }

    result = (int16_t)first_person_weapon_get_marker_data(fp->weapon_index, "flashlight", &marker, 1);
    if (result <= 0) {
        return;
    }

    out_origin->x = marker.node_transform.position.x - marker.node_transform.forward.i * 0.5f;
    out_origin->y = marker.node_transform.position.y - marker.node_transform.forward.j * 0.5f;
    out_origin->z = marker.node_transform.position.z - marker.node_transform.forward.k * 0.5f;

    out_extents->i = marker.node_transform.forward.i;
    out_extents->j = marker.node_transform.forward.j;
    out_extents->k = marker.node_transform.forward.k;

    out_direction->i = marker.node_transform.up.i;
    out_direction->j = marker.node_transform.up.j;
    out_direction->k = marker.node_transform.up.k;
}

#if 0
Original Ghidra decompilation (0x492b80):

void first_person_weapon_center_flashlight(float *param_1)

{
  short sVar1;
  float *unaff_EBX;
  undefined4 *unaff_ESI;
  undefined1 local_6c [60];
  float local_30;
  float local_2c;
  float local_28;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  float local_c;
  float local_8;
  float local_4;

  sVar1 = FUN_004940a0();
  if ((sVar1 != -1) && (*(char *)(sVar1 * 0x1ea0 + DAT_006b2d98) != '\0')) {
    sVar1 = first_person_weapon_get_marker_data
                      (*(undefined4 *)(sVar1 * 0x1ea0 + DAT_006b2d98 + 8),"flashlight",local_6c,1);
    if (0 < sVar1) {
      *param_1 = local_c - local_30 * 0.5;
      param_1[1] = local_8 - local_2c * 0.5;
      param_1[2] = local_4 - local_28 * 0.5;
      *unaff_EBX = local_30;
      unaff_EBX[1] = local_2c;
      unaff_EBX[2] = local_28;
      *unaff_ESI = local_18;
      unaff_ESI[1] = local_14;
      unaff_ESI[2] = local_10;
    }
  }
  return;
}

Disassembly (objdump, 0x492b80..0x492c2b) pinning the object_marker offsets used above:

  492b80: sub esp,0x6c              ; object_marker marker; (size 0x6c) at [esp+4..esp+0x70)
  492b88: call 0x4940a0              ; local_player_index_for_unit(edi = unit_index)
  492b97: mov edx,0x6b2d98
  492ba0: imul eax,eax,0x1ea0
  492ba6: mov cl,[eax+edx]           ; cl = first_person_weapon_interfaces[player].attached
  492baf: mov edx,[eax+8]             ; edx = fp->weapon_index
  492bb2: push 1                      ; name_arg = 1
  492bb4: lea ecx,[esp+8]             ; ecx = &marker
  492bb9: push 0x66991c               ; "flashlight" (unused inside the callee, see its own file)
  492bbe: push edx                    ; weapon_index
  492bbf: call 0x492ad0                ; first_person_weapon_get_marker_data
  492bcc: flds [esp+0x40]             ; [esp+0x40] == marker+0x3c == node_transform.forward.i
  492bd4: fmuls 0x672abc               ; * 0.5f
  492be2: fsubrs [esp+0x64]           ; [esp+0x64] == marker+0x60 == node_transform.position.x
  492be6: fstps [ebp+0]                ; out_origin->x
  ... (same pattern for .y at +0x44/+0x68/+0x30(0x672ac0? no, same 0x672abc)/[ebp+4], .z at +0x48/+0x6c/[ebp+8])
  492c0b: mov [ebx],eax                ; out_extents->i = node_transform.forward.i (raw copy)
  492c0d: mov eax,[esp+0x58]           ; [esp+0x58] == marker+0x54 == node_transform.up.i
  492c1f: mov [esi],eax                ; out_direction->i = node_transform.up.i
  ; (.j/.k of both follow the same +4/+8 pattern)
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
