// rasterizer_lens_flare_occlusion_query_get_result  (Ghidra: rasterizer_lens_flare_occlusion_query_get_result, already named)
// address 0x537b40, size 104 bytes
// name confidence: 0.55   rewrite confidence: 0.85 (REWRITTEN from objdump 0x537b40..0x537bad: result local starts at -1 (the draft reused a union holding the query pointer, so a failed GetData returned pointer bits = a huge visible count); disabled toggle -> 1; no query or slot >= 0x400 -> 2; GetData(query, &value, 4, 1) retried while S_FALSE)
// evidence: functions.md summary ("Polls the occlusion query for one lens-flare slot until a
//   result is available, returning the query's visible-pixel-count result"); GetData(pData,
//   dwSize=4, dwGetDataFlags) at query vtable +0x1c matches IDirect3DQuery9, looping while it
//   returns S_FALSE (1, "not ready yet").
// register convention: ESI -> slot_index.
// blam-cc: ESI -> slot_index
// UNSURE: the returned pointer on failure (1 = queries disabled, 2 = no query for this slot) is a
//   sentinel integer cast to a pointer, not a real int*; preserved as Ghidra shows it.

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern uint8_t console_debug_toggle_689424; // 0x00689424
extern void *lens_flare_occlusion_queries[k_lens_flare_occlusion_queries]; // 0x006e1dc8

typedef int32_t (__stdcall *d3d_query_get_data_fn)(void *query, void *data, uint32_t size, uint32_t flags);

// Polls the occlusion query for one lens-flare slot until a result is available, returning the
// query's visible-pixel-count result. UNSURE/note: the original overwrites its own "query
// pointer" stack slot in place with the 4-byte GetData result and returns that slot's value
// reinterpreted as a pointer, rather than returning through a separate out-parameter; the same
// in-place reuse is reproduced here via `slot`.
// the result is a pixel count (1 when queries are disabled, 2 when the slot has no query), not a pointer; its
// caller multiplies it by 0xff (0x513821)
int32_t rasterizer_lens_flare_occlusion_query_get_result(int32_t slot_index)
{
    int32_t value = -1; // 0x537b48: the result local starts at -1 and is only written by a successful GetData
    void *query;
    int32_t hr;

    if (console_debug_toggle_689424 == 0) {
        return 1;
    }

    query = lens_flare_occlusion_queries[slot_index]; // 0x537b51 reads the slot before the bounds check
    if (query != 0 && slot_index < k_lens_flare_occlusion_queries) {
        void **vt = *(void ***)query;
        d3d_query_get_data_fn get_data = (d3d_query_get_data_fn)vt[0x1c / 4];

        hr = get_data(query, &value, 4, 1);
        while (hr == 1) { // S_FALSE: not ready yet; the slot is re-read each time (0x537b80)
            query = lens_flare_occlusion_queries[slot_index];
            hr = get_data(query, &value, 4, 1);
        }
        return value;
    }
    return 2;
}

#if 0
Original Ghidra decompilation (0x537b40):

int * rasterizer_lens_flare_occlusion_query_get_result(void)

{
  int iVar1;
  int unaff_ESI;
  int *piStack_14;
  undefined4 *puStack_10;
  undefined4 uStack_c;
  undefined4 uStack_8;
  undefined4 local_4;

  local_4 = 0xffffffff;
  if (DAT_00689424 == '\0') {
    return (int *)0x1;
  }
  piStack_14 = (int *)(&DAT_006e1dc8)[unaff_ESI];
  if ((piStack_14 != (int *)0x0) && (unaff_ESI < 0x400)) {
    uStack_8 = 1;
    uStack_c = 4;
    puStack_10 = &local_4;
    iVar1 = (**(code **)(*piStack_14 + 0x1c))();
    while (iVar1 == 1) {
      iVar1 = (**(code **)(*(int *)(&DAT_006e1dc8)[unaff_ESI] + 0x1c))
                        ((int *)(&DAT_006e1dc8)[unaff_ESI],&piStack_14,4,1);
    }
    return piStack_14;
  }
  return (int *)0x2;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
