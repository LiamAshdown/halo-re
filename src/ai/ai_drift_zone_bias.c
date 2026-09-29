// ai_drift_zone_bias  (Ghidra: ai_drift_zone_bias, renamed)
// address 0x42a9d0, size 180 bytes
// name confidence: 0.35   rewrite confidence: 0.45
// evidence: types/ai.h ai_globals.unknown_0c (a float, global drift rate),
//   encounter.first_squad (0x04), encounter_squad_state.unknown_08 (a float). Phase-4
//   summary: "Randomly nudges a per-zone/index bias value up or down, bounded by a floor
//   derived from a global rate, used to slowly drift an AI behavior parameter over time."
// register convention: EAX -> encounter_index, stack -> squad_offset, bias.
//   // blam-cc: EAX -> encounter_index, stack -> squad_offset, bias

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "ai.h"
#include "fn_ai.h"

extern ai_globals *ai_globals_ptr; // 0x00880354
extern data_array *encounter_data; // 0x008802c8
extern encounter_squad_state *encounter_squad_states; // 0x008802cc
extern uint32_t random_seed_global; // 0x00719cd0
extern double fabs(double x);

// blam-cc: EAX -> encounter_index, stack -> squad_offset, bias
// Nudges one encounter squad's bias field (unknown_08) up or down by a random +-1 step,
// clamped so it never drifts the global rate (ai_globals.unknown_0c) below a floor derived
// from the squad's own current bias, and keeps the two in sync.
uint8_t ai_drift_zone_bias(datum_index encounter_index, int16_t squad_offset, float bias)
{
    uint8_t hit;
    encounter *enc = &((encounter *)encounter_data->data)[encounter_index & 0xffff];
    encounter_squad_state *squad = &encounter_squad_states[(int16_t)(enc->first_squad + squad_offset)];
    float floor = ai_globals_ptr->unknown_0c * -0.33333334f;
    float step;

    if ((float)fabs((double)floor) <= (float)fabs((double)(-squad->unknown_08))) {
        floor = -squad->unknown_08;
    }

    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    hit = (float)(int32_t)(random_seed_global >> 0x10) * 1.5259022e-05f < floor + bias;
    step = (float)hit - bias;

    squad->unknown_08 = step + squad->unknown_08;
    ai_globals_ptr->unknown_0c = step + ai_globals_ptr->unknown_0c;
    return hit; // 0x42aa5c..0x42aa62: AL is the roll result (encounter_squad_spawn_actor uses it)
}

#if 0
Original Ghidra decompilation (0x42a9d0):

void FUN_0042a9d0(short param_1,float param_2)

{
  float fVar1;
  int iVar2;
  uint in_EAX;
  int iVar3;

  iVar2 = DAT_00880354;
  fVar1 = *(float *)(DAT_00880354 + 0xc) * -0.33333334;
  iVar3 = (short)(*(short *)((in_EAX & 0xffff) * 0x6c + 4 + *(int *)(DAT_008802c8 + 0x34)) + param_1
                 ) * 0x20 + DAT_008802cc;
  if (ABS(fVar1) <= ABS(-*(float *)(iVar3 + 8))) {
    fVar1 = -*(float *)(iVar3 + 8);
  }
  random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
  param_2 = (float)((float)(random_seed_global >> 0x10) * 1.5259022e-05 < fVar1 + param_2) - param_2
  ;
  *(float *)(iVar3 + 8) = param_2 + *(float *)(iVar3 + 8);
  *(float *)(iVar2 + 0xc) = param_2 + *(float *)(iVar2 + 0xc);
  return;
}
#endif
