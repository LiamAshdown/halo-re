// network_connection_endpoint_set  (Ghidra: FUN_004d8c50; renamed, no prior name)
// address 0x4d8c50, size 149 bytes
// name confidence: 0.4   rewrite confidence: 0.55
// evidence: out/phase4/networking_functions.md summary ("Sets a connection object's remote
// endpoint address fields and (re)allocates its per-endpoint control block").
// register convention: source endpoint data in ESI (unaff_ESI), connection/client object in
// EDI (unaff_EDI). // blam-cc: ESI -> source, EDI -> connection
// UNSURE: both operands are register-only in Ghidra's output (no visible callers in this
// batch), so the exact caller-side setup is unconfirmed; modeled with explicit parameters per
// the task's register-convention rule.
// This writes offsets 0xab4..0xadb of network_client_globals. The review pass folded that
// region into types/networking.h as network_client_globals::connection, a
// network_connection_endpoint. It used to be declared here as
// a local, file-scoped struct covering just the 0x28 bytes this function itself touches,
// immediately preceding the +0xadc channel pointer; not added to types/networking.h.
// network_connection_initiate.c (0x4d8cf0, same task batch) confirms the first five dwords are
// exactly an s_network_address -- it writes the size/port sub-fields at +0x10/+0x12 of this
// struct individually -- so the struct below embeds s_network_address rather than a flat array;
// the same typedef is duplicated there.

#include "win32.h"
#include "tags.h"
#include "memory.h"
#include "math.h"
#include "game.h"
#include "networking.h"



#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
int32_t network_connection_endpoint_set(const uint32_t *source, network_client_globals *connection)
    // blam-cc: ESI -> source, EDI -> connection
{
    network_connection_endpoint *endpoint;
    void *control_block;
    uint32_t *raw;
    int32_t i;

    endpoint = &connection->connection;
    raw = (uint32_t *)endpoint;
    endpoint->ready = 0;
    if (endpoint->control_block != 0) {
        GlobalFree(endpoint->control_block);
        endpoint->control_block = 0;
    }
    for (i = 0; i < 6; i = i + 1) {
        raw[i] = 0;
    }
    endpoint->last_send_ms = 0;
    endpoint->message_count = 0;
    endpoint->retry_count = 0;
    endpoint->current_ping_ms = 0;
    endpoint->ready = 0;
    endpoint->unknown_23 = 0;
    endpoint->control_block = 0;
    for (i = 0; i < 6; i = i + 1) {
        raw[i] = source[i];
    }
    endpoint->ready = 1;
    control_block = GlobalAlloc(0, 0x264);
    ((uint32_t *)control_block)[1] = 0;
    ((uint32_t *)control_block)[0] = 0;
    endpoint->control_block = control_block;
    return 1;
}

#if 0
Original Ghidra decompilation (0x4d8c50):

undefined4 FUN_004d8c50(void)

{
  undefined4 *puVar1;
  undefined4 *unaff_ESI;
  int unaff_EDI;

  *(undefined1 *)(unaff_EDI + 0xad6) = 0;
  if (*(HGLOBAL *)(unaff_EDI + 0xad8) != (HGLOBAL)0x0) {
    GlobalFree(*(HGLOBAL *)(unaff_EDI + 0xad8));
    *(undefined4 *)(unaff_EDI + 0xad8) = 0;
  }
  *(undefined4 *)(unaff_EDI + 0xab4) = 0;
  *(undefined4 *)(unaff_EDI + 0xab8) = 0;
  *(undefined4 *)(unaff_EDI + 0xabc) = 0;
  *(undefined4 *)(unaff_EDI + 0xac0) = 0;
  *(undefined4 *)(unaff_EDI + 0xac4) = 0;
  *(undefined4 *)(unaff_EDI + 0xac8) = 0;
  *(undefined4 *)(unaff_EDI + 0xacc) = 0;
  *(undefined4 *)(unaff_EDI + 0xad0) = 0;
  *(undefined4 *)(unaff_EDI + 0xad4) = 0;
  *(undefined4 *)(unaff_EDI + 0xad8) = 0;
  *(undefined4 *)(unaff_EDI + 0xab4) = *unaff_ESI;
  *(undefined4 *)(unaff_EDI + 0xab8) = unaff_ESI[1];
  *(undefined4 *)(unaff_EDI + 0xabc) = unaff_ESI[2];
  *(undefined4 *)(unaff_EDI + 0xac0) = unaff_ESI[3];
  *(undefined4 *)(unaff_EDI + 0xac4) = unaff_ESI[4];
  *(undefined4 *)(unaff_EDI + 0xac8) = unaff_ESI[5];
  *(undefined1 *)(unaff_EDI + 0xad6) = 1;
  puVar1 = GlobalAlloc(0,0x264);
  puVar1[1] = 0;
  *puVar1 = 0;
  *(undefined4 **)(unaff_EDI + 0xad8) = puVar1;
  return 1;
}
#endif
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
