// unit_get_first_person_marker_transform  (Ghidra: FUN_00492c30; named per
// symbols/review_queue.txt "0x492c30 unit_get_first_person_marker_transform")
// address 0x492c30, size 225 bytes
// name confidence: 0.35   rewrite confidence: 0.5
// evidence: out/phase4/interface_functions.md "Fetches a first-person weapon marker's transform
// for an object, but only if that object belongs to the current local player." Disassembled
// directly (objdump bin/halo.exe 0x492c30..0x492d10) since Ghidra shows the
// first_person_weapon_get_marker_data call with zero visible arguments; this also confirms the
// output fields are the raw (unsubtracted) node_transform.position/forward/up triples, unlike
// first_person_weapon_center_flashlight.c which offsets position by half of forward.
// register convention: object_index in EDX, out_extents in EDI, out_direction in ESI (all
// unrecognized by Ghidra); marker_name (dead, forwarded but unused by the callee) and
// out_position are the two Ghidra-recognized stack parameters (param_1, param_2).
// // blam-cc: object_index=EDX, out_extents=EDI, out_direction=ESI, marker_name=stack,
// out_position=stack

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0, "objects"
extern data_array *player_data;      // 0x0087a480, "players"
extern int16_t current_local_player_index; // 0x007c3108
extern first_person_weapon_interface *first_person_weapon_interfaces; // 0x006b2d98

extern uint32_t first_person_weapon_get_marker_data(datum_index weapon_index, const char *marker_name,
                                                      object_marker *out, uint32_t name_arg); // 0x492ad0, this module

// If object_index's parent object (e.g. the vehicle or unit wielding it) is controlled by the
// current local player, and that player's first-person weapon is attached, looks up
// object_index's first-person marker and returns its raw node_transform position, forward
// (as extents) and up (as direction) vectors. Returns 0 on any failed gate.
uint8_t unit_get_first_person_marker_transform(datum_index object_index, const char *marker_name,
                                                real_point3d *out_position, real_vector3d *out_extents,
                                                real_vector3d *out_direction)
{
    object_header *header;
    object *obj;
    object_header *parent_header;
    unit_data *parent_unit;
    datum_index controlling_player;
    player *p;
    int16_t local_player;
    first_person_weapon_interface *fp;
    object_marker marker;
    int16_t result;

    header = &((object_header *)object_data->data)[object_index & 0xffff];
    obj = header->data;

    parent_header = &((object_header *)object_data->data)[obj->parent_object & 0xffff];
    parent_unit = (unit_data *)((uint8_t *)parent_header->data + k_unit_data_offset);
    controlling_player = parent_unit->controlling_player;

    if (controlling_player == (datum_index)0xffffffff) {
        return 0;
    }

    p = (player *)((uint8_t *)player_data->data + (controlling_player & 0xffff) * sizeof(player));
    local_player = p->local_player_index;
    if (local_player == -1 || local_player != current_local_player_index) {
        return 0;
    }

    fp = &first_person_weapon_interfaces[local_player];
    if (fp->attached == 0) {
        return 0;
    }

    result = (int16_t)first_person_weapon_get_marker_data(object_index, marker_name, &marker, 1);
    if (result <= 0) {
        return 0;
    }

    *out_position = marker.node_transform.position;
    *out_extents = marker.node_transform.forward;
    *out_direction = marker.node_transform.up;
    return 1;
}

#if 0
Original Ghidra decompilation (0x492c30):

undefined4 FUN_00492c30(undefined4 param_1,undefined4 *param_2)

{
  uint uVar1;
  short sVar2;
  uint in_EDX;
  undefined4 *unaff_ESI;
  undefined4 *unaff_EDI;
  undefined4 local_30;
  undefined4 local_2c;
  undefined4 local_28;
  undefined4 local_18;
  undefined4 local_14;
  undefined4 local_10;
  undefined4 local_c;
  undefined4 local_8;
  undefined4 local_4;

  uVar1 = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                            (*(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                               (in_EDX & 0xffff) * 0xc) + 0x11c) & 0xffff) * 0xc) +
                   0x218);
  if (uVar1 != 0xffffffff) {
    sVar2 = *(short *)((uVar1 & 0xffff) * 0x200 + *(int *)(DAT_0087a480 + 0x34) + 2);
    if (((sVar2 != -1) && (sVar2 == DAT_007c3108)) &&
       (*(char *)(sVar2 * 0x1ea0 + DAT_006b2d98) != '\0')) {
      sVar2 = first_person_weapon_get_marker_data();
      if (0 < sVar2) {
        *param_2 = local_c;
        param_2[1] = local_8;
        param_2[2] = local_4;
        *unaff_EDI = local_30;
        unaff_EDI[1] = local_2c;
        unaff_EDI[2] = local_28;
        *unaff_ESI = local_18;
        unaff_ESI[1] = local_14;
        unaff_ESI[2] = local_10;
        return 1;
      }
    }
  }
  return 0;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
