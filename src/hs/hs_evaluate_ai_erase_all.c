// hs_evaluate_ai_erase_all  (not a Ghidra function; the evaluate handler of hs "ai_erase_all" (-> void))
// address 0x47d330, size 33 bytes
// name confidence: 0.9  rewrite confidence: 0.9
// evidence: hs_function_definitions 0x688b58[i] evaluate (+0xc) 0x47d330, only reachable through that pointer.
//   Campaign track: 15 uses across the campaign scripts (not a10); it had no C and would have trapped
//   (unlisted callback).
// objdump 0x47d330: ai_release_actors_filtered(EAX -1, EDI -1, stack -1, BL 0) -- every actor; returns 0.
// blam-cc: stack -> function_index, thread_index, first (cdecl)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "hs.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern void hs_thread_return(int32_t value, uint32_t thread_index); // 0x48a640
extern void ai_release_actors_filtered(datum_index encounter_index, int32_t platoon_index, int32_t squad_index,
    uint8_t is_dead); // 0x42ab00, EAX, EDI, stack, BL

void hs_evaluate_ai_erase_all(int16_t function_index, uint32_t thread_index, char first)
{
    (void)function_index;
    (void)first;
    ai_release_actors_filtered(0xffffffff, -1, -1, 0);
    hs_thread_return(0, thread_index);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
