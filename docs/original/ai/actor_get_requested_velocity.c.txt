// actor_get_requested_velocity  (Ghidra: actor_get_requested_velocity, renamed)
// address 0x417fa0, size 275 bytes
// name confidence: 0.4   rewrite confidence: 0.8
// evidence: reads back the four-float movement request actor_movement_update parks at
// actor+0x534..0x540 together with the "request pending" byte at actor+0x530 (inside
// unknown_530[20]), turns it into a velocity vector ((dir_x, dir_y) * speed, dz), clamps
// that vector to the caller's speed limit, and clears the pending byte. Swarm actors
// (actor.swarm) are handed off to their per-type vtable slot 0x1c instead.
// register convention: the clamp-exempt flag arrives in AL, the actor index in ECX and the
// output vector in EDX; the two floats are genuine stack parameters (objdump: [esp+0x1c]
// and [esp+0x20] behind the 0x18-byte frame).
// blam-cc: AL -> skip_clamp, ECX -> actor_index, EDX -> out_velocity, stack -> param_1, speed_limit
// UNSURE: mode_data[4] (actor+0xa0) compared against 3 while actor.mode is 10 is transcribed
// literally; the meaning of that mode-data word is not established anywhere in the module.
// UNSURE: Ghidra drops the third argument of actor_dispatch_type_vtable_0x1c; the
// disassembly pushes three dwords (param_1, speed_limit, out_velocity) and then adds 0xc.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *actor_data; // 0x00880360

extern double sqrt(double x); // FSQRT, Ghidra SQRT() pseudo-function

extern void actor_dispatch_type_vtable_0x1c(datum_index actor_index, uint32_t a, uint32_t b, uint32_t c); // 0x4266d0, ECX, stack

// blam-cc: AL -> skip_clamp, ECX -> actor_index, EDX -> out_velocity, stack -> param_1, speed_limit
uint8_t actor_get_requested_velocity(uint8_t skip_clamp, datum_index actor_index,
                                     real_vector3d *out_velocity, uint32_t object_index,
                                     float speed_limit)
{
    actor *self;
    float length;
    float scale;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    if (self->active_unit_index == (datum_index)k_datum_index_none) {
        if (self->swarm != 0) {
            // 0x417fd6: ECX = actor, stack (param_1, speed_limit, EDX = out_velocity)
            actor_dispatch_type_vtable_0x1c(actor_index, object_index, *(uint32_t *)&speed_limit, (uint32_t)out_velocity);
            self->jump_velocity_request[0] = 0; // 0x530
            return 1;
        }
        if (self->jump_velocity_request[0] != 0) { // 0x530
            if (self->mode == 10 && *(int16_t *)&self->mode_data.raw[4] == 3) { // 0xa0
                skip_clamp = 1;
            }
            out_velocity->j = *(float *)&self->jump_velocity_request[8] *
                              *(float *)&self->jump_velocity_request[12]; // 0x538 * 0x53c
            out_velocity->k = *(float *)&self->jump_velocity_request[16]; // 0x540
            out_velocity->i = *(float *)&self->jump_velocity_request[4] *
                              *(float *)&self->jump_velocity_request[12]; // 0x534 * 0x53c
            length = (float)sqrt((double)(out_velocity->i * out_velocity->i +
                                          out_velocity->j * out_velocity->j +
                                          out_velocity->k * out_velocity->k));
            if (skip_clamp == 0 && speed_limit < length) {
                scale = speed_limit / length;
                out_velocity->i = scale * out_velocity->i;
                out_velocity->j = scale * out_velocity->j;
                out_velocity->k = scale * out_velocity->k;
            }
        }
    }

    self->jump_velocity_request[0] = 0; // 0x530
    return 1;
}

#if 0
Original Ghidra decompilation (0x417fa0):

undefined4 FUN_00417fa0(undefined4 param_1,float param_2)

{
  float fVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  char in_AL;
  uint in_ECX;
  float *in_EDX;
  int iVar5;
  int iVar6;

  iVar5 = (in_ECX & 0xffff) * 0x724;
  iVar6 = iVar5 + *(int *)(DAT_00880360 + 0x34);
  if (*(int *)(iVar5 + 0x158 + *(int *)(DAT_00880360 + 0x34)) == -1) {
    if (*(char *)(iVar6 + 6) != '\0') {
      actor_dispatch_type_vtable_0x1c(param_1,param_2);
      *(undefined1 *)(iVar6 + 0x530) = 0;
      return 1;
    }
    if (*(char *)(iVar6 + 0x530) != '\0') {
      if ((*(short *)(iVar6 + 0x6c) == 10) && (*(short *)(iVar6 + 0xa0) == 3)) {
        in_AL = '\x01';
      }
      fVar4 = *(float *)(iVar6 + 0x538) * *(float *)(iVar6 + 0x53c);
      fVar3 = *(float *)(iVar6 + 0x540);
      fVar1 = *(float *)(iVar6 + 0x534);
      fVar2 = *(float *)(iVar6 + 0x53c);
      in_EDX[1] = fVar4;
      in_EDX[2] = fVar3;
      *in_EDX = fVar1 * fVar2;
      fVar1 = SQRT(fVar3 * fVar3 + fVar4 * fVar4 + *in_EDX * *in_EDX);
      if (in_AL == '\0') {
        if (param_2 < fVar1) {
          param_2 = param_2 / fVar1;
          *in_EDX = param_2 * *in_EDX;
          in_EDX[1] = fVar4 * param_2;
          in_EDX[2] = param_2 * fVar3;
        }
      }
    }
  }
  *(undefined1 *)(iVar6 + 0x530) = 0;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
