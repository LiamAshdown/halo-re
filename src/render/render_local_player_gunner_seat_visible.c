// render_local_player_gunner_seat_visible  (Ghidra: FUN_0050fcd0; new name, evidence below.
// Disagrees with the phase4 one-liner and out/phase4/render_types_notes.md's guess of a "media
// cluster marker" test -- see UNSURE note.)
// address 0x50fcd0, size 184 bytes (Ghidra's own boundary at 108 bytes is wrong: the real ret is
// at 0x50fd88, and the mid-function `cmp ecx,0xffffffff` at 0x50fd3c that Ghidra also lists as
// the separate, bogus function render_window_call_hook_weather_particle_systems_render is just
// this function's third early-out; see out/phase4/render_types_notes.md)
// name confidence: 0.4   rewrite confidence: 0.55
// evidence (objdump, since Ghidra's own decompile of this address range is truncated to the
//   first branch): the caller at 0x453bb1 passes render_local_player_index (0x007c3108) and
//   gates a contrail draw on the boolean result (0x453b99 tests Object/Contrail flags
//   0x10000/0x20000 first). Field chain confirmed against the headers: player_globals
//   (types/game.h) local_players[1] at +0x04 indexed by local_player_index (bound checked to
//   exactly slot 0, matching the header's "< 1" note); player.unit at +0x34; object.parent_object
//   at +0x11c (types/objects.h); the *unit's own* vehicle_seat_index at +0x2f0 (types/units.h);
//   the parent's tag data Unit.seats TagReflexive at +0x2e4/+0x2e8 (types/tags.h, stride
//   sizeof(UnitSeat) = 0x11c); and UnitSeatFlags bit 0x8 ("gunner", types/tags.h enum order).
// register convention: EAX = local_player_index (in_AX read as int16), no other arguments.
//   // blam-cc: EAX -> local_player_index
// UNSURE: camera_get_type_for_player's return value of 0 is assumed to mean "normal" (the
//   function short-circuits to true for it and only walks the seat chain for a non zero,
//   presumably cinematic/device, camera type). The phase4 pass's "media cluster marker" summary
//   does not match any field this function actually touches; renamed and re-evidenced from
//   scratch. Also UNSURE: this reads the departing unit's own vehicle_seat_index, not the
//   parent's, which only makes sense if a unit's seat index is kept valid after it dismounts;
//   preserved as decompiled.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "objects.h"
#include "units.h"
#include "cache.h"

extern int16_t camera_get_type_for_player(int16_t local_player_index); // 0x445ac0
extern player_globals *local_player_globals;                        // 0x0087a478
extern data_array *player_data;                                     // 0x0087a480 "players"
extern data_array *object_data;                                  // 0x008603b0
extern tag_instance *tag_instances;                                 // 0x0087bc14

// Returns true unconditionally for a normal camera (camera_get_type_for_player == 0). For any
// other camera type, returns true only if the given local player has a unit, that unit's parent
// object's Unit/Vehicle tag has a seat matching the unit's own vehicle_seat_index, and that seat
// is a gunner seat.
int16_t render_local_player_gunner_seat_visible(int16_t local_player_index)
{
    datum_index player_index;
    object_header *unit_header;
    object *unit;
    unit_data *unit_ext;
    datum_index parent_index;
    object_header *parent_header;
    object *parent;
    tag_instance *vehicle_tag;
    Unit *vehicle;
    UnitSeat *seats;
    int16_t seat_index;

    if (camera_get_type_for_player(local_player_index) == 0) {
        return 1;
    }

    if (local_player_index == -1 || local_player_index > 0) {
        player_index = k_datum_index_none;
    } else {
        player_index = local_player_globals->local_players[local_player_index];
    }

    player_index = ((player *)player_data->data)[(uint16_t)player_index].unit;
    if (player_index == k_datum_index_none) {
        return 0;
    }

    unit_header = &((object_header *)object_data->data)[(uint16_t)player_index];
    unit = unit_header->data;
    parent_index = unit->parent_object;
    if (parent_index == k_datum_index_none) {
        return 0;
    }
    unit_ext = (unit_data *)((uint8_t *)unit + k_unit_data_offset);
    seat_index = unit_ext->vehicle_seat_index;
    if (seat_index == -1) {
        return 0;
    }

    parent_header = &((object_header *)object_data->data)[(uint16_t)parent_index];
    parent = parent_header->data;
    vehicle_tag = &tag_instances[(uint16_t)parent->definition_tag];
    vehicle = (Unit *)vehicle_tag->data;
    seats = (UnitSeat *)vehicle->seats.pointer;

    return (seats[seat_index].flags & 8) != 0;
}

