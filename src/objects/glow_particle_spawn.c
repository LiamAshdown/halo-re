// glow_particle_spawn
// address 0x4fdb20, size 699 bytes
// name confidence: 0.7 (out/phase4/objects_types_notes.md misattribution table: "0x4fdb20 |
//   lightning_segment_spawn | glow_particle_spawn")
// rewrite confidence: 0.3 (random-fill logic mirrors glow_particle_new.c closely; kept as raw
//   Glow tag offsets for the same reason; the __ftol call feeding glow_particle+0x52
//   (lifetime) has no clearly-computed float argument in Ghidra's own decompile -- it follows
//   Ghidra's note "Removing unreachable block", so some elided code likely fed it -- and is
//   modeled here as truncating the just-computed particle size, which is a guess)
// evidence: types/objects.h glow (definition_tag 0x224, marker_count 0x04, particle_count
//   0x228, total_length 0x234), glow_particle (t 0x28, handle-adjacent segment index 0x02,
//   lifetime 0x52, flags 0x54 bit 1); this module's glow_particle_datum_new 0x4fdde0,
//   glow_particle_reposition 0x4fde40; __ftol 0x6391b4; vector3d_normalize_with_length 0x401990
//   (vector in ECX, established in antenna_update_physics.c).
// register convention: Ghidra shows a single unresolved `unaff_EBX`; by the same +0x224 anchor
//   as every sibling function, EBX is the glow entry.
// blam-cc: EBX -> entry

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "fn_math.h"

extern tag_instance *tag_instances; // 0x0087bc14
extern uint32_t effect_random_seed; // 0x00719cd4

extern glow_particle *glow_particle_datum_new(void); // this module, 0x4fdde0
extern void glow_particle_reposition(glow *entry, uint8_t *particle, float phase_rate); // 0x4fde40

extern int32_t __ftol(); // 0x006391b4, MSVC 7.1 CRT x87 float-to-int truncation
    // (verified by disassembling 0x006391b4: fld st(0) / fst [esp+0x18] / fistp qword /
    // fild qword ... , the classic _ftol2 body). The value arrives on the x87 stack, so
    // some call sites show a visible float argument and others show none; the empty
    // parameter list asserts no prototype, the same convention this module already uses
    // for FUN_00450870.

static float glow_next_random_unit(void)
{
    effect_random_seed = effect_random_seed * 0x19660dU + 0x3c6ef35fU;
    return (float)(effect_random_seed >> 16) * 1.5259022e-05f;
}

glow_particle *glow_particle_spawn(glow *entry /*EBX*/) // blam-cc: EBX -> entry
{
    uint8_t *tag = (uint8_t *)tag_instances[entry->definition_tag & 0xffff].data;
    glow_particle *p = glow_particle_datum_new();

    if (p != 0) {
        uint8_t *pb = (uint8_t *)p;
        float size;

        if (entry->marker_count < 2) {
            *(float *)(pb + 0x2c) = *(float *)((uint8_t *)entry + 0x68);
            *(float *)(pb + 0x30) = *(float *)((uint8_t *)entry + 0x6c);
            *(float *)(pb + 0x34) = *(float *)((uint8_t *)entry + 0x70);
        } else {
            float lo = *(float *)(tag + 0x108) * entry->total_length;
            float hi = *(float *)(tag + 0x10c) * entry->total_length;
            ((struct glow_particle *)pb)->t = (hi - lo) * glow_next_random_unit() + lo;
            glow_particle_reposition(entry, pb, 0.0f);
        }

        {
            int16_t velocity_mode = *(int16_t *)(tag + 0x26);
            if (velocity_mode == 0) {
                *(float *)(pb + 0x38) = 0.0f;
                *(float *)(pb + 0x3c) = 0.0f;
                *(float *)(pb + 0x40) = 1.0f;
            } else if (velocity_mode == 1) {
                uint8_t *marker = (uint8_t *)entry + ((struct glow_particle *)pb)->unknown_02 * 0x6c + 0x5c;
                *(float *)(pb + 0x38) = *(float *)marker;
                *(float *)(pb + 0x3c) = *(float *)(marker + 4);
                *(float *)(pb + 0x40) = *(float *)(marker + 8);
            } else if (velocity_mode == 2) {
                real_vector3d v;
                v.i = glow_next_random_unit() * 2.0f - 1.0f;
                v.j = glow_next_random_unit() * 2.0f - 1.0f;
                v.k = glow_next_random_unit() * 2.0f - 1.0f;
                vector3d_normalize_with_length(&v);
                *(float *)(pb + 0x38) = v.i;
                *(float *)(pb + 0x3c) = v.j;
                *(float *)(pb + 0x40) = v.k;
            }
        }

        {
            float speed = *(float *)(tag + 0x104) * 0.033333335f;
            *(float *)(pb + 0x38) *= speed;
            *(float *)(pb + 0x3c) *= speed;
            *(float *)(pb + 0x40) *= speed;
        }

        size = *(float *)(tag + 0xa0) +
               (*(float *)(tag + 0xa4) - *(float *)(tag + 0xa0)) * glow_next_random_unit();
        *(float *)(pb + 0x20) = size / (float)entry->particle_count;

        ((struct glow_particle *)pb)->lifetime = (int16_t)__ftol((double)*(float *)(pb + 0x20)); // UNSURE, see file header

        {
            float t = glow_next_random_unit();
            *(uint32_t *)(pb + 0xc) = 0x3f800000; // 1.0f
            *(float *)(pb + 0x10) = (*(float *)(tag + 0xc8) - *(float *)(tag + 0xb8)) * t + *(float *)(tag + 0xb8);
            *(float *)(pb + 0x14) = (*(float *)(tag + 0xcc) - *(float *)(tag + 0xbc)) * t + *(float *)(tag + 0xbc);
            ((struct glow_particle *)pb)->flags = ((struct glow_particle *)pb)->flags | 2;
            *(float *)(pb + 0x18) = (*(float *)(tag + 0xd0) - *(float *)(tag + 0xc0)) * t + *(float *)(tag + 0xc0);
        }
    }
    return p;
}

