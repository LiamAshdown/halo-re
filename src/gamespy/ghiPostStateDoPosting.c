// ghiPostStateDoPosting  (GameSpy SDK in halo.exe; no C existed)
// address 0x622810, size 421 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x622810..0x6229b4: stack state, ECX first, EDX connection: before the field
//   data its header goes out once ("name=" / "&name=", or the multipart part header with the boundary, and for files
//   filename and content type; an unknown type sends whatever the binary  stack held -- here nothing); then the data
//   by type.
// blam-cc: stack -> state, ECX -> first, EDX -> connection

#include "gamespy.h"

#include "ghttp.h"

int ghiPostStateDoPosting(GHIPostState *state, GHIConnection *connection, int first)
{
    char header[0x800];

    if (state->pos == -1) {
        GHIPostData *field = state->data;
        int result;

        state->pos = 0;
        header[0] = 0;
        if (connection->post->useMultipart == 0) {
            sprintf(header, first != 0 ? "%s=" : "&%s=", field->name);
        } else if (field->type == 0) {
            sprintf(header, "%sContent-Disposition: form-data; name=\"%s\"\r\n\r\n",
                first != 0 ? "--Qr4G823s23d---<<><><<<>--7d118e0536\r\n" : "\r\n--Qr4G823s23d---<<><><<<>--7d118e0536\r\n", field->name);
        } else if (field->type == 1 || field->type == 2) {
            const char *filename;
            const char *contentType;

            if (field->type == 1) {
                filename = field->data.fileDisk.reportFilename;
                contentType = field->data.fileDisk.contentType;
            } else {
                filename = field->data.fileMemory.reportFilename;
                contentType = field->data.fileMemory.contentType;
            }
            sprintf(header,
                "%sContent-Disposition: form-data; name=\"%s\"; filename=\"%s\"\r\nContent-Type: %s\r\n\r\n",
                first != 0 ? "--Qr4G823s23d---<<><><<<>--7d118e0536\r\n" : "\r\n--Qr4G823s23d---<<><><<<>--7d118e0536\r\n", field->name, filename, contentType);
        }
        result = ghiTrySendThenBuffer(connection, header, (int)strlen(header));
        if (result == 0) {
            return 0;
        }
        if (result == 2) {
            return 2;
        }
    }
    if (state->data->type == 0) {
        return ghiPostStringStateDoPosting(state, connection);
    }
    if (state->data->type == 1) {
        return ghiPostFileDiskStateDoPosting(state, connection);
    }
    return ghiPostFileMemoryStateDoPosting(state, connection);
}
