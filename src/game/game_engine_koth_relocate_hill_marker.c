// game_engine_koth_relocate_hill_marker  (Ghidra: FUN_0046bfe0; named per its summary)
// address 0x46bfe0, size 149 bytes
// name confidence: 0.45   rewrite confidence: 0.3
// evidence: out/phase4/game_functions.md ("Relocates the moving King-of-the-Hill marker:
//   detaches the current hill object, picks a new valid random position, and (re)spawns the
//   marker there"); types/tags.h GlobalsMultiplayerInformation::ball (TagDependency at +0x4c,
//   tag_id at +0x58); game_engine_variant::unknown_8c aliased 0x006f1d14 (gate: only runs
//   outside game_engine_index 1/2, i.e. not CTF/Slayer); object_placement_data_initialize /
//   object_mark_pending_delete already committed; game_engine_koth_find_marker_position (0x46beb0,
//   this batch).
// register convention: no parameters.
// UNSURE: object_new's exact signature -- modeled here taking the placement block by pointer
//   like its sibling object_new_with_datum_role_control, since Ghidra shows it called with no
//   visible arguments right after object_placement_data_initialize fills the same local block.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "game.h"

extern Globals *global_globals;      // 0x00746fa0
extern data_array *object_headers;   // 0x008603b0
extern game_variant game_engine_variant; // 0x006f1c88 (unknown_8c aliased 0x006f1d14)

extern void object_placement_data_initialize(object_placement_data *placement,
    datum_index definition_tag, datum_index role); // 0x4f53a0
extern datum_index object_new(object_placement_data *placement); // 0x4f5460, UNSURE signature
extern void object_mark_pending_delete(uint32_t object_index); // 0x4f50f0
extern void game_engine_koth_find_marker_position(real_point3d *out_position, int16_t type_filter); // 0x46beb0, this batch

// Outside game_engine_index 1/2, if the map has a multiplayer "ball" tag, builds a placement
// block for it, finds a new type-1 marker position, spawns the marker object there, and clears
// its header's in-PVS-pass bit (marking it pending-delete-eligible if not already active).
// FIXED 2026-09-28: ESI is the ball index -- 0x46c01a stores it as the placement owner team and 0x46c018 passes
//   it as 0x46beb0's type filter (ECX); every caller loads ESI with its loop index.
// blam-cc: ESI -> ball_index
void game_engine_koth_relocate_hill_marker(int32_t ball_index)
{
    if (game_engine_variant.unknown_8c < 1 || game_engine_variant.unknown_8c > 2) {
        GlobalsMultiplayerInformation *mp_info =
            (GlobalsMultiplayerInformation *)global_globals->multiplayer_information.pointer;
        uint32_t ball_tag = (uint32_t)((mp_info->ball.tag_id.id << 16) | mp_info->ball.tag_id.index);

        if ((int32_t)ball_tag != -1) {
            object_placement_data placement;
            datum_index new_object;
            object_header *hdr;
            uint8_t header_flags;

            object_placement_data_initialize(&placement, ball_tag, (datum_index)0xffffffff);
            placement.owner_team = (int16_t)ball_index;
            game_engine_koth_find_marker_position(&placement.position, (int16_t)ball_index); // was 1 (UNSURE); fixed
                // is register-passed (CX) at the real call site and not visible here; guessed

            new_object = object_new(&placement);

            hdr = (object_header *)object_headers->data + ((uint32_t)new_object & 0xffff);
            header_flags = hdr->flags;
            hdr->flags = header_flags & ~_object_header_in_pvs_pass_bit;
            if ((header_flags & _object_header_active_bit) == 0) {
                object_mark_pending_delete((uint32_t)new_object);
            }
        }
    }
}

#if 0
Original Ghidra decompilation (0x46bfe0), from tools/pack.py 0x46bfe0:

void FUN_0046bfe0(void)

{
  byte bVar1;
  int iVar2;
  undefined4 *puVar3;
  uint uVar4;
  undefined1 local_94 [36];
  undefined4 local_70;
  undefined4 local_6c;
  undefined4 local_68;

  if (((DAT_006f1d14 < 1) || (2 < DAT_006f1d14)) &&
     (iVar2 = *(int *)(*(int *)(DAT_00746fa0 + 0x168) + 0x58), iVar2 != -1)) {
    object_placement_data_initialize(iVar2,0xffffffff);
    puVar3 = (undefined4 *)FUN_0046beb0(local_94);
    local_70 = *puVar3;
    local_6c = puVar3[1];
    local_68 = puVar3[2];
    uVar4 = object_new();
    iVar2 = *(int *)(DAT_008603b0 + 0x34) + (uVar4 & 0xffff) * 0xc;
    bVar1 = *(byte *)(iVar2 + 2);
    *(byte *)(iVar2 + 2) = bVar1 & 0xbf;
    if ((bVar1 & 1) == 0) {
      object_mark_pending_delete();
    }
  }
  return;
}
#endif
