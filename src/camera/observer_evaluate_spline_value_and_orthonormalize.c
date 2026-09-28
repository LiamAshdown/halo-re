// observer_evaluate_spline_value_and_orthonormalize  (Ghidra: FUN_00448210; renamed for this
// rewrite)
// address 0x448210, size 1258 bytes
// name confidence: 0.55   rewrite confidence: 0.8
// evidence: out/phase4/camera_types_notes.md's proposed name and its "value = t*c1+...+c0"
// formula (types/camera.h); field offsets (current_command.parameters +0xc, coefficient_t5..t0,
// velocity +0xe8, parameters +0xb0, forward +0x20/up +0x2c within parameters) all match
// types/camera.h's observer layout, and the two per-channel float-count tables (0x00686ae0
// {3,3,1,1,6} parameter space, 0x00686aec {3,3,1,1,3} derivative space) match exactly. The
// orientation channel (channel 4) hands its computed value (an axis*angle vector) to
// vector3d_rotate_basis_by_axis_angle as EAX, confirmed against objdump ("mov eax,ebp" -- ebp is
// the value scratch -- immediately before "call 0x448880"). The trailing re-orthonormalisation
// (forward/up unit-length and perpendicularity checks, then a double cross product) is Ghidra's
// own decompile, trusted directly; it matches vector3d_is_unit_length's exact formula twice over.
// register convention: local player index in AX (in_AX); no other parameters.
//
// review (phase 4 gate, line by line against objdump 0x448210..0x448702): matches, including
// the expired-channel test (t = countdown - observer_dt; t <= 0 with the command valid bit set
// copies the command parameters, 0x4482a0..0x4482da), the t^5..t^0 evaluation order, the value
// scratch that the binary advances per channel (EBP; the rewrite restarts it at 0, which is
// equivalent) and the three 0.001 (double 0x00672b18) orthonormality tests.
// The += is correct, not an anomaly: 0x447be0 refits the quintic every tick so that P(T) = 0 at
// the current time left T and P(0) = the remaining offset (c0), so P(T - dt) is exactly the part
// of the offset covered during this tick and is added onto the running state.

#include "crt.h"
#include "tags.h"
#include "memory.h"
#include "camera.h"

extern observer observers[1];       // 0x006ac65c
extern float observer_dt;           // 0x006ac658
extern int16_t observer_parameter_float_counts[5];   // 0x00686ae0, {3,3,1,1,6}
extern int16_t observer_derivative_float_counts[5];  // 0x00686aec, {3,3,1,1,3}

extern double sqrt(double x);     // FSQRT
extern double fabs(double x);     // FABS

// blam-cc: EAX -> axis_angle; forward, up = stack (0x448880, this module)
extern void vector3d_rotate_basis_by_axis_angle(Vector3D *axis_angle, Vector3D *forward,
    Vector3D *up);

