// contrail_update  (Ghidra: chimera__contrail_update; the "chimera__" prefix is a Chimera
// signature match artifact, not part of the retail symbol -- out/phase4/effects_types_notes.md:
// "chimera__contrail_update 0x44cb50 ... [is a] Chimera signature name for contrail_update")
// address 0x44cb50, size 571 bytes
// name confidence: 0.5   rewrite confidence: 0.4
// evidence: types/effects.h contrail (every field), types/tags.h Contrail (animation_rate 0x2c,
// texture_animation_u/v 0x24/0x28, ContrailScaleFlags bits 5/8/9); types/objects.h
// function_out_values/function_valid_flags.
// register convention: __cdecl, delta_time on the stack.
// UNSURE: the tail of the original decompile inlines datum_next 0x4d0630's own body instead of
// calling it (Ghidra failed to recognise the second call as a call at all); it is byte-for-byte
// the same scan datum_next performs, so this rewrite calls datum_next directly instead of
// duplicating its logic, which is semantically identical.
// UNSURE: the per-contrail sequence/frame-advance loop ("while more than one frame's worth of
// animation time has elapsed, call contrail_next_sequence and consume it") is reconstructed by
// analogy with the structurally identical loop in contrail_points_due 0x44cf80; Ghidra's own
// decompile of this loop is corrupted by stale x87 FPU stack values (extraout_ST1) that cannot
// be resolved from the decompiled C alone.
// UNSURE: the force-regenerate-a-point call when the emitting state flips
// (contrail_generate_points(contrail_index, 1, 1) with scale temporarily zeroed) matches
// contrail_new's own (1, 1) call exactly; the temporary zero-scale is preserved as given even
// though its purpose is not established here.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "effects.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern data_array *contrail_data; // 0x0087abec
extern data_array *object_data;   // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

extern datum_index datum_next(int16_t after_index, data_array *array); // 0x4d0630,
    // memory module; blam-cc: DX -> after_index, EDI -> array
extern void contrail_next_sequence(contrail *self); // 0x44ced0, this module; blam-cc: EAX -> self
extern int16_t contrail_points_due(datum_index contrail_handle, real elapsed_time); // 0x44cf80,
    // this module; blam-cc: EAX -> contrail_handle, stack -> elapsed_time
extern void contrail_generate_points(datum_index contrail_handle, int16_t point_count,
    uint8_t force); // 0x44d020, this module;
    // blam-cc: EAX -> contrail_handle, stack -> (point_count, force)
extern void contrail_age_points(datum_index contrail_handle, real delta_time); // 0x44d470,
    // this module; blam-cc: EAX -> contrail_handle, stack -> delta_time
extern void contrail_delete(datum_index contrail_index); // 0x44cad0, this module

// Per-tick driver: advances every live contrail's emitting state, sequence/frame animation and
// texture scroll, ages its points, and deletes it once it has no points left and no owning
// object.
void contrail_update(real delta_time)
{
    datum_index contrail_index = datum_next(-1, contrail_data);

    while (contrail_index != k_datum_index_none) {
        contrail *self = &((contrail *)contrail_data->data)[(uint16_t)contrail_index];
        Contrail *tag = (Contrail *)tag_instances[(uint16_t)self->definition_index].data;
        real remaining = delta_time - self->accumulated_delta_time;

        self->accumulated_delta_time = 0.0f;

        if (self->object_index != k_datum_index_none) {
            int16_t scale_function_index = self->scale_function_index;
            uint8_t want_emitting;

            if (scale_function_index == -1) {
                self->scale = 1.0f;
                want_emitting = 1;
            } else {
                object *owner = ((object_header *)object_data->data)[(uint16_t)self->object_index].data;
                self->scale = owner->function_out_values[scale_function_index];
                want_emitting = (owner->function_valid_flags & (1u << (scale_function_index & 0x1f))) != 0;
            }

            if (want_emitting != (uint8_t)(self->flags & _contrail_emitting_bit)) {
                real saved_scale = self->scale;
                self->scale = 0.0f;
                contrail_generate_points(contrail_index, 1, 1);
                self->scale = saved_scale;
            }

            if (!want_emitting) {
                self->flags = self->flags & ~_contrail_emitting_bit;
            } else {
                self->flags = self->flags | _contrail_emitting_bit;
                {
                    int16_t due = contrail_points_due(contrail_index, delta_time);
                    contrail_generate_points(contrail_index, due, 0);
                }
            }
        }

        {
            real animation_rate = tag->animation_rate;
            if ((tag->scale_flags & (1u << 5)) != 0) {
                animation_rate = animation_rate * self->scale;
            }

            while (remaining > 0.0f) {
                real time_to_next_frame = (1.0f / animation_rate) - self->animation_timer;
                if (time_to_next_frame > remaining) {
                    self->animation_timer = self->animation_timer + remaining;
                    break;
                }
                contrail_next_sequence(self); // resets animation_timer to 0
                remaining = remaining - time_to_next_frame;
            }
        }

        {
            real u_rate = tag->texture_animation_u;
            if ((tag->scale_flags & (1u << 8)) != 0) {
                u_rate = u_rate * self->scale;
            }
            self->texture_offset_u = self->texture_offset_u - u_rate * remaining;
        }
        {
            real v_rate = tag->texture_animation_v;
            if ((tag->scale_flags & (1u << 9)) != 0) {
                v_rate = v_rate * self->scale;
            }
            self->texture_offset_v = self->texture_offset_v + v_rate * remaining;
        }

        contrail_age_points(contrail_index, delta_time);

        {
            int list;
            for (list = 0; list < 4 && self->first_point[list] == k_datum_index_none; list++) {
            }
            if (list == 4 && self->object_index == k_datum_index_none) {
                contrail_delete(contrail_index);
            }
        }

        contrail_index = datum_next((int16_t)contrail_index, contrail_data);
    }
}

