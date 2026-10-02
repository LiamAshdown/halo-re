// unit_track_target_lock_timeout  (Ghidra: unit_track_target_lock_timeout, renamed)
// address 0x55ec90, size 94 bytes
// name confidence: 0.3   rewrite confidence: 0.55
// evidence: biped_data.flags bit 0 "grounded" and unknown_508 (types/units.h),
//   biped_data.unknown_504 ("ticks without a target lock (0x55ec90)" -- already attributed to
//   this function by name), unit_data.control_flags bit 2 (_unit_control_flag_jump).
// register convention: object index in EDX (in_EDX).
//   // blam-cc: EDX -> object_index
// FIXED (register inputs, objdump): EDX carries object_index (read at 0x55ec99, mov eax,edx);
// this file had no "// blam-cc:" note at all, so the checker saw no register mapping.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "units.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *object_data; // 0x008603b0

extern uint32_t unit_snap_to_min_ground_height(uint32_t object_index); // 0x55ecf0, this batch

// While a biped is airborne and has no resolved target-lock comparison, counts ticks (saturating
// at 0x7f) since the last target lock; once past 5 ticks while the jump control is held, snaps
// it to its minimum ground height.
void unit_track_target_lock_timeout(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    unit_data *unit = (unit_data *)((uint8_t *)obj + k_unit_data_offset);
    biped_data *biped = (biped_data *)((uint8_t *)obj + k_unit_object_size);

    if ((biped->flags & 1) == 0 && biped->landing_type != 1) {
        if ((int8_t)biped->jump_ticks < 0x7f) {
            biped->jump_ticks = biped->jump_ticks + 1;
        }
        if ((unit->control_flags & 2) != 0 && (int8_t)biped->jump_ticks > 5) {
            unit_snap_to_min_ground_height(object_index);
        }
    }
}

#if 0
Original Ghidra decompilation (0x55ec90):

void FUN_0055ec90(void)

{
  int iVar1;
  uint in_EDX;

  iVar1 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (in_EDX & 0xffff) * 0xc);
  if (((*(byte *)(iVar1 + 0x4cc) & 1) == 0) && (*(short *)(iVar1 + 0x508) != 1)) {
    if (*(char *)(iVar1 + 0x504) < '\x7f') {
      *(char *)(iVar1 + 0x504) = *(char *)(iVar1 + 0x504) + '\x01';
    }
    if (((*(byte *)(iVar1 + 0x208) & 2) != 0) && ('\x05' < *(char *)(iVar1 + 0x504))) {
      FUN_0055ecf0();
    }
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
