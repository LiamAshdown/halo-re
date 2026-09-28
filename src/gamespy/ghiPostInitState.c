// ghiPostInitState  (GameSpy SDK in halo.exe; no C existed)
// address 0x6223c0, size 375 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x6223c0..0x622536: with a post: counters cleared, the post callback taken, one
//   posting state per field (a failure closes the files opened so far and frees them: 0); the total body length by
//   encoding.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"

int ghiPostInitState(GHIConnection *connection)
{
    GHIPost *post = connection->post;
    int count;
    int i;

    if (post == 0) {
        return 0;
    }
    connection->objectsPosted = 0;
    connection->bytesPosted = 0;
    connection->totalBytes = 0;
    connection->postCallback = post->callback;
    connection->postCallbackParam = post->param;
    count = ArrayLength(post->data);
    connection->postingStates = ArrayNew(sizeof(GHIPostState), count, 0);
    if (connection->postingStates == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostState state;

        state.data = (GHIPostData *)ArrayNth(connection->post->data, i);
        state.pos = 0;
        state.file = 0;
        state.len = 0;
        if (!ghiPostStateInit(&state)) {
            for (i--; i >= 0; i--) {
                GHIPostState *opened = (GHIPostState *)ArrayNth(connection->postingStates, i);

                if (opened->data->type == 1) {
                    if (opened->file != 0) {
                        fclose(opened->file);
                    }
                    opened->file = 0;
                }
            }
            ArrayFree(connection->postingStates);
            connection->postingStates = 0;
            return 0;
        }
        ArrayAppend(connection->postingStates, &state);
    }
    if (connection->post == 0) {
        connection->totalBytes = 0;
        return 1;
    }
    if (connection->post->useMultipart != 0) {
        connection->totalBytes = ghiPostGetHasFilesContentLength(connection);
    } else {
        connection->totalBytes = ghiPostGetNoFilesContentLength(connection);
    }
    return 1;
}
