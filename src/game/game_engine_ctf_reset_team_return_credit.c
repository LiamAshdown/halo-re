// game_engine_ctf_reset_team_return_credit  (Ghidra: FUN_00468840; named per its summary)
// address 0x468840, size 101 bytes
// name confidence: 0.45   rewrite confidence: 0.4
// evidence: out/phase4/game_functions.md ("Resets a team's flag-return credit tracking and, if
//   a flag object exists for that team, clears its carrier state and updates its flag bits");
//   types/objects.h object.flags (_object_changed_bit 0x04000000); ctf_flag_object_clear_carrier
//   (0x4666c0, already committed) which itself takes EBX -> flag_object_index, EDI -> position
//   and is called here with zero visible arguments, so both must be pass-through from this
//   function's own (unrecovered) caller, matching the forwarded-parameter precedent already used
//   for game_engine_ctf_respawn_team_flag.c (this batch). object + 0xb8 is read as a team index
//   here (the objects.h name_index / team_index conflict PLAN.md flags); kept literal.
// register convention: object handle in EAX; flag_object_index/position forwarded straight
//   through to ctf_flag_object_clear_carrier without being read or written here.
//   // blam-cc: EAX -> object_index, EBX -> forwarded_flag_object_index, EDI ->
//   //   forwarded_position
// UNSURE: object + 0x22c here is equipment_data's own unresolved first dword (bit 0x40 cleared);
//   object + 0xb8 read as "team" rather than object.name_index (see evidence above).

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "game.h"

extern data_array *object_headers; // 0x008603b0

extern uint8_t ctf_team_return_credit_active[2]; // 0x006b0ea4
extern int32_t ctf_team_return_credit_ticks[2];  // 0x006b0ea8
extern real_point3d *ctf_team_flag_stand_position[2]; // 0x006b0e88

extern void ctf_flag_object_clear_carrier(datum_index flag_object_index, real_point3d *position); // 0x4666c0

// blam-cc: EAX -> object_index, EBX -> forwarded_flag_object_index, EDI -> forwarded_position
// Reads the object's team (object + 0xb8, see UNSURE above), clears that team's flag-return
// credit tracking, and, if that team currently has a live flag object, clears its carrier state
// (forwarding flag_object_index/position through to ctf_flag_object_clear_carrier), clears an
// equipment-runtime bit, and marks the flag object changed for network sync.
void game_engine_ctf_reset_team_return_credit(uint32_t object_index,
    datum_index forwarded_flag_object_index, real_point3d *forwarded_position)
{
    object *obj = ((object_header *)object_headers->data)[object_index & 0xffff].data;
    int16_t team = *(int16_t *)((uint8_t *)obj + 0xb8); // UNSURE: name_index/team_index conflict

    ctf_team_return_credit_active[team] = 0;
    ctf_team_return_credit_ticks[team] = 0;

    if (ctf_team_flag_stand_position[team] != (real_point3d *)0) {
        uint32_t *unknown_22c = (uint32_t *)((uint8_t *)obj + 0x22c); // UNSURE: equipment_data+0x00

        ctf_flag_object_clear_carrier(forwarded_flag_object_index, forwarded_position);
        *unknown_22c &= 0xffffffbf;
        obj->flags |= _object_changed_bit;
    }
}

#if 0
Original Ghidra decompilation (0x468840), from tools/pack.py 0x468840:

void FUN_00468840(void)

{
  short sVar1;
  int iVar2;
  uint in_EAX;

  iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc);
  sVar1 = *(short *)(iVar2 + 0xb8);
  (&DAT_006b0ea4)[sVar1] = 0;
  (&DAT_006b0ea8)[sVar1] = 0;
  if ((&DAT_006b0e88)[*(short *)(iVar2 + 0xb8)] != 0) {
    ctf_flag_object_clear_carrier();
    *(uint *)(iVar2 + 0x22c) = *(uint *)(iVar2 + 0x22c) & 0xffffffbf;
    *(uint *)(iVar2 + 0x10) = *(uint *)(iVar2 + 0x10) | 0x4000000;
  }
  return;
}
#endif
