// shell_display_adapter_enumerate_callback  (not a Ghidra function; passed to DirectDrawEnumerateExA)
// address 0x57d370, size 154 bytes
// name confidence: 0.7  rewrite confidence: 0.9
// evidence: shell_detect_hardware_specs 0x57d880 passes 0x57d370 to DirectDrawEnumerateExA
//   (DDENUM_ATTACHEDSECONDARYDEVICES) and then walks display_adapters[0..display_adapter_count).
//   First-boot track: the standalone exe reached this callback and trapped (no C existed).
// Rewritten from objdump 0x57d370..0x57d40b (ret 0x14: the five-argument LPDDENUMCALLBACKEXA, __stdcall):
//   - entries past 0 copy the 16-byte GUID (entry 0 is the primary display and keeps its zero GUID)
//   - the name copied is the THIRD argument, lpDriverName ([esp+0x10] after push edi), not the description;
//     it is left empty when strlen >= 0x1f
//   - the count is incremented and stored either way; the result is (count != 10), stopping the enumeration
//     at k_shell_maximum_display_adapters
// blam-cc: __stdcall, stack -> guid, description, driver_name, context, monitor

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "rasterizer.h"
#include "shell.h"
#include "fn_shell.h"

extern uint32_t display_adapter_count;                                         // 0x00722bb4
extern shell_display_adapter display_adapters[k_shell_maximum_display_adapters]; // 0x006efdc0

int32_t __stdcall shell_display_adapter_enumerate_callback(void *guid, char *description, char *driver_name,
                                                           void *context, void *monitor)
{
    uint32_t index = display_adapter_count;
    shell_display_adapter *adapter = &display_adapters[index];
    const char *s;
    int32_t i;

    if (index != 0) {
        const uint32_t *source = (const uint32_t *)guid;
        adapter->guid[0] = source[0];
        adapter->guid[1] = source[1];
        adapter->guid[2] = source[2];
        adapter->guid[3] = source[3];
    }
    for (s = driver_name; *s; s++) {
    }
    if ((uint32_t)(s - driver_name) < 0x1f) {
        i = 0;
        do {
            adapter->driver_name[i] = driver_name[i];
        } while (driver_name[i++] != 0);
    } else {
        adapter->driver_name[0] = 0;
    }
    display_adapter_count = index + 1;
    return index + 1 != k_shell_maximum_display_adapters;
}
