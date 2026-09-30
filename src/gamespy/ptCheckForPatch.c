// ptCheckForPatch  (GameSpy SDK in halo.exe; no C existed)
// address 0x61c260, size 151 bytes
// name confidence: 0.8   rewrite confidence: 0.85
// WRITTEN 2026-09-28 from objdump 0x61c260..0x61c2f6: (product id, version unique id, distribution id, callback,
//   blocking, param): asks hpcup.bungie.net/motd/vercheck.asp (no-cache) through ghttpGetEx; the static completion
//   0x61c0d0 answers the callback with (available, mandatory "\\lockout\\", "\\newvername\\" up to 255, file id
//   "\\fpfileid\\", "\\dlurl\\" up to 100, param) when "\\newver\\" is set, else (0, 0, "", 0, "", param) -- also on
//   an HTTP failure -- and frees its data. 0 without a version or callback, out of memory, or when a non-blocking
//   request could not start; else 1.
// blam-cc: cdecl

#include "gamespy.h"

#include "ghttp.h"
#include "fn_gamespy.h"

typedef void (*ptPatchCallback)(int available, int mandatory, const char *versionName, int fileID,
    const char *downloadURL, void *param);

typedef struct ptaPatchData {
    ptPatchCallback callback;         // 0x0
    void *param;                      // 0x4
} ptaPatchData;

extern char ptaURL[0x200];                // 0x006a3070


static int ptaPatchCompletedCallback(int request, int result, char *buffer, int bufferLen, void *param) // 0x61c0d0
{
    ptaPatchData *data = (ptaPatchData *)param;
    char versionName[0x100];
    char downloadURL[0x65];
    const char *value;
    int mandatory;
    int fileID;

    (void)request;
    (void)bufferLen;
    if (result != 0 || (value = ptaGetKeyValue(buffer, "\\newver\\")) == 0 || atoi(value) == 0) {
        if (data->callback != 0) {
            data->callback(0, 0, "", 0, "", data->param);
        }
        free(data);
        return 1;
    }
    value = ptaGetKeyValue(buffer, "\\lockout\\");
    mandatory = value != 0 && atoi(value) != 0 ? 1 : 0;
    value = ptaGetKeyValue(buffer, "\\fpfileid\\");
    fileID = value != 0 ? atoi(value) : 0;
    value = ptaGetKeyValue(buffer, "\\newvername\\");
    if (value != 0) {
        strncpy(versionName, value, 0x100);
        versionName[0xff] = 0;
    } else {
        versionName[0] = 0;
    }
    value = ptaGetKeyValue(buffer, "\\dlurl\\");
    if (value != 0) {
        strncpy(downloadURL, value, 0x65);
        downloadURL[0x64] = 0;
    } else {
        downloadURL[0] = 0;
    }
    if (data->callback != 0) {
        data->callback(1, mandatory, versionName, fileID, downloadURL, data->param);
    }
    free(data);
    return 1;
}

int ptCheckForPatch(int productID, const char *versionUniqueID, int distributionID, ptPatchCallback callback,
    int blocking, void *param)
{
    ptaPatchData *data;

    if (versionUniqueID == 0 || callback == 0) {
        return 0;
    }
    data = (ptaPatchData *)malloc(sizeof(ptaPatchData));
    if (data == 0) {
        return 0;
    }
    data->callback = callback;
    data->param = param;
    sprintf(ptaURL, "http://hpcup.bungie.net/motd/vercheck.asp?productid=%d&versionuniqueid=%s&distid=%d", productID,
        versionUniqueID, distributionID);
    if (ghttpGetEx(ptaURL, "Pragma: no-cache\r\n", 0, 0, 0, 0, blocking, 0, ptaPatchCompletedCallback, data) == -1 &&
        blocking == 0) {
        return 0;
    }
    return 1;
}