// blam-cc: AX -> local_player_index
void observer_evaluate_spline_value_and_orthonormalize(int16_t local_player_index)
{
    observer *o = &observers[local_player_index];
    float *command_parameters = (float *)&o->current_command.parameters;
    float *coefficient_t5 = (float *)&o->coefficient_t5;
    float *coefficient_t4 = (float *)&o->coefficient_t4;
    float *coefficient_t3 = (float *)&o->coefficient_t3;
    float *coefficient_t2 = (float *)&o->coefficient_t2;
    float *coefficient_t1 = (float *)&o->coefficient_t1;
    float *coefficient_t0 = (float *)&o->coefficient_t0;
    float *velocity = (float *)&o->velocity;
    float *parameters = (float *)&o->parameters;
    float value[14];
    int16_t channel;
    int32_t derivative_index = 0;
    int32_t parameter_index = 0;

    for (channel = 0; channel < k_observer_parameter_count; channel++) {
        int16_t derivative_count = observer_derivative_float_counts[channel];
        int16_t parameter_count = observer_parameter_float_counts[channel];
        float t = o->current_command.channel_times[channel] - observer_dt;
        int16_t i;

        if (t > 0.0f || (o->current_command.flags & _observer_command_valid_bit) == 0) {
            if (t <= 0.0f) {
                for (i = 0; i < derivative_count; i++) {
                    value[i] = -(observer_dt * velocity[derivative_index + i]);
                }
            } else {
                float t3 = t * t * t;
                float t4 = t3 * t;

                for (i = 0; i < derivative_count; i++) {
                    int32_t idx = derivative_index + i;

                    value[i] = t * coefficient_t1[idx] + t * t * coefficient_t2[idx] +
                        t3 * coefficient_t3[idx] + t4 * coefficient_t4[idx] +
                        t4 * t * coefficient_t5[idx] + coefficient_t0[idx];
                }
            }

            if (channel < _observer_parameter_orientation) {
                // position, focus_offset, distance, field_of_view
                for (i = 0; i < derivative_count; i++) {
                    parameters[parameter_index + i] = value[i] + parameters[parameter_index + i];
                }
            } else {
                // orientation: value is an axis*angle rotation delta
                vector3d_rotate_basis_by_axis_angle((Vector3D *)value,
                    (Vector3D *)&o->parameters.forward, (Vector3D *)&o->parameters.up);
            }
        } else {
            for (i = 0; i < parameter_count; i++) {
                parameters[parameter_index + i] = command_parameters[parameter_index + i];
            }
        }

        derivative_index += derivative_count;
        parameter_index += parameter_count;
    }

    {
        Vector3D *forward = &o->parameters.forward;
        Vector3D *up = &o->parameters.up;
        float check;
        float length;

        check = (forward->k * forward->k + forward->j * forward->j + forward->i * forward->i) -
            1.0f;
        if (_isnan((double)check) == 0 && fabs((double)check) < 0.001) {
            check = (up->k * up->k + up->j * up->j + up->i * up->i) - 1.0f;
            if (_isnan((double)check) == 0 && fabs((double)check) < 0.001) {
                check = up->j * forward->j + up->k * forward->k + up->i * forward->i;
                if (_isnan((double)check) == 0 && fabs((double)check) < 0.001) {
                    return;
                }
            }
        }

        {
            float right_i = up->j * forward->k - up->k * forward->j;
            float right_j = up->k * forward->i - forward->k * up->i;
            float right_k = forward->j * up->i - up->j * forward->i;

            up->i = right_k * forward->j - right_j * forward->k;
            up->j = right_i * forward->k - right_k * forward->i;
            up->k = right_j * forward->i - right_i * forward->j;
        }

        length = (float)sqrt((double)(forward->k * forward->k + forward->j * forward->j +
            forward->i * forward->i));
        if (0.0001 <= fabs((double)length)) {
            length = 1.0f / length;
            forward->i = length * forward->i;
            forward->j = length * forward->j;
            forward->k = length * forward->k;
        }

        length = (float)sqrt((double)(up->k * up->k + up->j * up->j + up->i * up->i));
        if (fabs((double)length) < 0.0001) {
            return;
        }
        length = 1.0f / length;
        up->i = length * up->i;
        up->j = length * up->j;
        up->k = length * up->k;
    }
}

#if 0
Original Ghidra decompilation (0x448210):

/* WARNING: Globals starting with '_' overlap smaller symbols at the same address */

void FUN_00448210(void)

