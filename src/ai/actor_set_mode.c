// actor_set_mode  (Ghidra: actor_set_mode, already named)
// address 0x40d8d0, size 236 bytes
// name confidence: 0.5   rewrite confidence: 0.7
// evidence: types/ai.h actor_mode_definition (exit_proc +0x18, combat_grade +0x04,
//   data_size +0x00, enter_proc +0x08, table at 0x00655254) and actor.mode_data.raw /
//   recognition[4] / awareness_level, all cited as established by this function.
// register convention: already a full C signature in the decompilation
//   (actor_index, mode, mode_data pointer); no unresolved registers.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern data_array *actor_data;                            // 0x00880360
extern actor_mode_definition actor_mode_definitions[16];  // 0x00655254

// Central actor mode setter: runs the outgoing mode's exit callback, clamps or raises
// awareness_level depending on the new mode's combat_grade, clears the recognition-history
// ring, copies up to data_size bytes of mode-specific data into actor.mode_data.raw, commits
// the new mode number and mode_changed flag, then runs the new mode's enter callback.
void actor_set_mode(datum_index actor_index, int32_t mode, void *mode_data)
{
    actor *self;
    uint32_t data_size;
    int i;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (actor_mode_definitions[self->mode].exit_proc != 0) {
        ((void (*)(datum_index))actor_mode_definitions[self->mode].exit_proc)(actor_index);
    }

    if (actor_mode_definitions[mode].combat_grade == 0) {
        if (2 < self->awareness_level) {
            self->awareness_level = 2;
        }
    } else if (self->awareness_level < 3) {
        self->awareness_level = 3;
    }

    self->recognition_cursor = 0;
    for (i = 0; i < 4; i++) {
        self->recognition[i].firing_position_index = -1;
    }
    if (self->recognition_valid != 0) {
        self->recognition_valid = 0;
    }

    data_size = actor_mode_definitions[mode].data_size;
    if (data_size != 0 && mode_data != 0) {
        uint8_t *src = (uint8_t *)mode_data;
        uint8_t *dst = self->mode_data.raw;
        uint32_t n;
        for (n = data_size >> 2; n != 0; n--) {
            *(uint32_t *)dst = *(uint32_t *)src;
            src += 4;
            dst += 4;
        }
        for (n = data_size & 3; n != 0; n--) {
            *dst = *src;
            src++;
            dst++;
        }
    }

    self->mode = (int16_t)mode;
    self->mode_changed = 1;

    if (actor_mode_definitions[mode].enter_proc != 0) {
        ((void (*)(datum_index))actor_mode_definitions[mode].enter_proc)(actor_index);
    }
}

#if 0
Original Ghidra decompilation (0x40d8d0):

void actor_set_mode(uint param_1,int param_2,undefined4 *param_3)

{
  undefined2 *puVar1;
  uint uVar2;
  uint uVar3;
  int iVar4;
  int iVar5;
  int iVar6;
  undefined4 *puVar7;

  iVar6 = (param_1 & 0xffff) * 0x724;
  iVar5 = *(int *)(DAT_00880360 + 0x34) + iVar6;
  if (*(code **)(&DAT_0065526c + *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iVar6) * 0x38) !=
      (code *)0x0) {
    (**(code **)(&DAT_0065526c + *(short *)(*(int *)(DAT_00880360 + 0x34) + 0x6c + iVar6) * 0x38))
              (param_1);
  }
  if (*(short *)(&DAT_00655258 + param_2 * 0x38) == 0) {
    if (2 < *(short *)(iVar5 + 0x6a)) {
      *(undefined2 *)(iVar5 + 0x6a) = 2;
    }
  }
  else if (*(short *)(iVar5 + 0x6a) < 3) {
    *(undefined2 *)(iVar5 + 0x6a) = 3;
  }
  iVar6 = *(int *)(DAT_00880360 + 0x34) + iVar6;
  *(undefined2 *)(iVar6 + 0x3c6) = 0;
  puVar1 = (undefined2 *)(iVar6 + 0x3ca);
  iVar4 = 4;
  do {
    *puVar1 = 0xffff;
    puVar1 = puVar1 + 2;
    iVar4 = iVar4 + -1;
  } while (iVar4 != 0);
  if (*(char *)(iVar6 + 0x3d8) != '\0') {
    *(undefined1 *)(iVar6 + 0x3d8) = 0;
  }
  uVar3 = *(uint *)(&DAT_00655254 + param_2 * 0x38);
  if ((uVar3 != 0) && (param_3 != (undefined4 *)0x0)) {
    puVar7 = (undefined4 *)(iVar5 + 0x9c);
    for (uVar2 = uVar3 >> 2; uVar2 != 0; uVar2 = uVar2 - 1) {
      *puVar7 = *param_3;
      param_3 = param_3 + 1;
      puVar7 = puVar7 + 1;
    }
    for (uVar3 = uVar3 & 3; uVar3 != 0; uVar3 = uVar3 - 1) {
      *(undefined1 *)puVar7 = *(undefined1 *)param_3;
      param_3 = (undefined4 *)((int)param_3 + 1);
      puVar7 = (undefined4 *)((int)puVar7 + 1);
    }
  }
  *(short *)(iVar5 + 0x6c) = (short)param_2;
  *(undefined1 *)(iVar5 + 0x70) = 1;
  if (*(code **)(&DAT_0065525c + (short)param_2 * 0x38) != (code *)0x0) {
    (**(code **)(&DAT_0065525c + (short)param_2 * 0x38))(param_1);
  }
  return;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