#if 0
Original Ghidra decompilation (0x4fdb20):

int FUN_004fdb20(void)

{
  undefined4 *puVar1;
  float fVar2;
  float fVar3;
  float fVar4;
  short sVar5;
  int iVar6;
  float fVar7;
  undefined2 uVar8;
  int iVar9;
  uint uVar10;
  int unaff_EBX;

  iVar6 = *(int *)((*(uint *)(unaff_EBX + 0x224) & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  iVar9 = FUN_004fdde0();
  if (iVar9 != 0) {
    if (*(short *)(unaff_EBX + 4) < 2) {
      *(undefined4 *)(iVar9 + 0x2c) = *(undefined4 *)(unaff_EBX + 0x68);
      *(undefined4 *)(iVar9 + 0x30) = *(undefined4 *)(unaff_EBX + 0x6c);
      *(undefined4 *)(iVar9 + 0x34) = *(undefined4 *)(unaff_EBX + 0x70);
    }
    else {
      fVar2 = *(float *)(iVar6 + 0x108) * *(float *)(unaff_EBX + 0x234);
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      *(float *)(iVar9 + 0x28) =
           (*(float *)(iVar6 + 0x10c) * *(float *)(unaff_EBX + 0x234) - fVar2) *
           (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05 + fVar2;
      FUN_004fde40(iVar9,0);
    }
    sVar5 = *(short *)(iVar6 + 0x26);
    if (sVar5 == 0) {
      *(undefined4 *)(iVar9 + 0x38) = 0;
      *(undefined4 *)(iVar9 + 0x3c) = 0;
      *(undefined4 *)(iVar9 + 0x40) = 0x3f800000;
    }
    else if (sVar5 == 1) {
      puVar1 = (undefined4 *)(*(short *)(iVar9 + 2) * 0x6c + 0x5c + unaff_EBX);
      *(undefined4 *)(iVar9 + 0x38) = *puVar1;
      *(undefined4 *)(iVar9 + 0x3c) = puVar1[1];
      *(undefined4 *)(iVar9 + 0x40) = puVar1[2];
    }
    else if (sVar5 == 2) {
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      fVar2 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
      *(float *)(iVar9 + 0x38) = (fVar2 + fVar2) - 1.0;
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      fVar2 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
      *(float *)(iVar9 + 0x3c) = (fVar2 + fVar2) - 1.0;
      DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
      fVar2 = (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
      *(float *)(iVar9 + 0x40) = (fVar2 + fVar2) - 1.0;
      vector3d_normalize_with_length();
    }
    fVar2 = *(float *)(iVar6 + 0x104) * 0.033333335;
    *(float *)(iVar9 + 0x38) = fVar2 * *(float *)(iVar9 + 0x38);
    *(float *)(iVar9 + 0x3c) = fVar2 * *(float *)(iVar9 + 0x3c);
    *(float *)(iVar9 + 0x40) = fVar2 * *(float *)(iVar9 + 0x40);
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    fVar2 = *(float *)(iVar6 + 0xa0) +
            (*(float *)(iVar6 + 0xa4) - *(float *)(iVar6 + 0xa0)) *
            (float)(DAT_00719cd4 >> 0x10) * 1.5259022e-05;
    *(float *)(iVar9 + 0x20) = fVar2;
    *(float *)(iVar9 + 0x20) = fVar2 / (float)(int)*(short *)(unaff_EBX + 0x228);
    uVar8 = FUN_006391b4();
    *(undefined2 *)(iVar9 + 0x52) = uVar8;
    DAT_00719cd4 = DAT_00719cd4 * 0x19660d + 0x3c6ef35f;
    uVar10 = DAT_00719cd4 >> 0x10;
    *(undefined4 *)(iVar9 + 0xc) = 0x3f800000;
    fVar7 = (float)uVar10 * 1.5259022e-05;
    *(float *)(iVar9 + 0x10) =
         (*(float *)(iVar6 + 200) - *(float *)(iVar6 + 0xb8)) * fVar7 + *(float *)(iVar6 + 0xb8);
    *(float *)(iVar9 + 0x14) =
         (*(float *)(iVar6 + 0xcc) - *(float *)(iVar6 + 0xbc)) * fVar7 + *(float *)(iVar6 + 0xbc);
    fVar2 = *(float *)(iVar6 + 0xd0);
    fVar3 = *(float *)(iVar6 + 0xc0);
    fVar4 = *(float *)(iVar6 + 0xc0);
    *(uint *)(iVar9 + 0x54) = *(uint *)(iVar9 + 0x54) | 2;
    *(float *)(iVar9 + 0x18) = (fVar2 - fVar3) * fVar7 + fVar4;
  }
  return iVar9;
}
#endif
