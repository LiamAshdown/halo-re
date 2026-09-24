// camera_observer_target_is_valid  (Ghidra: FUN_00459dd0; renamed per symbols/review_queue.txt)
// address 0x459dd0, size 168 bytes
// name confidence: 0.4   rewrite confidence: 0.75
// evidence: called by camera_observer_find_best_target (0x459a00) as the final acceptance filter
//   on a sorted candidate list; types/objects.h object (parent_object 0x11c).
//
// NOT a transcription of Ghidra's C. Ghidra drops ECX, EDI and the one stack parameter of this
// function entirely, which loses the root-object tracking and four of the five arguments of the
// 0x505880 call. This rewrite is taken from
//   objdump -d -M intel --start-address=0x459dd0 --stop-address=0x459e74 bin/halo.exe
// which shows, unambiguously:
//   or   ecx,0xffffffff          ; root = -1
//   ...  mov ecx,eax             ; root = current
//        mov eax,[obj+0x11c]     ; current = current->parent_object
//        cmp eax,-1 / jne        ; so ECX ends up holding the LAST non-(-1) handle, i.e. the
//                                ; chain root -- Ghidra's decompile leaves it as -1 forever.
//   push edx(scratch) / push ecx(root) / push eax(&delta) / push esi(origin) / push 0xc2ad
//                                ; five arguments, matching the collision_test_movement_segment prototype already
//                                ; established in src/objects and src/projectiles.
//   fld [edi] / fsub [esi] ...   ; delta = *target_position - *observer_position, three floats
//   cmp word [esp+0x14],3        ; the int16 at scratch+0x00 is the result code
//   mov ecx,[esp+0x4c] -> 0x4f6fb0 ; scratch+0x38 (the BLOCKING object) is the first root lookup
//   mov ecx,[esp+0x68] -> 0x4f6fb0 ; the stack parameter (the CANDIDATE object) is the second
// register convention: EAX, ECX and EDI are all live on entry (EDI is inherited from the caller,
//   which is why Ghidra reports no parameters at all); the candidate handle is the single stack
//   parameter.
//   // blam-cc: EAX -> exclude_object, ECX -> observer_position, EDI -> target_position,
//   //          stack -> target_object
//
// UNSURE: the 0x50+-byte scratch block collision_test_movement_segment fills is not typed anywhere in this repo;
// only its +0x00 int16 result code and its +0x38 object handle are read here, so it is declared
// as an opaque byte array with those two accesses spelled out.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"

extern data_array *object_data; // 0x008603b0

extern uint8_t collision_test_movement_segment(uint32_t mask, real_point3d *origin, real_vector3d *delta,
                             uint32_t exclude_object, void *scratch); // 0x505880
extern datum_index object_get_root_object_index(datum_index object_index); // 0x4f6fb0, objects module

// Line-of-sight test for one observer candidate. Walks `exclude_object` (the observer's own
// unit) up its parent chain to the root, then casts from `observer_position` to
// `target_position` ignoring that root. An unobstructed cast accepts the candidate; an
// obstructed one is still accepted when the blocker and the candidate share a root object
// (result code 3, i.e. the candidate occluded itself or its own vehicle).
char camera_observer_target_is_valid(datum_index exclude_object, real_point3d *observer_position,
                                     real_point3d *target_position, datum_index target_object)
    // blam-cc: EAX -> exclude_object, ECX -> observer_position, EDI -> target_position,
    //          stack -> target_object
{
    datum_index root;
    datum_index current;
    real_vector3d delta;
    uint8_t scratch[0x50];

    root = k_datum_index_none;
    current = exclude_object;
    if (current != k_datum_index_none) {
        do {
            root = current;
            current = ((object_header *)object_data->data)[current & 0xffff].data->parent_object;
        } while (current != k_datum_index_none);
    }

    delta.i = target_position->x - observer_position->x;
    delta.j = target_position->y - observer_position->y;
    delta.k = target_position->z - observer_position->z;

    if (collision_test_movement_segment(0xc2ad, observer_position, &delta, root, scratch) == 0) {
        return 1;
    }
    if (*(int16_t *)scratch != 3) {
        return 0;
    }
    if (object_get_root_object_index(*(datum_index *)(scratch + 0x38)) ==
        object_get_root_object_index(target_object)) {
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x459dd0), from tools/pack.py 0x459dd0:

undefined4 FUN_00459dd0(void)

{
  char cVar1;
  uint in_EAX;
  int iVar2;
  int iVar3;
  short local_50;

  if (in_EAX != 0xffffffff) {
    do {
      in_EAX = *(uint *)(*(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EAX & 0xffff) * 0xc) +
                        0x11c);
    } while (in_EAX != 0xffffffff);
  }
  cVar1 = FUN_00505880(0xc2ad);
  if (cVar1 != '\0') {
    if (local_50 == 3) {
      iVar2 = object_get_root_object_index();
      iVar3 = object_get_root_object_index();
      if (iVar2 == iVar3) {
        return 1;
      }
    }
    return 0;
  }
  return 1;
}
#endif
