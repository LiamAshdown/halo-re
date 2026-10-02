// path_build_full  (not a Ghidra function; builds the full path of a file reference)
// address 0x5560d0, size 155 bytes
// name confidence: 0.6   rewrite confidence: 0.9
// WRITTEN 2026-09-28 from objdump 0x5560d0..0x55616a: location 2 copies the path as is; a positive location prefixes
//   the root template (0x0069fa50, "?:\\" with the drive patched in) and appends at most 0xfb characters (strncat);
//   otherwise a path that is not rooted (second character neither '\\' nor ':') gets ".\\" (0x00671f94) in front.
// blam-cc: EAX source, EDX destination, CX location

#include "tags.h"
#include "memory.h"
#include "math.h"
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char file_root_template[4]; // 0x0069fa50

void path_build_full(char *source, char *destination, int16_t location)
{
    destination[0] = 0;
    if (location == 2) {
        strcpy(destination, source);
        return;
    }
    if (location > 0) {
        strcpy(destination, file_root_template);
        strncat(destination, source, 0xfb);
        return;
    }
    if (source[1] != '\\' && source[1] != ':') {
        memcpy(destination, ".\\", 3); // 0x00671f94
    }
    strcat(destination, source);
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
