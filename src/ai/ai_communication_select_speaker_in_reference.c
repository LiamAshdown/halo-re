// ai_communication_select_speaker_in_reference  (Ghidra: ai_communication_select_speaker_in_reference; named for this rewrite)
// address 0x42ff80, size 326 bytes
// name confidence: 0.45   rewrite confidence: 0.85
// VERIFIED 2026-09-27 (static loop) against objdump 0x42ff80..0x4300c5: marker position = node_transform +0x60, the
// 10-argument push order, strict best-score test, -1 on an empty or invalid reference.
// evidence: phase-4 summary ("iterates the actors of a squad (or reference), scoring each as
// a candidate speaker and returning the best match"). The iterator pair it drives
// (ai_reference_actor_iterator_new / ai_reference_actor_iterator_next) is the ai-reference actor iterator, and the scorer is
// ai_communication_rate_speaker (0x42fb90).
// register convention: EAX -> reference, EDI -> object_a, EBX -> object_b. Recovered from
// the disassembly: `mov esi,eax` then `push esi` into ai_reference_actor_iterator_new at 0x430018, and
// `push edi` / `push ebx` as the first argument of the two
// object_get_node_local_transform calls at 0x42ffb3 / 0x42ffee. All seven stack arguments
// are forwarded verbatim to ai_communication_rate_speaker as its param_4..param_10.
// blam-cc: EAX -> reference, EDI -> object_a, EBX -> object_b, stack -> radius,
//          allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags
//
// UNSURE: Ghidra's own decompile shows this function as taking no arguments at all, shows
// both object_get_node_local_transform calls with an empty argument list, and drops the
// iterator entirely. Everything below comes from the disassembly (objdump -d -M intel
// --start-address=0x42ff80 --stop-address=0x4300c6 bin/halo.exe).
// UNSURE: the current actor's index is read from the iterator block at +0x10 here, while
// ai_communication_select_speaker_by_team reads its own iterator's at +0x14; the two
// iterators are different types (ai-reference vs actor_iterator_state), so this is not a
// contradiction, but neither offset is confirmed by a constructor.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "objects.h"
#include "ai.h"
#include <stdint.h>

extern char ai_marker_name_a[]; // 0x0066bfa0

extern int32_t object_get_node_local_transform(datum_index object_index, char *marker_name,
                                               object_marker *marker, uint32_t flags); // 0x4f6080
extern float ai_communication_rate_speaker(datum_index actor_index, datum_index object_b,
                                           real_point3d *position_b, float radius,
                                           int16_t allow_unreachable, uint32_t fade_limit,
                                           uint32_t line_class, uint32_t line_id,
                                           int16_t seat_filter, uint8_t flags,
                                           real_point3d *position_a, datum_index object_a); // 0x42fb90

extern void ai_reference_actor_iterator_new(uint32_t reference,
                                            ai_reference_actor_iterator *iterator); // 0x432650, blam-cc: ECX -> iterator, stack -> reference
extern void *ai_reference_actor_iterator_next(ai_reference_actor_iterator *iterator); // 0x4326d0, blam-cc: EDX -> iterator

// blam-cc: EAX -> reference, EDI -> object_a, EBX -> object_b, stack -> radius,
//          allow_unreachable, fade_limit, line_class, line_id, seat_filter, flags
// Resolves each subject object's marker position once, then walks every actor the packed
// ai reference names, scores it with ai_communication_rate_speaker, and returns the handle
// of the highest-scoring actor (none when the reference is invalid or no actor scores above
// 0.0).
datum_index ai_communication_select_speaker_in_reference(float radius, int16_t allow_unreachable,
                                                         uint32_t fade_limit, uint32_t line_class,
                                                         uint32_t line_id, int16_t seat_filter,
                                                         uint8_t flags,
                                                         uint32_t reference /* EAX */,
                                                         datum_index object_a /* EDI */,
                                                         datum_index object_b /* EBX */)
{
    object_marker marker;
    real_point3d position_a;
    real_point3d position_b;
    ai_reference_actor_iterator iterator;
    datum_index best;
    float best_score;
    datum_index actor_index;
    float score;

    best = (datum_index)k_datum_index_none;
    best_score = 0.0f;

    if (reference == (uint32_t)k_datum_index_none) {
        return (datum_index)k_datum_index_none;
    }

    if (object_a != (datum_index)k_datum_index_none) {
        object_get_node_local_transform(object_a, ai_marker_name_a, &marker, 1);
        position_a = marker.node_transform.position;
    }
    if (object_b != (datum_index)k_datum_index_none) {
        object_get_node_local_transform(object_b, ai_marker_name_a, &marker, 1);
        position_b = marker.node_transform.position;
    }

    ai_reference_actor_iterator_new(reference, &iterator);
    if (ai_reference_actor_iterator_next(&iterator) == 0) {
        return (datum_index)k_datum_index_none;
    }
    do {
        actor_index = iterator.actor_index;
        score = ai_communication_rate_speaker(actor_index, object_b, &position_b, radius,
                                              allow_unreachable, fade_limit, line_class, line_id,
                                              seat_filter, flags, &position_a, object_a);
        if (best_score < score) {
            best_score = score;
            best = actor_index;
        }
    } while (ai_reference_actor_iterator_next(&iterator) != 0);

    return best;
}

#if 0
Original Ghidra decompilation (0x42ff80):

undefined4 FUN_0042ff80(void)

{
  int in_EAX;
  int iVar1;
  int unaff_EBX;
  int unaff_EDI;
  float10 fVar2;
  float local_a4;
  undefined4 local_a0;
  undefined4 local_74;

  local_a0 = 0xffffffff;
  local_a4 = 0.0;
  if (in_EAX != -1) {
    if (unaff_EDI != -1) {
      object_get_node_local_transform();
    }
    if (unaff_EBX != -1) {
      object_get_node_local_transform();
    }
    FUN_00432650();
    iVar1 = FUN_004326d0();
    if (iVar1 != 0) {
      do {
        fVar2 = (float10)FUN_0042fb90(local_74);
        if ((float10)local_a4 < fVar2) {
          local_a4 = (float)fVar2;
          local_a0 = local_74;
        }
        iVar1 = FUN_004326d0();
      } while (iVar1 != 0);
      return local_a0;
    }
  }
  return 0xffffffff;
}

Disassembly excerpt establishing the register roles and the argument forwarding
(objdump -d -M intel, retail halo.exe 1.0.10):

  42ff88: mov    esi,eax                  ; EAX -> reference
  42ffb3: push   edi                      ; EDI -> object_a
  42ffb4: call   0x4f6080                 ; object_get_node_local_transform
  42ffee: push   ebx                      ; EBX -> object_b
  42ffef: call   0x4f6080
  430018: push   esi                      ; reference
  430019: lea    ecx,[esp+0x2c]           ; ECX -> iterator
  43001d: call   0x432650
  430025: lea    edx,[esp+0x28]           ; EDX -> iterator
  430029: call   0x4326d0
  430055: mov    esi,DWORD PTR [esp+0x38] ; iterator + 0x10 = current actor index
  430074: push   ebp                      ; the 7 stack arguments, in reverse
  43007a: push   ebx
  43007b: push   esi
  43007c: lea    eax,[esp+0x44]           ; EAX -> position_a
  430080: mov    ecx,edi                  ; ECX -> object_a
  430082: call   0x42fb90
#endif
