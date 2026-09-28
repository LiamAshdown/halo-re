// ghiPostCleanupState  (GameSpy SDK in halo.exe; no C existed)
// address 0x622540, size 162 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622540..0x6225e1: closes the disk files of every posting state and frees them;
//   an auto-free post is freed.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

void ghiPostCleanupState(GHIConnection *connection)
{
    GHIPost *post;

    if (connection->postingStates != 0) {
        int count = ArrayLength(connection->postingStates);
        int i;

        for (i = 0; i < count; i++) {
            GHIPostState *state = (GHIPostState *)ArrayNth(connection->postingStates, i);

            if (state->data->type == 1) {
                if (state->file != 0) {
                    fclose(state->file);
                }
                state->file = 0;
            }
        }
        ArrayFree(connection->postingStates);
        connection->postingStates = 0;
    }
    post = connection->post;
    if (post != 0 && post->autoFree != 0) {
        ArrayFree(post->data);
        free(post);
        connection->post = 0;
    }
}
