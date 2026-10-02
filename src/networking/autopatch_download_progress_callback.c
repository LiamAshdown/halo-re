// autopatch_download_progress_callback  (Ghidra: LAB_00576a70, not a function; no C existed)
// address 0x576a70, size 86 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x576a70..0x576ac5: the ghttp progress callback autopatch_download_start passes:
//   finds the download slot holding the request (0x6ef93c, two 0x14-byte slots); a request no slot owns is cancelled
//   (ghttpCancelRequest 0x61c030). Otherwise the slot's state becomes 3 while ghttp is receiving the status, headers
//   or file (ghttp states 5..7), else 2.
// blam-cc: cdecl (a ghttp progress callback: request, state, buffer, len, bytes received, total size, param)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern autopatch_download_slot autopatch_download_slots[2]; // 0x006ef93c
extern void ghttpCancelRequest(int32_t request); // 0x61c030

void autopatch_download_progress_callback(int32_t request, int32_t state, const char *buffer, int32_t buffer_length,
    int32_t bytes_received, int32_t total_size, void *param)
{
    int32_t i;

    (void)buffer;
    (void)buffer_length;
    (void)bytes_received;
    (void)total_size;
    (void)param;
    for (i = 0; i < 2; i++) {
        if (autopatch_download_slots[i].request_id == request) {
            break;
        }
    }
    if (i == 2) {
        ghttpCancelRequest(request);
        return;
    }
    if (state >= 5 && state <= 7) {
        autopatch_download_slots[i].state = 3;
    } else {
        autopatch_download_slots[i].state = 2;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
