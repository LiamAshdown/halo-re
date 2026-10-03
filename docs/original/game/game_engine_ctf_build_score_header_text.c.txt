// game_engine_ctf_build_score_header_text  (not a Ghidra function; the ctf game engine definition's +0x58 slot (build_score_header_text); no C existed, so that
//   stored pointer trapped as unlisted_469a30)
// address 0x469a30, size 120 bytes
// name confidence: 0.6   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x469a30..0x469aa7: copies string 0x9a of ui\multiplayer_game_text (inlined
//   lookup: missing -> L"<missing string>", no tag -> L"") into the buffer and returns it.
// blam-cc: cdecl (called through the engine definition)

#include "tags.h"
#include "memory.h"
#include "math.h"
#include "cache.h"
#include "game.h"
#include <wchar.h>
#include <string.h>

#ifdef __cplusplus
extern "C" { /* HALO_CXX_LINKAGE */
#endif
extern datum_index tag_lookup(tag_group group, char *path); // 0x442550, blam-cc: EDI group
extern tag_instance *tag_instances; // 0x0087bc14
extern uint16_t missing_string_text[]; // 0x00671fac

static uint16_t *multiplayer_text(int16_t index)
{
    datum_index list = tag_lookup(0x75737472, (char *)"ui\\multiplayer_game_text"); // 'ustr', 0x00660c38

    if (list == 0xffffffff) {
        return (uint16_t *)L""; // 0x00660c34
    }
    {
        uint8_t *strings = (uint8_t *)tag_instances[list & 0xffff].data;

        if (*(int32_t *)strings > index) {
            uint8_t *element = *(uint8_t **)(strings + 4) + index * 0x14;
            uint32_t size = *(uint32_t *)element;

            if ((int32_t)size > 0) {
                uint16_t *text = *(uint16_t **)(element + 0xc);

                text[(size >> 1) - 1] = 0;
                return text;
            }
        }
        return missing_string_text;
    }
}

wchar_t *game_engine_ctf_build_score_header_text(wchar_t *buffer)
{
    wcscpy(buffer, (const wchar_t *)multiplayer_text(0x9a));
    return buffer;
}
#ifdef __cplusplus
} /* HALO_CXX_LINKAGE */
#endif
