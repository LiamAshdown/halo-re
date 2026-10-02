// ui_handler_4a68f0  (not a Ghidra function; an entry of the UI function table at 0x006927d0, slot 0x00692b6c)
// address 0x4a68f0, size 71 bytes
// name confidence: 0.3 (named by address: the handler names live only in the widget tag definitions)
// rewrite confidence: 0.9
// evidence: only reachable through the UI function table (cdecl, the widget instance on the stack); first-boot
//   track: called while the main menu is built. objdump 0x4a68f0..0x4a6936: the value at widget +0x40 (int16)
//   becomes the child block's (+0x4c) index at +0x58, clamped to 0 when negative; with networking disabled
//   (0x007196ec) the record at (+0x34)->+0x2c gets +0x12 = 1 and +0x24 = 0.333 (0x3eaa7efa).
// blam-cc: stack -> widget (cdecl)
#include "tags.h"
#include "memory.h"
#include "math.h"
#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif

extern int32_t network_disabled_flag; // 0x007196ec

void ui_handler_4a68f0(uint8_t *widget)
{
    uint8_t *child = *(uint8_t **)(widget + 0x4c);
    uint8_t *record;

    *(int16_t *)(child + 0x58) = *(int16_t *)(widget + 0x40);
    if (*(int16_t *)(child + 0x58) < 0) {
        *(int16_t *)(child + 0x58) = 0;
    }
    if (network_disabled_flag != 0) {
        record = *(uint8_t **)(*(uint8_t **)(widget + 0x34) + 0x2c);
        record[0x12] = 1;
        *(uint32_t *)(record + 0x24) = 0x3eaa7efa;
    }
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