#if 0
Original Ghidra decompilation (0x44cb50):

void __cdecl chimera__contrail_update(float delta_time)

{
  float fVar1;
  float fVar2;
  uint contrail_index;
  undefined4 uVar3;
  short *psVar4;
  short sVar5;
  int iVar6;
  int iVar7;
  int iVar8;
  int iVar9;
  bool bVar10;
  ushort uVar11;
  float10 fVar12;
  float10 fVar13;
  float10 extraout_ST1;

  iVar7 = DAT_0087abec;
  contrail_index = datum_next();
  do {
    do {
      if (contrail_index == 0xffffffff) {
        return;
      }
      iVar6 = *(int *)(iVar7 + 0x34);
      iVar8 = (contrail_index & 0xffff) * 0x44;
      fVar2 = delta_time - *(float *)(iVar8 + 0x28 + iVar6);
      iVar9 = iVar8 + iVar6;
      iVar6 = *(int *)((*(uint *)(iVar8 + 4 + iVar6) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
      *(undefined4 *)(iVar9 + 0x28) = 0;
      if (*(uint *)(iVar9 + 8) != 0xffffffff) {
        sVar5 = *(short *)(iVar9 + 0xe);
        iVar8 = *(int *)(*(int *)(DAT_008603b0 + 0x34) + 8 + (*(uint *)(iVar9 + 8) & 0xffff) * 0xc);
        if (sVar5 == -1) {
          *(undefined4 *)(iVar9 + 0x10) = 0x3f800000;
          bVar10 = true;
        }
        else {
          *(undefined4 *)(iVar9 + 0x10) = *(undefined4 *)(iVar8 + 0x134 + sVar5 * 4);
          bVar10 = (*(byte *)(iVar8 + 0x123) & (byte)(1 << ((byte)sVar5 & 0x1f))) != 0;
          iVar7 = DAT_0087abec;
        }
        if (bVar10 != (bool)(*(byte *)(iVar9 + 2) & 1)) {
          uVar3 = *(undefined4 *)(iVar9 + 0x10);
          *(undefined4 *)(iVar9 + 0x10) = 0;
          contrail_generate_points(1,1);
          iVar7 = DAT_0087abec;
          *(undefined4 *)(iVar9 + 0x10) = uVar3;
        }
        if (bVar10 == false) {
          *(byte *)(iVar9 + 2) = *(byte *)(iVar9 + 2) & 0xfe;
        }
        else {
          *(byte *)(iVar9 + 2) = *(byte *)(iVar9 + 2) | 1;
          uVar3 = FUN_0044cf80(delta_time,1);
          contrail_generate_points(uVar3);
          iVar7 = DAT_0087abec;
        }
      }
      fVar12 = (float10)fVar2;
      fVar1 = *(float *)(iVar6 + 0x2c);
      if ((*(byte *)(iVar6 + 2) & 0x20) != 0) {
        fVar1 = fVar1 * *(float *)(iVar9 + 0x10);
      }
      uVar11 = (ushort)(fVar2 < 0.0) << 8 | (ushort)(fVar2 == 0.0) << 0xe;
      while (uVar11 == 0) {
        fVar13 = (float10)(1.0 / fVar1) - (float10)*(float *)(iVar9 + 0x24);
        if (fVar13 < fVar12 == (fVar13 == fVar12)) {
          *(float *)(iVar9 + 0x24) = (float)(fVar12 + (float10)*(float *)(iVar9 + 0x24));
          break;
        }
        fVar12 = (float10)FUN_0044ced0();
        fVar12 = extraout_ST1 - fVar12;
        uVar11 = (ushort)(fVar12 < (float10)0.0) << 8 | (ushort)(fVar12 == (float10)0.0) << 0xe;
      }
      fVar1 = *(float *)(iVar6 + 0x24);
      if ((*(byte *)(iVar6 + 3) & 1) != 0) {
        fVar1 = fVar1 * *(float *)(iVar9 + 0x10);
      }
      *(float *)(iVar9 + 0x18) = *(float *)(iVar9 + 0x18) - fVar1 * fVar2;
      fVar1 = *(float *)(iVar6 + 0x28);
      if ((*(byte *)(iVar6 + 3) & 2) != 0) {
        fVar1 = fVar1 * *(float *)(iVar9 + 0x10);
      }
      *(float *)(iVar9 + 0x1c) = fVar1 * fVar2 + *(float *)(iVar9 + 0x1c);
      FUN_0044d470(contrail_index,delta_time);
      sVar5 = 0;
      do {
        if (*(int *)(iVar9 + 0x34 + sVar5 * 4) != -1) break;
        sVar5 = sVar5 + 1;
      } while (sVar5 < 4);
      if ((sVar5 == 4) && (*(int *)(iVar9 + 8) == -1)) {
        contrail_delete(contrail_index);
      }
      iVar6 = contrail_index + 1;
      sVar5 = (short)iVar6;
      contrail_index = 0xffffffff;
    } while ((sVar5 < 0) || (*(short *)(iVar7 + 0x2e) <= sVar5));
    psVar4 = (short *)((int)sVar5 * (int)*(short *)(iVar7 + 0x22) + *(int *)(iVar7 + 0x34));
    do {
      if (*psVar4 != 0) {
        contrail_index = (int)*psVar4 << 0x10 | (int)(short)iVar6;
        break;
      }
      iVar6 = iVar6 + 1;
      psVar4 = (short *)((int)psVar4 + (int)*(short *)(iVar7 + 0x22));
    } while ((short)iVar6 < *(short *)(iVar7 + 0x2e));
  } while( true );
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
