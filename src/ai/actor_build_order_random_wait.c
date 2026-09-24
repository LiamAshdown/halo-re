// actor_build_order_random_wait  (Ghidra: actor_build_order_random_wait, renamed)
// address 0x409a90, size 153 bytes
// name confidence: 0.4   rewrite confidence: 0.4
// evidence: types/ai.h actor.order_committed (0x160)/unknown_1cc; types/game.h
//   game_time_globals.game_time; phase-4 summary "builds a wait order with a randomized
//   duration between roughly 300 and 600 ticks".
// register convention: actor index in EAX, order pointer in ECX, a caller byte in the
//   recognized stack parameter.
//   // blam-cc: EAX -> actor_index, ECX -> order, stack -> byte_a
// TYPES-GAP: this order record's tail (0x18 bytes: byte offsets 1/2/3, short at 0xc, dword
//   at 8, short at 0x10) does not match actor_order; kept as raw offsets.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "game.h"

extern data_array *actor_data;       // 0x00880360
extern game_time_globals *game_time; // 0x006f1d6c
extern uint32_t random_seed_global;  // 0x00719cd0

int32_t actor_build_order_random_wait(uint32_t actor_index, uint8_t byte_a, uint32_t *order)
{
    actor *a = &((actor *)actor_data->data)[actor_index & 0xffff];
    uint32_t *body = order;
    int32_t i;

    for (i = 0; i < 6; i++) {
        body[i] = 0;
    }

    if (a->order_committed == 0) {
        *((uint8_t *)order + 1) = a->unknown_1cc;
        *((uint8_t *)order + 2) = byte_a;
        order[2] = (uint32_t)game_time->game_time;
        *(int16_t *)((uint8_t *)order + 0xe) = 0;
        *(int16_t *)((uint8_t *)order + 0xc) = 0x78;
        *((uint8_t *)order + 3) = 1;
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        *(int16_t *)(order + 4) = (int16_t)(((random_seed_global >> 0x10) * 300) >> 0x10) + 300;
        return 1;
    }
    return 0;
}

#if 0
Original Ghidra decompilation (0x409a90):

uint FUN_00409a90(undefined1 param_1)

{
  undefined4 uVar1;
  uint in_EAX;
  uint uVar2;
  undefined4 *in_ECX;

  uVar2 = (in_EAX & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  *in_ECX = 0;
  in_ECX[1] = 0;
  in_ECX[2] = 0;
  in_ECX[3] = 0;
  in_ECX[4] = 0;
  in_ECX[5] = 0;
  if (*(char *)(uVar2 + 0x160) == '\0') {
    *(undefined1 *)((int)in_ECX + 1) = *(undefined1 *)(uVar2 + 0x1cc);
    *(undefined1 *)((int)in_ECX + 2) = param_1;
    uVar1 = *(undefined4 *)(DAT_006f1d6c + 0xc);
    *(undefined2 *)((int)in_ECX + 0xe) = 0;
    in_ECX[2] = uVar1;
    *(undefined2 *)(in_ECX + 3) = 0x78;
    *(undefined1 *)((int)in_ECX + 3) = 1;
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    *(short *)(in_ECX + 4) = (short)((random_seed_global >> 0x10) * 300 >> 0x10) + 300;
    return CONCAT31((int3)((uint)uVar1 >> 8),1);
  }
  return uVar2 & 0xffffff00;
}
#endif
