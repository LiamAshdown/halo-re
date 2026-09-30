// actor_resolve_flee_source_point  (Ghidra: actor_resolve_flee_source_point, renamed)
// address 0x4146c0, size 555 bytes
// name confidence: 0.4   rewrite confidence: 0.9 (VERIFIED against objdump 0x4146c0..0x4148ea (jump table decoded))
// evidence: phase-4 summary "resolves the source point/direction to flee away from, based on
// a caller-selected flee-reason code"; the one caller inside this address range
// (actor_update_flee_response @0x414250, src/ai/actor_update_flee_response.c) passes
// &actor.vocalization_unknown_3ec as the reason record and &actor.unknown_524 as the output
// vector, matching case 0/2's use of actor+0x518/+0x68c/+0x63c as direct-copy sources and
// confirming actor.flee_from_point (0x2b0, case 5) as the field this function is named for.
// register convention: reconstructed from objdump -d -M intel over 0x4146c0..0x4148ea
// (Ghidra lost every register argument here). reason record pointer in EAX, output vector
// pointer in EDI, actor_index on the stack (the only stack dword, at [esp+4] on entry).
// blam-cc: EAX -> reason, EDI -> out, stack -> actor_index
// UNSURE: prop.aim_marker_x/0x108/0x10c (case 1) are read here as a contiguous 3-float vector
// but are declared as three separate uint32_t in types/ai.h; reinterpreted via a real_point3d
// alias over their address rather than by declaring a new field name, since I could not find
// another reader of these three offsets to confirm a specific meaning.
// UNSURE: object_get_position and object_try_and_get's declared conventions elsewhere in this
// module already match this call site (EAX/ECX out+index for the former, ECX+stack for the
// latter). unit_get_primary_eye_marker_position (0x568f50, src/units) documents EAX as the
// forwarded object index, but this call site clearly loads the handle into ECX before the
// call (objdump: `mov ecx,[esp+0x1c]` immediately before `call 0x568f50`); declared to match
// what this call site actually does.
// TYPES-GAP: actor_flee_source_reason -- the ad hoc {int16 code; payload} record every caller
// builds inline (an actor field pair, or a stack scratch) rather than a named struct anywhere
// in the binary; listed in the module summary, not added to types/ai.h.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include "fn_ai.h"
#include "fn_math.h"

// actor_flee_source_reason now lives in types/ai.h (folded from this file).

extern data_array *actor_data; // 0x00880360
extern data_array *prop_data;  // 0x008802c0

extern void * datum_get(datum_index handle, data_array *array);                          // 0x4d0680
extern void *object_try_and_get(datum_index object_index, int32_t kind);        // 0x4f6ec0
extern void object_get_position(real_point3d *out_position, datum_index object_index);           // 0x4f6900, EAX->out, ECX->object_index
extern void unit_get_primary_eye_marker_position(datum_index object_index, real_point3d *out); // 0x568f50, ECX->object_index, ESI->out (see UNSURE)


