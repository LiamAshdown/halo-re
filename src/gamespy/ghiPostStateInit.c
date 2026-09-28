// ghiPostStateInit  (GameSpy SDK in halo.exe; no C existed)
// address 0x622340, size 113 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622340..0x6223b0: ESI state: not started (-1); a disk file is opened "rb" and
//   measured (0 when either fails); strings and memory files need nothing; other types 0.
// blam-cc: ESI -> state

#include "gamespy.h"

#include "ghttp.h"

int ghiPostStateInit(GHIPostState *state)
{
    GHIPostData *field = state->data;

    state->pos = -1;
    if (field->type == 0) {
        return 1;
    }
    if (field->type == 1) {
        state->file = fopen(field->data.fileDisk.filename, "rb");
        if (state->file == 0 || fseek(state->file, 0, SEEK_END) != 0) {
            return 0;
        }
        state->len = ftell(state->file);
        if (state->len == -1) {
            return 0;
        }
        rewind(state->file);
        return 1;
    }
    if (field->type == 2) {
        return 1;
    }
    return 0;
}
