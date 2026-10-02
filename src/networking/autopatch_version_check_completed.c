// autopatch_version_check_completed  (not a Ghidra function; no C existed)
// address 0x5777d0, size 118 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x5777d0..0x577845: the ptCheckForPatch callback (available, mandatory, version
//   name, file id, download url, param): with a patch and a url, keeps the url (0xff chars, 0x007228d8) and the
//   version name (0x3f chars, 0x007229d8), the file id (0x007228d4) and state 3; otherwise clears them and sets state
//   2.
// blam-cc: cdecl (the autopatch check callback)

#include "tags.h"

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern char autopatch_update_url[0x100];      // 0x007228d8
extern char autopatch_update_version[0x100];  // 0x007229d8
extern int32_t autopatch_update_file_id;      // 0x007228d4
extern int32_t autopatch_update_check_state;  // 0x0069fe04
extern char *strncpy(char *dest, const char *source, unsigned int count);

void autopatch_version_check_completed(int32_t available, int32_t mandatory, const char *version_name, int32_t file_id,
    const char *download_url, void *param)
{
    (void)mandatory;
    (void)param;
    if (available == 0 || download_url[0] == 0) {
        autopatch_update_file_id = 0;
        autopatch_update_url[0] = 0;
        autopatch_update_version[0] = 0;
        autopatch_update_check_state = 2;
        return;
    }
    strncpy(autopatch_update_url, download_url, 0xff);
    autopatch_update_url[0xff] = 0;
    autopatch_update_check_state = 3;
    autopatch_update_file_id = file_id;
    strncpy(autopatch_update_version, version_name, 0x3f);
    autopatch_update_version[0x3f] = 0;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