// blam-cc: EAX -> reason, EDI -> out, stack -> actor_index
// Fills *out with a direction (or, for a handful of reason codes, an absolute point copied
// verbatim) built from one of seven sources selected by reason->code, and returns whether the
// result is usable: reason codes 2 (only when it reads the direct facing/lean vectors) and 4
// return true unconditionally with no normalization; every other code subtracts the actor's
// aim_origin, normalizes the result in place, and returns true only if the pre-normalize
// length was greater than zero.
uint8_t actor_resolve_flee_source_point(actor_flee_source_reason *reason, real_vector3d *out, datum_index actor_index)
{
    actor *self;
    prop *target_prop;
    real_point3d *target_point;
    object *target_object;
    float length;

    self = (actor *)((uint8_t *)actor_data->data + (actor_index & 0xffff) * sizeof(actor));

    switch (reason->code) {
    case 0:
        if (self->desired_direction_valid == 0) {
            return 0;
        }
        *out = *(real_vector3d *)&self->desired_direction;
        break;

    case 1: {
        target_prop = (prop *)datum_get(reason->payload.handle, prop_data);
        if (target_prop == 0) {
            return 0;
        }
        // UNSURE: see file header -- reinterpreting three unnamed uint32_t as a point.
        target_point = (real_point3d *)&target_prop->aim_marker_x;
        out->i = target_point->x - self->aim_origin.x;
        out->j = target_point->y - self->aim_origin.y;
        out->k = target_point->z - self->aim_origin.z;
        break;
    }

    case 2:
        if (self->unknown_5f2 == 2) {
            *out = self->unknown_68c;
            return 1;
        }
        if (self->unknown_628 != 0) {
            *out = *(real_vector3d *)&self->unknown_63c;
            return 1;
        }
        if (self->target_unit_index == (datum_index)k_datum_index_none) {
            return 0;
        }
        target_prop = &((prop *)prop_data->data)[self->target_unit_index & 0xffff];
        out->i = target_prop->aim_offset.x - self->aim_origin.x;
        out->j = target_prop->aim_offset.y - self->aim_origin.y;
        out->k = target_prop->aim_offset.z - self->aim_origin.z;
        break;

    case 3:
        out->i = reason->payload.point.x - self->aim_origin.x;
        out->j = reason->payload.point.y - self->aim_origin.y;
        out->k = reason->payload.point.z - self->aim_origin.z;
        break;

    case 4:
        *out = *(real_vector3d *)&reason->payload.point;
        return 1;

    case 5:
        if (self->danger_type < 1) {
            return 0;
        }
        out->i = self->flee_from_point.x - self->aim_origin.x;
        out->j = self->flee_from_point.y - self->aim_origin.y;
        out->k = self->flee_from_point.z - self->aim_origin.z;
        break;

    case 6: {
        real_point3d source_position;

        target_object = object_try_and_get(reason->payload.handle, 0xffffffff);
        if (target_object == 0) {
            return 0;
        }
        if ((1 << (target_object->type & 0x1f) & 3) != 0) {
            unit_get_primary_eye_marker_position(reason->payload.handle, &source_position);
        } else {
            object_get_position(&source_position, reason->payload.handle);
        }
        out->i = source_position.x - self->aim_origin.x;
        out->j = source_position.y - self->aim_origin.y;
        out->k = source_position.z - self->aim_origin.z;
        break;
    }

    default:
        return 0;
    }

    length = vector3d_normalize_with_length(out);
    return (0.0f < length) ? 1 : 0; // 0x00672ac0 is the compiled-in 0.0f constant
}

#if 0
Original Ghidra decompilation (0x4146c0):

undefined4 FUN_004146c0(uint param_1)

