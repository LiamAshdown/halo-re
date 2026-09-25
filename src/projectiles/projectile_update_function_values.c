// projectile_update_function_values  (Ghidra: item_update_function_values; renamed per
// out/phase4/projectiles_types_notes.md: "reads the four ProjectileFunctionIn at tag 0x184 and
// writes object.function_in_values (0x124)")
// address 0x4c0250, size 179 bytes
// name confidence: 0.8   rewrite confidence: 0.9 (raised by the phase-4 verification pass, which re-derived
//   this function from `objdump -d -M intel bin/halo.exe` rather than from the decompilation;
//   the corrections it made are listed in src/projectiles/README.md)
// evidence: types/tags.h ProjectileFunctionIn (none/range_remaining/time_remaining/tracer),
//   Projectile.projectile_a_in..d_in (tag 0x184, four ProjectileFunctionIn_t); types/objects.h
//   object.function_in_values (0x124); types/projectiles.h projectile_data.distance_travelled
//   (0x250), .detonation_timer (0x240), .flags (0x22c) and Projectile.maximum_range (tag 0x1c8);
//   _projectile_tracer_bit (0x02).
// Leaves a function_in_values[] slot at its previous value when the corresponding
// ProjectileFunctionIn is projectilefunctionin_none, rather than zeroing it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "objects.h"
#include "projectiles.h"

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14

// FIXED (register inputs, objdump): the original never reads EAX as an input (it overwrites or only saves it); those parameters arrive on the stack (1 stack argument(s) read).
// blam-cc: stack -> object_index
void projectile_update_function_values(uint32_t object_index)
{
    object *obj = ((object_header *)object_data->data)[object_index & 0xffff].data;
    Projectile *tag = (Projectile *)tag_instances[(uint16_t)obj->definition_tag].data;
    projectile_data *proj = (projectile_data *)((uint8_t *)obj + k_projectile_data_offset);
    ProjectileFunctionIn_t *function_in = &tag->projectile_a_in; // four contiguous int16 fields
    float *out = obj->function_in_values;
    int32_t i;

    for (i = 0; i < 4; i++) {
        ProjectileFunctionIn_t kind = function_in[i];
        float value = 0.0f; // the shared fallback: written as-is for range_remaining with a
                             // zero maximum_range, for tracer while the tracer bit is clear, and
                             // for any kind value Ghidra's if/else-if chain does not recognize

        if (kind == projectilefunctionin_none) {
            continue; // leaves out[i] at its previous value; every other kind writes below
        }
        if (kind == projectilefunctionin_range_remaining) {
            if (tag->maximum_range != 0.0f) {
                value = proj->distance_travelled / tag->maximum_range;
            }
        } else if (kind == projectilefunctionin_time_remaining) {
            value = proj->detonation_timer;
        } else if (kind == projectilefunctionin_tracer) {
            if ((proj->flags & _projectile_tracer_bit) != 0) {
                value = 1.0f;
            }
        }
        out[i] = value;
    }
}

#if 0
Original Ghidra decompilation (0x4c0250):

void __cdecl item_update_function_values(uint item_index)

{
  float fVar1;
  short sVar2;
  uint *puVar3;
  int iVar4;
  short *psVar5;
  int iVar6;
  float *pfVar7;

  puVar3 = *(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (item_index & 0xffff) * 0xc);
  iVar4 = *(int *)((*puVar3 & 0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  pfVar7 = (float *)(puVar3 + 0x49);
  psVar5 = (short *)(iVar4 + 0x184);
  iVar6 = 4;
  do {
    sVar2 = *psVar5;
    if (sVar2 != 0) {
      fVar1 = 0.0;
      if (sVar2 == 1) {
        if (*(float *)(iVar4 + 0x1c8) == 0.0) {
LAB_004c02ee:
          fVar1 = 0.0;
        }
        else {
          fVar1 = (float)puVar3[0x94] / *(float *)(iVar4 + 0x1c8);
        }
      }
      else if (sVar2 == 2) {
        fVar1 = (float)puVar3[0x90];
      }
      else if (sVar2 == 3) {
        if ((puVar3[0x8b] & 2) == 0) goto LAB_004c02ee;
        fVar1 = 1.0;
      }
      *pfVar7 = fVar1;
    }
    psVar5 = psVar5 + 1;
    pfVar7 = pfVar7 + 1;
    iVar6 = iVar6 + -1;
    if (iVar6 == 0) {
      return;
    }
  } while( true );
}
#endif