{
  float *pfVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short in_AX;
  short sVar5;
  int iVar6;
  short sVar7;
  undefined *puVar8;
  undefined *puVar9;
  undefined1 *puVar10;
  int iVar11;
  int iVar12;
  float *local_84;
  undefined *local_80;
  undefined *local_70;
  undefined *local_6c;
  undefined *local_64;
  undefined *local_60;
  undefined *local_5c;
  undefined *local_58;
  undefined1 local_38 [56];

  iVar12 = in_AX * 0x29c;
  local_70 = &DAT_006ac668 + iVar12;
  local_58 = &DAT_006ac7b4 + iVar12;
  local_60 = &DAT_006ac7e0 + iVar12;
  local_64 = &DAT_006ac80c + iVar12;
  local_6c = &DAT_006ac838 + iVar12;
  local_5c = &DAT_006ac864 + iVar12;
  puVar8 = &DAT_006ac744 + iVar12;
  local_80 = &DAT_006ac890 + iVar12;
  local_84 = (float *)(&DAT_006ac6b8 + iVar12);
  iVar11 = 0;
  puVar9 = &DAT_006ac70c + iVar12;
  puVar10 = local_38;
  sVar5 = 0;
  do {
    fVar4 = *local_84 - _DAT_006ac658;
    if ((0.0 < fVar4) || (((&DAT_006ac664)[iVar12] & 1) == 0)) {
      sVar7 = 0;
      if (fVar4 <= 0.0) {
        if (0 < *(short *)((int)&DAT_00686aec + iVar11)) {
          do {
            iVar6 = (int)sVar7;
            sVar7 = sVar7 + 1;
            *(float *)(puVar10 + iVar6 * 4) = -(_DAT_006ac658 * *(float *)(puVar8 + iVar6 * 4));
          } while (sVar7 < *(short *)((int)&DAT_00686aec + iVar11));
        }
      }
      else {
        fVar2 = fVar4 * fVar4 * fVar4;
        fVar3 = fVar2 * fVar4;
        if (0 < *(short *)((int)&DAT_00686aec + iVar11)) {
          do {
            iVar6 = sVar7 * 4;
            sVar7 = sVar7 + 1;
            *(float *)(puVar10 + iVar6) =
                 fVar4 * *(float *)(local_5c + iVar6) +
                 fVar4 * fVar4 * *(float *)(local_6c + iVar6) +
                 fVar2 * *(float *)(local_64 + iVar6) +
                 fVar3 * *(float *)(local_60 + iVar6) + fVar3 * fVar4 * *(float *)(local_58 + iVar6)
                 + *(float *)(local_80 + iVar6);
          } while (sVar7 < *(short *)((int)&DAT_00686aec + iVar11));
        }
      }
      if (sVar5 < 4) {
        sVar7 = 0;
        if (0 < *(short *)((int)&DAT_00686aec + iVar11)) {
          do {
            iVar6 = sVar7 * 4;
            sVar7 = sVar7 + 1;
            *(float *)(puVar9 + iVar6) = *(float *)(puVar10 + iVar6) + *(float *)(puVar9 + iVar6);
          } while (sVar7 < *(short *)((int)&DAT_00686aec + iVar11));
        }
      }
      else {
        FUN_00448880(puVar9,puVar9 + 0xc);
      }
    }
    else {
      sVar7 = 0;
      if (0 < *(short *)((int)&DAT_00686ae0 + iVar11)) {
        do {
          iVar6 = (int)sVar7;
          sVar7 = sVar7 + 1;
          *(undefined4 *)(puVar9 + iVar6 * 4) = *(undefined4 *)(local_70 + iVar6 * 4);
        } while (sVar7 < *(short *)((int)&DAT_00686ae0 + iVar11));
      }
    }
    iVar6 = *(short *)((int)&DAT_00686ae0 + iVar11) * 4;
    local_70 = local_70 + iVar6;
    puVar9 = puVar9 + iVar6;
    iVar6 = *(short *)((int)&DAT_00686aec + iVar11) * 4;
    local_58 = local_58 + iVar6;
    local_60 = local_60 + iVar6;
    local_64 = local_64 + iVar6;
    local_6c = local_6c + iVar6;
    local_5c = local_5c + iVar6;
    puVar8 = puVar8 + iVar6;
    puVar10 = puVar10 + iVar6;
    local_80 = local_80 + iVar6;
    local_84 = local_84 + 1;
    sVar5 = sVar5 + 1;
    iVar11 = iVar11 + 2;
  } while (sVar5 < 5);
  pfVar1 = (float *)(&DAT_006ac738 + iVar12);
  fVar4 = (*(float *)(&DAT_006ac734 + iVar12) * *(float *)(&DAT_006ac734 + iVar12) +
          *(float *)(&DAT_006ac730 + iVar12) * *(float *)(&DAT_006ac730 + iVar12) +
          *(float *)(&DAT_006ac72c + iVar12) * *(float *)(&DAT_006ac72c + iVar12)) - 1.0;
  iVar11 = __isnan((double)fVar4);
  if ((iVar11 == 0) && (ABS(fVar4) < 0.001)) {
    fVar4 = (*(float *)(&DAT_006ac740 + iVar12) * *(float *)(&DAT_006ac740 + iVar12) +
            *(float *)(&DAT_006ac73c + iVar12) * *(float *)(&DAT_006ac73c + iVar12) +
            *pfVar1 * *pfVar1) - 1.0;
    iVar11 = __isnan((double)fVar4);
    if ((iVar11 == 0) && (ABS(fVar4) < 0.001)) {
      fVar4 = *(float *)(&DAT_006ac73c + iVar12) * *(float *)(&DAT_006ac730 + iVar12) +
              *(float *)(&DAT_006ac72c + iVar12) * *pfVar1 +
              *(float *)(&DAT_006ac740 + iVar12) * *(float *)(&DAT_006ac734 + iVar12);
      iVar11 = __isnan((double)fVar4);
      if ((iVar11 == 0) && (ABS(fVar4) < 0.001)) {
        return;
      }
    }
  }
  fVar4 = *(float *)(&DAT_006ac73c + iVar12) * *(float *)(&DAT_006ac734 + iVar12) -
          *(float *)(&DAT_006ac740 + iVar12) * *(float *)(&DAT_006ac730 + iVar12);
  fVar2 = *(float *)(&DAT_006ac740 + iVar12) * *(float *)(&DAT_006ac72c + iVar12) -
          *(float *)(&DAT_006ac734 + iVar12) * *pfVar1;
  fVar3 = *(float *)(&DAT_006ac730 + iVar12) * *pfVar1 -
          *(float *)(&DAT_006ac73c + iVar12) * *(float *)(&DAT_006ac72c + iVar12);
  *pfVar1 = fVar3 * *(float *)(&DAT_006ac730 + iVar12) - fVar2 * *(float *)(&DAT_006ac734 + iVar12);
  *(float *)(&DAT_006ac73c + iVar12) =
       fVar4 * *(float *)(&DAT_006ac734 + iVar12) - fVar3 * *(float *)(&DAT_006ac72c + iVar12);
  *(float *)(&DAT_006ac740 + iVar12) =
       fVar2 * *(float *)(&DAT_006ac72c + iVar12) - fVar4 * *(float *)(&DAT_006ac730 + iVar12);
  fVar4 = SQRT(*(float *)(&DAT_006ac734 + iVar12) * *(float *)(&DAT_006ac734 + iVar12) +
               *(float *)(&DAT_006ac730 + iVar12) * *(float *)(&DAT_006ac730 + iVar12) +
               *(float *)(&DAT_006ac72c + iVar12) * *(float *)(&DAT_006ac72c + iVar12));
  if (0.0001 <= ABS(fVar4)) {
    fVar4 = 1.0 / fVar4;
    *(float *)(&DAT_006ac72c + iVar12) = fVar4 * *(float *)(&DAT_006ac72c + iVar12);
    *(float *)(&DAT_006ac730 + iVar12) = fVar4 * *(float *)(&DAT_006ac730 + iVar12);
    *(float *)(&DAT_006ac734 + iVar12) = fVar4 * *(float *)(&DAT_006ac734 + iVar12);
  }
  fVar4 = SQRT(*(float *)(&DAT_006ac740 + iVar12) * *(float *)(&DAT_006ac740 + iVar12) +
               *(float *)(&DAT_006ac73c + iVar12) * *(float *)(&DAT_006ac73c + iVar12) +
               *pfVar1 * *pfVar1);
  if (ABS(fVar4) < 0.0001) {
    return;
  }
  fVar4 = 1.0 / fVar4;
  *pfVar1 = fVar4 * *pfVar1;
  *(float *)(&DAT_006ac73c + iVar12) = fVar4 * *(float *)(&DAT_006ac73c + iVar12);
  *(float *)(&DAT_006ac740 + iVar12) = fVar4 * *(float *)(&DAT_006ac740 + iVar12);
  return;
}
#endif