#if 0
Original Ghidra decompilation (0x50fcd0), truncated by Ghidra at its own (incorrect) 108 byte
function boundary; the real function continues to the ret at 0x50fd88 (see
out/phase4/render_types_notes.md and the FUN_0050fcd0 / render_window_call_hook_... entries):

undefined4 FUN_0050fcd0(void)

{
  short sVar1;
  int iVar2;
  char cVar3;
  short in_AX;
  undefined4 uVar4;
  undefined3 uVar5;
  uint uVar6;

  uVar4 = camera_get_type_for_player();
  uVar5 = (undefined3)(CONCAT22((short)((uint)uVar4 >> 0x10),-(short)uVar4) >> 8);
  cVar3 = '\x01' - ((short)uVar4 != 0);
  uVar4 = CONCAT31(uVar5,cVar3);
  if (cVar3 == '\0') {
    if ((in_AX == -1) || (0 < in_AX)) {
      uVar6 = 0xffffffff;
    }
    else {
      uVar6 = *(uint *)(DAT_0087a478 + 4 + in_AX * 4);
    }
    uVar6 = *(uint *)((uVar6 & 0xffff) * 0x200 + 0x34 + *(int *)(DAT_0087a480 + 0x34));
    if (uVar6 != 0xffffffff) {
      iVar2 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar6 & 0xffff) * 0xc);
      uVar6 = *(uint *)(iVar2 + 0x11c);
      if (((uVar6 != 0xffffffff) && (sVar1 = *(short *)(iVar2 + 0x2f0), sVar1 != -1)) &&
         ((*(byte *)(sVar1 * 0x11c +
                    *(int *)(*(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 +
                                                  (uVar6 & 0xffff) * 0xc) & 0xffff) * 0x20 + 0x14 +
                                     DAT_0087bc14) + 0x2e8)) & 8) != 0)) {
        uVar4 = CONCAT31(uVar5,1);
      }
    }
  }
  return uVar4;
}

Confirmed against objdump 0x50fcd0..0x50fd88 (the true extent of this function):

0050fcd0 <.text+0x10ecd0>:
  50fcd0: push   %esi
  50fcd1: mov    %eax,%esi
  50fcd3: mov    %esi,%ecx
  50fcd5: call   0x445ac0
  50fcda: neg    %ax
  50fcdd: sbb    %al,%al
  50fcdf: inc    %al
  50fce1: jne    0x50fd87
  50fce7: cmp    $0xffff,%si
  50fceb: je     0x50fd02
  50fced: cmp    $0x1,%si
  50fcf1: jge    0x50fd02
  50fcf3: mov    0x87a478,%edx
  50fcf9: movswl %si,%ecx
  50fcfc: mov    0x4(%edx,%ecx,4),%ecx
  50fd00: jmp    0x50fd05
  50fd02: or     $0xffffffff,%ecx
  50fd05: mov    0x87a480,%edx
  50fd0b: mov    0x34(%edx),%edx
  50fd0e: and    $0xffff,%ecx
  50fd14: shl    $0x9,%ecx
  50fd17: mov    0x34(%ecx,%edx,1),%ecx
  50fd1b: cmp    $0xffffffff,%ecx
  50fd1e: je     0x50fd87
  50fd20: mov    0x8603b0,%edx
  50fd26: mov    0x34(%edx),%edx
  50fd29: and    $0xffff,%ecx
  50fd2f: lea    (%ecx,%ecx,2),%ecx
  50fd32: mov    0x8(%edx,%ecx,4),%esi
  50fd36: mov    0x11c(%esi),%ecx
  50fd3c: cmp    $0xffffffff,%ecx
  50fd3f: je     0x50fd87
  50fd41: mov    0x2f0(%esi),%si
  50fd48: cmp    $0xffff,%si
  50fd4c: je     0x50fd87
  50fd4e: and    $0xffff,%ecx
  50fd54: lea    (%ecx,%ecx,2),%ecx
  50fd57: mov    0x8(%edx,%ecx,4),%edx
  50fd5b: mov    (%edx),%ecx
  50fd5d: mov    0x87bc14,%edx
  50fd63: and    $0xffff,%ecx
  50fd69: shl    $0x5,%ecx
  50fd6c: mov    0x14(%ecx,%edx,1),%ecx
  50fd70: mov    0x2e8(%ecx),%ecx
  50fd76: movswl %si,%edx
  50fd79: imul   $0x11c,%edx,%edx
  50fd7f: testb  $0x8,(%edx,%ecx,1)
  50fd83: je     0x50fd87
  50fd85: mov    $0x1,%al
  50fd87: pop    %esi
  50fd88: ret
#endif
