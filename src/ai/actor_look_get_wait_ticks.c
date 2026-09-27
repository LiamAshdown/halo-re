// actor_look_get_wait_ticks  (Ghidra: actor_look_get_wait_ticks, already named)
// address 0x415150, size 267 bytes
// name confidence: 0.5   rewrite confidence: 0.9
// evidence: phase-4 summary matches directly; picks one of three float pairs out of a
// caller-supplied table by mode, randomizes (or defaults to 0.5) between them, scales by the
// threat's weapon tag float at +0x410 and an optional 1.5x bonus, then converts to ticks at
// 30 ticks/second with a floor of 1.
// register convention: reconstructed from objdump -d -M intel over 0x415150..0x41525a.
// mode and flags are genuine stack parameters (Ghidra found both); the float table pointer
// is EDI, an implicit register argument Ghidra rendered as unaff_EDI.
// blam-cc: EAX -> actor_index, stack -> mode, stack -> flags, EDI -> deviation_table
// FIXED (objdump 0x415150): EAX is the actor, whose threat weapon (0x4282c0, then its tag) scales the wait by
//   Weapon +0x410; the draft had no actor and called a different helper without operands.
// UNSURE: flags is a 4-byte union -- its bit pattern is used both as a float (the fallback
// value when mode selects neither of the three real pairs) and as a plain byte (the 1.5x
// bonus flag); reinterpreted via a pointer cast to match exactly what the disassembly reads,
// rather than splitting it into two parameters that were never two parameters in the ABI.
// UNSURE: the Weapon tag field at +0x410 has no established name in types/tags.h; kept as a
// raw offset off the Weapon tag pointer actor_get_threat_weapon_definition already resolves.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "cache.h"
#include "ai.h"

extern uint32_t random_seed_global; // 0x00719cd0

extern data_array *object_data;     // 0x008603b0
extern tag_instance *tag_instances; // 0x0087bc14
extern datum_index actor_get_threat_weapon_object_index(datum_index actor_index); // 0x4282c0, EAX
extern int32_t fistp_round(float x); // harness/x87_shims.c

// blam-cc: stack -> mode, stack -> flags, EDI -> deviation_table
// mode 0/1/2 select deviation_table[0..1]/[2..3]/[4..5]; any other mode falls back to using
// flags (reinterpreted as a float) as both ends of the pair, which collapses the random step
// below to always return 0.5.
int32_t actor_look_get_wait_ticks(datum_index actor_index, int16_t mode, uint32_t flags, float *deviation_table)
{
    float lo, hi;
    float fraction;
    float ticks;
    uint32_t rng;
    void *weapon_definition;
    int32_t result;

    switch (mode) {
    case 0:
        lo = deviation_table[0];
        hi = deviation_table[1];
        break;
    case 1:
        lo = deviation_table[2];
        hi = deviation_table[3];
        break;
    case 2:
        lo = deviation_table[4];
        hi = deviation_table[5];
        break;
    default:
        lo = *(float *)&flags;
        hi = *(float *)&flags;
        break;
    }

    // UNSURE: this is Ghidra's exact NaN-safe idiom for "lo > 0.0 (or NaN) or hi > 0.0 (or
    // NaN)", preserved literally rather than simplified, since the two are not equivalent
    // for lo/hi <= 0.
    if (((lo < 0.0f) == (lo == 0.0f)) || ((hi < 0.0f) == (hi == 0.0f))) {
        random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
        rng = random_seed_global;
        fraction = (hi - lo) * (float)(rng >> 0x10) * 1.5259022e-05f + lo;
    } else {
        fraction = 0.5f;
    }

    {
        datum_index weapon = actor_get_threat_weapon_object_index(actor_index);

        weapon_definition = weapon == k_datum_index_none ? 0 :
            tag_instances[*(datum_index *)((object_header *)object_data->data)[weapon & 0xffff].data & 0xffff].data;
    }
    if (weapon_definition != 0 && 0.0f < *(float *)((uint8_t *)weapon_definition + 0x410)) {
        fraction = fraction * *(float *)((uint8_t *)weapon_definition + 0x410);
    }

    if ((uint8_t)flags != 0) {
        fraction = fraction * 1.5f;
    }

    ticks = fraction * 30.0f;
    result = fistp_round(ticks); // 0x415248 fistp
    if (result < 2) {
        result = 1;
    }
    return result;
}

#if 0
Original Ghidra decompilation (0x415150):

int actor_look_get_wait_ticks(short param_1,float param_2)

{
  float fVar1;
  float fVar2;
  uint uVar3;
  int iVar4;
  float *unaff_EDI;

  iVar4 = 0;
  uVar3 = actor_get_threat_weapon_object_index();
  if (uVar3 != 0xffffffff) {
    iVar4 = *(int *)((**(uint **)(*(int *)(DAT_008603b0 + 0x34) + 8 + (uVar3 & 0xffff) * 0xc) &
                     0xffff) * 0x20 + 0x14 + DAT_0087bc14);
  }
  if (param_1 == 0) {
    fVar1 = *unaff_EDI;
    fVar2 = unaff_EDI[1];
  }
  else if (param_1 == 1) {
    fVar1 = unaff_EDI[2];
    fVar2 = unaff_EDI[3];
  }
  else {
    fVar1 = param_2;
    fVar2 = param_2;
    if (param_1 == 2) {
      fVar1 = unaff_EDI[4];
      fVar2 = unaff_EDI[5];
    }
  }
  if ((fVar1 < 0.0 == (fVar1 == 0.0)) || (fVar2 < 0.0 == (fVar2 == 0.0))) {
    random_seed_global = random_seed_global * 0x19660d + 0x3c6ef35f;
    fVar1 = (fVar2 - fVar1) * (float)(random_seed_global >> 0x10) * 1.5259022e-05 + fVar1;
  }
  else {
    fVar1 = 0.5;
  }
  if ((iVar4 != 0) && (0.0 < *(float *)(iVar4 + 0x410))) {
    fVar1 = fVar1 * *(float *)(iVar4 + 0x410);
  }
  if (param_2._0_1_ != '\0') {
    fVar1 = fVar1 * 1.5;
  }
  iVar4 = (int)ROUND(fVar1 * 30.0);
  if (iVar4 < 2) {
    iVar4 = 1;
  }
  return iVar4;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe, 0x415150..0x41525a): EDI is read
directly ([edi+0x10] etc.) with no push/pop of EDI in this function's own prologue/epilogue,
confirming it is a genuine caller-supplied register argument rather than a spilled local.
This rewrite folds Ghidra's own resolve-the-weapon-tag block (calling actor_get_threat_
weapon_object_index and indexing object_data/tag_instances by hand) into the already-rewritten
actor_get_threat_weapon_definition (0x40f970), which does the identical lookup.
#endif