{
  undefined2 *in_EAX;
  int iVar1;
  int iVar2;
  int iVar3;
  float *pfVar4;
  float *unaff_EDI;
  float10 fVar5;
  float local_c;
  float local_8;
  float local_4;

  iVar3 = (param_1 & 0xffff) * 0x724 + *(int *)(DAT_00880360 + 0x34);
  switch(*in_EAX) {
  case 0:
    if (*(char *)(iVar3 + 0x504) == '\0') {
      return 0;
    }
    *unaff_EDI = *(float *)(iVar3 + 0x518);
    unaff_EDI[1] = *(float *)(iVar3 + 0x51c);
    unaff_EDI[2] = *(float *)(iVar3 + 0x520);
    goto LAB_004148c8;
  case 1:
    iVar2 = datum_get();
    if (iVar2 == 0) {
      return 0;
    }
    *unaff_EDI = *(float *)(iVar2 + 0x104) - *(float *)(iVar3 + 0x120);
    unaff_EDI[1] = *(float *)(iVar2 + 0x108) - *(float *)(iVar3 + 0x124);
    local_4 = *(float *)(iVar2 + 0x10c);
    break;
  case 2:
    if (*(short *)(iVar3 + 0x5f2) == 2) {
      pfVar4 = (float *)(iVar3 + 0x68c);
LAB_00414732:
      *unaff_EDI = *pfVar4;
      unaff_EDI[1] = pfVar4[1];
      unaff_EDI[2] = pfVar4[2];
      return 1;
    }
    if (*(char *)(iVar3 + 0x628) != '\0') {
      pfVar4 = (float *)(iVar3 + 0x63c);
      goto LAB_00414732;
    }
    if (*(uint *)(iVar3 + 0x270) == 0xffffffff) {
      return 0;
    }
    iVar2 = *(int *)(DAT_008802c0 + 0x34);
    iVar1 = (*(uint *)(iVar3 + 0x270) & 0xffff) * 0x138;
    *unaff_EDI = *(float *)(iVar1 + 200 + iVar2) - *(float *)(iVar3 + 0x120);
    unaff_EDI[1] = *(float *)(iVar1 + 0xcc + iVar2) - *(float *)(iVar3 + 0x124);
    local_4 = *(float *)(iVar1 + iVar2 + 0xd0);
    break;
  case 3:
    *unaff_EDI = *(float *)(in_EAX + 2) - *(float *)(iVar3 + 0x120);
    unaff_EDI[1] = *(float *)(in_EAX + 4) - *(float *)(iVar3 + 0x124);
    local_4 = *(float *)(in_EAX + 6);
    break;
  case 4:
    *unaff_EDI = *(float *)(in_EAX + 2);
    unaff_EDI[1] = *(float *)(in_EAX + 4);
    unaff_EDI[2] = *(float *)(in_EAX + 6);
    return 1;
  case 5:
    if (*(short *)(iVar3 + 0x280) < 1) {
      return 0;
    }
    *unaff_EDI = *(float *)(iVar3 + 0x2b0) - *(float *)(iVar3 + 0x120);
    unaff_EDI[1] = *(float *)(iVar3 + 0x2b4) - *(float *)(iVar3 + 0x124);
    local_4 = *(float *)(iVar3 + 0x2b8);
    break;
  case 6:
    iVar2 = object_try_and_get(0xffffffff);
    if (iVar2 == 0) {
      return 0;
    }
    if ((1 << (*(byte *)(iVar2 + 0xb4) & 0x1f) & 3U) == 0) {
      object_get_position();
    }
    else {
      FUN_00568f50();
    }
    *unaff_EDI = local_c - *(float *)(iVar3 + 0x120);
    unaff_EDI[1] = local_8 - *(float *)(iVar3 + 0x124);
    break;
  default:
    goto switchD_004146ef_default;
  }
  unaff_EDI[2] = local_4 - *(float *)(iVar3 + 0x128);
LAB_004148c8:
  fVar5 = (float10)vector3d_normalize_with_length();
  if ((float10)0.0 < fVar5) {
    return 1;
  }
switchD_004146ef_default:
  return 0;
}

Disassembly cross-check (objdump -d -M intel bin/halo.exe, 0x4146c0..0x4148ea), used to recover the
register arguments Ghidra dropped:

004146c0: mov ecx,ds:0x880360 / sub esp,0xc / push ebx / push ebp
004146cb: mov ebp,[esp+0x18]           ; ebp = stack actor_index (the only stack dword arg)
004146cf: and ebp,0xffff / imul ebp,ebp,0x724
004146db: push esi / mov esi,[ecx+0x34]
004146df: movsx ecx,word ptr [eax]     ; eax = reason record pointer (register arg)
004146e2: add ebp,esi                  ; ebp = actor base
004146e4: xor bl,bl
004146e6: cmp ecx,0x6 / ja default
004146ef: jmp [ecx*4+0x4148ec]         ; table: 4146f6 4147b1 414722 4147ef 41480e 41482c 41485f
...
004147b1: mov edx,[eax+4] / mov esi,ds:0x8802c0 / call 0x4d0680      ; datum_get(edx=handle, esi=prop_data)
...
00414876: mov cl,[eax+0xb4]            ; object.type, eax = object_try_and_get's result
00414888: mov ecx,[esp+0x1c] / lea esi,[esp+0xc] / call 0x568f50    ; ECX=handle, ESI=out
00414897: lea eax,[esp+0xc] / mov ecx,esi / call 0x4f6900           ; EAX=out, ECX=handle
004148c8: mov ecx,edi / call 0x401990                                ; vector3d_normalize_with_length(ecx=out)
004148cf: fcomp ds:0x672ac0 / fnstsw ax / test ah,0x41 / je 0x414821 ; length > 0.0f -> success
#endif
