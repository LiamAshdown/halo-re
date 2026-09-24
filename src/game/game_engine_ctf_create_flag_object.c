// game_engine_ctf_create_flag_object  (Ghidra: FUN_00468360; named per its summary)
// address 0x468360, size 196 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: VERIFIED against the disassembly (objdump -d -M intel --start-address=0x468360
//   --stop-address=0x468424): `mov esi,eax` at entry means the position pointer Ghidra shows as
//   in_EAX is preserved in ESI across the object_placement_data_initialize call and read back
//   afterward, so this function's own blam-cc is EAX -> position, stack -> name_index (Ghidra's
//   own visible `param_1`). object_placement_data_initialize (0x4f53a0, already committed,
//   src/objects/) is called with EAX -> &placement, definition_tag =
//   global_globals->multiplayer_information.pointer->flag.tag_id (types/tags.h
//   GlobalsMultiplayerInformation::flag), role = -1; it zeroes and defaults the whole struct, so
//   this function only overlays position and name_index on top. types/objects.h
//   object_type_definitions[12] (0x0069bfdc) and object_type_definition::unknown_10 (0x10);
//   types/tags.h Object tag's own object_type field (offset 0) read out of the flag tag's raw
//   data, matched against _object_type_weapon (2). object_mark_pending_delete (0x4f50f0,
//   already committed) matches the trailing header-flags dance exactly (clear
//   _object_header_in_pvs_pass_bit, then call it only if _object_header_active_bit was not
//   already set).
// register convention: position (real_point3d *) in EAX, name_index (uint16_t) on the stack.
//   // blam-cc: EAX -> position, stack -> name_index
// UNSURE: the exact significance of object_type_definition::unknown_10 != -1 as a "use role 0
//   instead of 3" gate (host-only) is not recovered beyond this literal transcription.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern Globals *global_globals;                                 // 0x00746fa0
extern int16_t network_game_mode;                                // 0x00719720
extern tag_instance *tag_instances;                              // 0x0087bc14
extern data_array *object_headers;                                // 0x008603b0
extern object_type_definition *object_type_definitions[k_maximum_object_types]; // 0x0069bfdc

extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role);          // 0x4f53a0
extern datum_index object_new_with_datum_role_control(object_placement_data *placement,
    uint32_t role);                                          // 0x4f54b0
extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0

// blam-cc: EAX -> position, stack -> name_index
// Builds a placement block for the current game's CTF flag weapon tag (role -1, otherwise
// zeroed/defaulted by object_placement_data_initialize), overlays `position` and `name_index`,
// then creates the object with role 0 when hosting and the flag tag's own object_type has a
// registered object_type_definition with unknown_10 != -1, or role 3 otherwise. Finally clears
// the new object header's _object_header_in_pvs_pass_bit and marks it pending-delete-eligible
// if it was not already active.
datum_index game_engine_ctf_create_flag_object(real_point3d *position, uint16_t name_index)
{
    object_placement_data placement;
    datum_index flag_tag;
    uint32_t role;
    datum_index new_object;
    object_header *hdr;
    uint8_t header_flags;
    GlobalsMultiplayerInformation *mp_info =
        (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;

    flag_tag = (datum_index)(uint32_t)((mp_info->flag.tag_id.id << 16) | mp_info->flag.tag_id.index);

    object_placement_data_initialize(&placement, flag_tag, (datum_index)0xffffffff);
    placement.position = *position;
    placement.name_index = (int16_t)name_index;

    role = 3;
    if (network_game_mode == 2) {
        int16_t object_type = *(int16_t *)tag_instances[(uint32_t)placement.definition_tag & 0xffff].data;
        if (*(int32_t *)((uint8_t *)object_type_definitions[object_type] + 0x10) != -1) {
            role = 0;
        }
    }

    new_object = object_new_with_datum_role_control(&placement, role);

    hdr = (object_header *)object_headers->data + ((uint32_t)new_object & 0xffff);
    header_flags = hdr->flags;
    hdr->flags = header_flags & ~_object_header_in_pvs_pass_bit;
    if ((header_flags & _object_header_active_bit) == 0) {
        object_mark_pending_delete((uint32_t)new_object);
    }

    return new_object;
}

#if 0
Original Ghidra decompilation (0x468360), from tools/pack.py 0x468360:

uint FUN_00468360(undefined2 param_1)

{
  int iVar1;
  byte bVar2;
  undefined4 *in_EAX;
  undefined4 uVar3;
  uint uVar4;
  uint local_88 [5];
  undefined2 local_74;
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;

  FUN_004f53a0(*(undefined4 *)(*(int *)(DAT_00746fa0 + 0x168) + 0xc),0xffffffff);
  local_70 = *in_EAX;
  local_6c = in_EAX[1];
  local_68 = in_EAX[2];
  local_74 = param_1;
  uVar3 = 3;
  if ((DAT_00719720 == 2) &&
     (*(int *)((&PTR_PTR_0069bfdc)
               [**(short **)((local_88[0] & 0xffff) * 0x20 + 0x14 + DAT_0087bc14)] + 0x10) != -1)) {
    uVar3 = 0;
  }
  uVar4 = object_new_with_datum_role_control(local_88,uVar3);
  iVar1 = *(int *)(DAT_008603b0 + 0x34) + (uVar4 & 0xffff) * 0xc;
  bVar2 = *(byte *)(iVar1 + 2);
  *(byte *)(iVar1 + 2) = bVar2 & 0xbf;
  if ((bVar2 & 1) == 0) {
    FUN_004f50f0();
  }
  return uVar4;
}
#endif
