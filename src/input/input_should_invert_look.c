// input_should_invert_look  (Ghidra: FUN_0048fd60)
// address 0x48fd60, size 242 bytes
// name confidence: 0.5   rewrite confidence: 0.5
// evidence: out/phase4/input_types_notes.md names this "input_should_invert_look (0x48fd60)
// reads player / object / unit-definition data owned by game.h / objects.h / tags.h" and
// documents the result: "returns true for a driver seat (seat flag bit 2) when the unit
// definition short at +0x2f4 is 3 or 5. The meaning of that short is not pinned." Every field
// below matches an existing header exactly: types/game.h player_globals::local_players (+0x04)
// and the players data_array; the local_players/player_data validity check is a verbatim inline
// of datum_get (src/memory/datum_get.c); types/objects.h object::parent_object (+0x11c),
// object::definition_tag (+0x000), object_header::data (+0x08) and the object_data data_array;
// types/units.h k_unit_data_offset and unit_data::vehicle_seat_index (+0x2f0); types/tags.h
// UnitSeat::flags bit 2 ("driver") and Unit::seats (TagReflexive at +0x2e4, so .pointer is at
// +0x2e8, stride 0x11c).
// UNSURE (new since input_types_notes.md): offset +0x2f4 on the parent's tag data is exactly
// Vehicle::vehicle_type (Unit base ends at 0x2f0, vehicle_flags is 0x2f0..0x2f4, vehicle_type is
// the int16 right after), and 3 / 5 are vehicletype_human_plane / vehicletype_alien_fighter --
// i.e. this looks like "invert pitch while driving an aircraft". That assumes the parent unit's
// tag is always a Vehicle, which is not verified against a case where it might be a Biped.
// phase-4 review fix: the object handle passed to object_try_and_get is the player unit
//   ([player + 0x34] in ECX, 0x48fdcf); the first rewrite had dropped it.
// register convention: local_player_index in AX (in_AX)

#include "tags.h"
#include "memory.h"
#include "cache.h"
#include "math.h"
#include "game.h"
#include "networking.h"
#include "interface.h"
#include "saved_games.h"
#include "input.h"

#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern player_globals *local_player_globals; // 0x0087a478
extern data_array *player_data;              // 0x0087a480
extern data_array *object_data;              // 0x008603b0
extern tag_instance *tag_instances;          // 0x0087bc14

extern void *datum_get(datum_index handle, data_array *array); // memory module, 0x4d0680
extern object *object_try_and_get(datum_index object_index, uint32_t type_mask); // 0x4f6ec0, objects module, blam-cc: ECX -> object_index

// blam-cc: local_player_index in EAX
// True when local_player_index's controlled unit is seated (as a driver) in a parent vehicle
// whose type is vehicletype_human_plane or vehicletype_alien_fighter.
uint8_t input_should_invert_look(int16_t local_player_index)
{
    datum_index player_handle;
    void *player_record;
    object *unit_object;
    unit_data *unit;
    object_header *parent_header;
    object *parent_object;
    Vehicle *parent_tag;
    UnitSeat *seat;

    if (local_player_index == -1 || local_player_index >= 1) {
        return 0;
    }
    player_handle = local_player_globals->local_players[local_player_index];
    if (player_handle == (datum_index)0xffffffff) {
        return 0;
    }
    player_record = datum_get(player_handle, player_data);
    if (player_record == (void *)0) {
        return 0;
    }

    // 0x48fdcf: mov ecx,[eax+0x34] (player->unit) ; push 3 ; call object_try_and_get
    unit_object = object_try_and_get(((player *)player_record)->unit, 3);
    if (unit_object == (object *)0) {
        return 0;
    }
    if (unit_object->parent_object == (datum_index)0xffffffff) {
        return 0;
    }
    unit = (unit_data *)((uint8_t *)unit_object + k_unit_data_offset);
    if (unit->vehicle_seat_index == -1) {
        return 0;
    }

    parent_header = &((object_header *)object_data->data)[(uint16_t)unit_object->parent_object];
    parent_object = parent_header->data;
    parent_tag = (Vehicle *)tag_instances[(uint16_t)parent_object->definition_tag].data;

    if (parent_tag->vehicle_type == 3 || parent_tag->vehicle_type == 5) {
        seat = (UnitSeat *)((uint8_t *)parent_tag->base.seats.pointer +
                             (uint32_t)unit->vehicle_seat_index * 0x11c);
        if ((seat->flags & 4) != 0) {
            return 1;
        }
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x48fd60):

undefined1 FUN_0048fd60(void)

{
  int iVar1;
  short in_AX;
  int iVar2;
  short sVar3;
  undefined1 uVar4;
  short sVar6;
  undefined1 uVar5;

  uVar5 = 0;
  uVar4 = 0;
  if (((((((in_AX != -1) && (in_AX < 1)) &&
         (iVar2 = *(int *)(DAT_0087a478 + 4 + in_AX * 4), iVar2 != -1)) &&
        ((sVar3 = (short)iVar2, uVar4 = uVar5, -1 < sVar3 &&
         (sVar3 < *(short *)(DAT_0087a480 + 0x20))))) &&
       ((sVar3 = *(short *)((int)*(short *)(DAT_0087a480 + 0x22) * (int)sVar3 +
                           *(int *)(DAT_0087a480 + 0x34)), sVar3 != 0 &&
        ((sVar6 = (short)((uint)iVar2 >> 0x10), sVar6 == 0 || (sVar3 == sVar6)))))) &&
      (iVar2 = object_try_and_get(3), iVar2 != 0)) &&
     ((*(uint *)(iVar2 + 0x11c) != 0xffffffff && (*(short *)(iVar2 + 0x2f0) != -1)))) {
    iVar1 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                 (*(uint *)(iVar2 + 0x11c) & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14
                    + DAT_0087bc14);
    sVar3 = *(short *)(iVar1 + 0x2f4);
    if (((sVar3 == 3) || (sVar3 == 5)) &&
       ((*(byte *)(*(short *)(iVar2 + 0x2f0) * 0x11c + *(int *)(iVar1 + 0x2e8)) & 4) != 0)) {
      uVar4 = 1;
    }
  }
  return uVar4;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
