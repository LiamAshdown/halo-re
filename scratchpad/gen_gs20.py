exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gen_gs18.py').read().split("FAIL = '''")[0].replace("exec(open(r'C:\\Users\\Liam-\\halo-re\\scratchpad\\gs_lib.py').read())", ''))
exec(open(r'C:\Users\Liam-\halo-re\scratchpad\gs_lib.py').read())
FAIL = '''    connection->completed = 1;
    connection->result = %s;
'''
FIRST = '--' + BOUNDARY + '\\r\\n'
NEXT = '\\r\\n--' + BOUNDARY + '\\r\\n'
LAST = '\\r\\n--' + BOUNDARY + '--\\r\\n'

e(0x6220f0, 8, 'ghiIsPostAutoFree', 'the post  auto-free flag.', '''
int ghiIsPostAutoFree(GHIPost *post)
{
    return post->autoFree;
}
''')
e(0x622100, 24, 'ghttpFreePost', 'frees the post  data array and the post.', '''
void ghttpFreePost(GHIPost *post)
{
    ArrayFree(post->data);
    free(post);
}
''')
e(0x622120, 38, 'ghiPostGetContentType', '"" without a post; multipart/form-data with the boundary, or application/x-www-form-urlencoded.', '''
const char *ghiPostGetContentType(GHIConnection *connection)
{
    if (connection->post == 0) {
        return "";
    }
    if (connection->post->useMultipart != 0) {
        return "multipart/form-data; boundary=''' + BOUNDARY + '''";
    }
    return "application/x-www-form-urlencoded";
}
''')
e(0x622150, 108, 'ghiPostGetNoFilesContentLength', 'EAX connection: the url-encoded body length: per field name, "=", the value with 2 more per extended character, "&" between.', '''
int ghiPostGetNoFilesContentLength(GHIConnection *connection)
{
    DArray data = connection->post->data;
    int count = ArrayLength(data);
    int total = 0;
    int i;

    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        GHIPostData *field = (GHIPostData *)ArrayNth(data, i);

        total += (int)strlen(field->name) + field->data.string.len + field->data.string.extendedChars * 2 + 1;
    }
    return total + count - 1;
}
''', cc='EAX -> connection')
e(0x6221c0, 383, 'ghiPostGetHasFilesContentLength', 'the multipart body length: fixed parts (set once: boundary 0x25, string part 0x54, file part 0x71, end 0x29) plus names, file names, content types and data lengths (a disk file  from its posting state); an unknown field type gives 0.', '''
static int boundaryLength;       // 0x006a3290
static int filePartLength;       // 0x006a3294
static int stringPartLength;     // 0x006a3298
static int endLength;            // 0x006a329c

int ghiPostGetHasFilesContentLength(GHIConnection *connection)
{
    DArray data = connection->post->data;
    int total = 0;
    int count;
    int i;

    if (boundaryLength == 0) {
        boundaryLength = 0x25;
        stringPartLength = 0x54;
        filePartLength = 0x71;
        endLength = 0x29;
    }
    count = ArrayLength(data);
    for (i = 0; i < count; i++) {
        GHIPostData *field = (GHIPostData *)ArrayNth(data, i);

        if (field->type == 0) {
            total += (int)strlen(field->name) + field->data.string.len + stringPartLength;
        } else if (field->type == 1) {
            total += (int)strlen(field->data.fileDisk.contentType) + (int)strlen(field->data.fileDisk.reportFilename) +
                     (int)strlen(field->name) + filePartLength;
            total += ((GHIPostState *)ArrayNth(connection->postingStates, i))->len;
        } else if (field->type == 2) {
            total += field->data.fileMemory.len + (int)strlen(field->data.fileMemory.contentType) +
                     (int)strlen(field->data.fileMemory.reportFilename) + (int)strlen(field->name) + filePartLength;
        } else {
            return 0;
        }
    }
    return endLength + total;
}
''')
e(0x622340, 113, 'ghiPostStateInit', 'ESI state: not started (-1); a disk file is opened "rb" and measured (0 when either fails); strings and memory files need nothing; other types 0.', '''
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
''', cc='ESI -> state')
e(0x6223c0, 375, 'ghiPostInitState', 'with a post: counters cleared, the post callback taken, one posting state per field (a failure closes the files opened so far and frees them: 0); the total body length by encoding.', '''
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
''')
e(0x622540, 162, 'ghiPostCleanupState', 'closes the disk files of every posting state and frees them; an auto-free post is freed.', '''
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
''')
e(0x6225f0, 261, 'ghiPostStringStateDoPosting', 'EAX state, EDX connection: an empty value is done. Url-encoded with invalid characters: the whole string is escaped into the send buffer (safe characters as is, space "+", others "%XX" from the SIGNED character -- a high one indexes before the digit table in the binary). Otherwise sent directly: from the START of the string each time (the position is only counted); 1 done, 2 partial, 0 error.', '''
int ghiPostStringStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    static const char digits[] = "0123456789ABCDEF";
    GHIPostData *field = state->data;
    int len = field->data.string.len;
    int remaining;
    int sent;

    if (len == 0) {
        return 1;
    }
    if (connection->post->useMultipart == 0 && field->data.string.invalidChars != 0) {
        const char *c = field->data.string.string;
        char hex[4] = {'%', '0', '0', 0};

        for (; *c != 0; c++) {
            int value = *c;

            if (strchr("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_@-.*", value) != 0) {
                ghiAppendCharToBuffer(&connection->sendBuffer, value);
            } else if (value == ' ') {
                ghiAppendCharToBuffer(&connection->sendBuffer, '+');
            } else {
                hex[1] = digits[value / 16];
                hex[2] = digits[value % 16];
                ghiAppendDataToBuffer(&connection->sendBuffer, hex, 3);
            }
        }
        return 1;
    }
    remaining = len - state->pos;
    sent = ghiDoSend(connection, field->data.string.string, remaining);
    if (sent == -1) {
        return 0;
    }
    state->pos += sent;
    return (sent != remaining) + 1;
}
''', cc='EAX -> state, EDX -> connection')
e(0x622700, 183, 'ghiPostFileDiskStateDoPosting', 'ESI state, EDI connection: 4 KB reads of the file (a short or over-long read fails with file read failed 14) sent or queued until the whole file went (1); a queued part returns 2.', '''
int ghiPostFileDiskStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    char buffer[0x1000];

    for (;;) {
        int n = (int)fread(buffer, 1, 0x1000, state->file);
        int result;

        if (n <= 0) {
            break;
        }
        state->pos += n;
        if (state->pos > state->len) {
            break;
        }
        result = ghiTrySendThenBuffer(connection, buffer, n);
        if (result == 0) {
            return 0;
        }
        if (state->pos == state->len) {
            return 1;
        }
        if (result != 1) {
            return 2;
        }
    }
''' + FAIL % '0xe' + '''    return 0;
}
''', cc='ESI -> state, EDI -> connection')
e(0x6227c0, 67, 'ghiPostFileMemoryStateDoPosting', 'EDI state, stack connection: sends the rest of the memory file directly; 1 all sent, 2 partial, 0 error.', '''
int ghiPostFileMemoryStateDoPosting(GHIPostState *state, GHIConnection *connection)
{
    int len = state->data->data.fileMemory.len;
    int remaining;
    int sent;

    if (len == 0) {
        return 1;
    }
    remaining = len - state->pos;
    sent = ghiDoSend(connection, state->data->data.fileMemory.buffer + state->pos, remaining);
    if (sent == -1) {
        return 0;
    }
    state->pos += sent;
    return (sent != remaining) + 1;
}
''', cc='EDI -> state, stack -> connection')
e(0x622810, 421, 'ghiPostStateDoPosting', 'stack state, ECX first, EDX connection: before the field  data its header goes out once ("name=" / "&name=", or the multipart part header with the boundary, and for files filename and content type; an unknown type sends whatever the binary  stack held -- here nothing); then the data by type.', '''
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
            sprintf(header, "%sContent-Disposition: form-data; name=\\"%s\\"\\r\\n\\r\\n",
                first != 0 ? "''' + FIRST + '''" : "''' + NEXT + '''", field->name);
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
                "%sContent-Disposition: form-data; name=\\"%s\\"; filename=\\"%s\\"\\r\\nContent-Type: %s\\r\\n\\r\\n",
                first != 0 ? "''' + FIRST + '''" : "''' + NEXT + '''", field->name, filename, contentType);
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
''', cc='stack -> state, ECX -> first, EDX -> connection')
e(0x6229c0, 235, 'ghiPostDoPosting', 'flushes queued data first (partial: 2); posts each remaining field in order (first flag for field 0); multipart ends with the closing boundary. 1 when nothing is left queued, 2 while some is, 0 on error.', '''
int ghiPostDoPosting(GHIConnection *connection)
{
    int count = ArrayLength(connection->postingStates);

    if (connection->sendBuffer.len != 0) {
        if (!ghiSendBufferedData(&connection->sendBuffer, connection)) {
            return 0;
        }
        if (connection->sendBuffer.pos < connection->sendBuffer.len) {
            return 2;
        }
        ghiResetBuffer(&connection->sendBuffer);
        if (connection->objectsPosted == count) {
            return 1;
        }
    }
    for (; connection->objectsPosted < count; connection->objectsPosted++) {
        int result = ghiPostStateDoPosting((GHIPostState *)ArrayNth(connection->postingStates, connection->objectsPosted),
            connection, connection->objectsPosted == 0);

        if (result == 0) {
            return 0;
        }
        if (result == 2) {
            return 2;
        }
    }
    if (connection->post->useMultipart != 0 && !ghiTrySendThenBuffer(connection, "''' + LAST + '''", 0x2b)) {
        return 0;
    }
    return (connection->sendBuffer.len != 0) + 1;
}
''')

# ------------------------------------------------------------------ ghttpMain.c
e(0x61bd00, 52, 'ghttpStartup', 'counts a startup (under the lock, which does not exist yet the first time); the first creates the lock and sets the throttle to 125 bytes / 250 ms -- and leaves without unlocking; later ones unlock.', THROTTLE + '''
extern int ghiReferenceCount;             // 0x006a2e6c

void ghttpStartup(void)
{
    ghiLock();
    ghiReferenceCount++;
    if (ghiReferenceCount == 1) {
        ghiCreateLock();
        ghiThrottleBufferSize = 0x7d;
        ghiThrottleTimeDelay = 0xfa;
        return;
    }
    ghiUnlock();
}
''')
e(0x61bd40, 61, 'ghttpCleanup', 'the last cleanup frees every connection and the proxy string, unlocks and frees the lock; earlier ones just unlock.', PROXY + '''
extern int ghiReferenceCount;             // 0x006a2e6c

void ghttpCleanup(void)
{
    ghiLock();
    ghiReferenceCount--;
    if (ghiReferenceCount != 0) {
        ghiUnlock();
        return;
    }
    ghiCleanupConnections();
    if (ghiProxyAddress != 0) {
        free(ghiProxyAddress);
        ghiProxyAddress = 0;
    }
    ghiUnlock();
    ghiFreeLock();
}
''')
e(0x61bd80, 357, 'ghttpGetEx', '(URL, headers, buffer, buffer size, post, throttle, blocking, progress, completed, param): -1 for no URL, a negative size or a buffer without a size; starts ghttp when needed; a get connection with copies of the URL and headers, the caller  buffer (fixed) or a 2 KB growable one, and the post state. Blocking: processed every 10 ms until done (0); else the request index. -1 (connection freed) on failure.', '''
extern int ghiReferenceCount;             // 0x006a2e6c

int ghttpGetEx(const char *URL, const char *headers, char *buffer, int bufferSize, GHIPost *post, int throttle,
    int blocking, ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void *param)
{
    GHIConnection *connection;
    int ok;

    if (URL == 0 || *URL == 0 || bufferSize < 0) {
        return -1;
    }
    if (buffer != 0 && bufferSize == 0) {
        return -1;
    }
    if (ghiReferenceCount == 0) {
        ghttpStartup();
    }
    connection = ghiNewConnection();
    if (connection == 0) {
        return -1;
    }
    connection->type = 0;
    connection->URL = _strdup(URL);
    if (connection->URL == 0) {
        goto fail;
    }
    if (headers != 0 && *headers != 0) {
        connection->sendHeaders = _strdup(headers);
        if (connection->sendHeaders == 0) {
            goto fail;
        }
    }
    connection->progressCallback = progressCallback;
    connection->throttle = throttle;
    connection->post = post;
    connection->blocking = blocking;
    connection->completedCallback = completedCallback;
    connection->callbackParam = param;
    connection->userBufferSupplied = buffer != 0;
    if (connection->userBufferSupplied) {
        ok = ghiInitFixedBuffer(connection, &connection->getFileBuffer, buffer, bufferSize);
    } else {
        ok = ghiInitBuffer(connection, &connection->getFileBuffer, 0x800, 0x800);
    }
    if (!ok || (post != 0 && !ghiPostInitState(connection))) {
        goto fail;
    }
    if (blocking != 0) {
        while (!ghiProcessConnection(connection)) {
            msleep(10);
        }
        return 0;
    }
    return connection->request;

fail:
    ghiFreeConnection(connection);
    return -1;
}
''')
e(0x61bef0, 289, 'ghttpSaveEx', '(URL, file name, headers, post, throttle, blocking, progress, completed, param): like ghttpGetEx but a save connection writing the file opened "wb" (after the post state).', '''
extern int ghiReferenceCount;             // 0x006a2e6c

int ghttpSaveEx(const char *URL, const char *filename, const char *headers, GHIPost *post, int throttle, int blocking,
    ghttpProgressCallback progressCallback, ghttpCompletedCallback completedCallback, void *param)
{
    GHIConnection *connection;

    if (URL == 0 || *URL == 0 || filename == 0 || *filename == 0) {
        return -1;
    }
    if (ghiReferenceCount == 0) {
        ghttpStartup();
    }
    connection = ghiNewConnection();
    if (connection == 0) {
        return -1;
    }
    connection->type = 1;
    connection->URL = _strdup(URL);
    if (connection->URL == 0) {
        goto fail;
    }
    if (headers != 0 && *headers != 0) {
        connection->sendHeaders = _strdup(headers);
        if (connection->sendHeaders == 0) {
            goto fail;
        }
    }
    connection->progressCallback = progressCallback;
    connection->completedCallback = completedCallback;
    connection->post = post;
    connection->blocking = blocking;
    connection->callbackParam = param;
    connection->throttle = throttle;
    if (post != 0 && !ghiPostInitState(connection)) {
        goto fail;
    }
    connection->saveFile = fopen(filename, "wb");
    if (connection->saveFile == 0) {
        goto fail;
    }
    if (blocking != 0) {
        while (!ghiProcessConnection(connection)) {
            msleep(10);
        }
        return 0;
    }
    return connection->request;

fail:
    ghiFreeConnection(connection);
    return -1;
}
''')
e(0x61c020, 12, 'ghttpThink', 'processes every live connection.', '''
void ghttpThink(void)
{
    ghiEnumConnections(ghiProcessConnection);
}
''')
e(0x61c030, 27, 'ghttpCancelRequest', 'frees the request  connection when it is live.', '''
void ghttpCancelRequest(int request)
{
    GHIConnection *connection = ghiRequestToConnection(request);

    if (connection != 0) {
        ghiFreeConnection(connection);
    }
}
''')

# ------------------------------------------------------------------ pt.c
e(0x61c060, 105, 'ptaGetKeyValue', 'EAX buffer, ECX key: the value after the key up to the next backslash (at most 255 chars) copied into a static buffer (0x6a2e70); NULL when the key is missing.', '''
extern char ptaKeyValue[0x100];           // 0x006a2e70

char *ptaGetKeyValue(const char *buffer, const char *key)
{
    const char *value = strstr(buffer, key);
    size_t len;

    if (value == 0) {
        return 0;
    }
    value += strlen(key);
    len = strcspn(value, "\\\\");
    if (len >= 0xff) {
        len = 0xff;
    }
    memcpy(ptaKeyValue, value, len);
    ptaKeyValue[len] = 0;
    return ptaKeyValue;
}
''', cc='EAX -> buffer, ECX -> key')
e(0x61c260, 151, 'ptCheckForPatch', '(product id, version unique id, distribution id, callback, blocking, param): asks hpcup.bungie.net/motd/vercheck.asp (no-cache) through ghttpGetEx; the static completion 0x61c0d0 answers the callback with (available, mandatory "\\\\lockout\\\\", "\\\\newvername\\\\" up to 255, file id "\\\\fpfileid\\\\", "\\\\dlurl\\\\" up to 100, param) when "\\\\newver\\\\" is set, else (0, 0, "", 0, "", param) -- also on an HTTP failure -- and frees its data. 0 without a version or callback, out of memory, or when a non-blocking request could not start; else 1.', '''
typedef void (*ptPatchCallback)(int available, int mandatory, const char *versionName, int fileID,
    const char *downloadURL, void *param);

typedef struct ptaPatchData {
    ptPatchCallback callback;         // 0x0
    void *param;                      // 0x4
} ptaPatchData;

extern char ptaURL[0x200];                // 0x006a3070
extern char *ptaGetKeyValue(const char *buffer, const char *key);

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
    if (result != 0 || (value = ptaGetKeyValue(buffer, "\\\\newver\\\\")) == 0 || atoi(value) == 0) {
        if (data->callback != 0) {
            data->callback(0, 0, "", 0, "", data->param);
        }
        free(data);
        return 1;
    }
    value = ptaGetKeyValue(buffer, "\\\\lockout\\\\");
    mandatory = value != 0 && atoi(value) != 0 ? 1 : 0;
    value = ptaGetKeyValue(buffer, "\\\\fpfileid\\\\");
    fileID = value != 0 ? atoi(value) : 0;
    value = ptaGetKeyValue(buffer, "\\\\newvername\\\\");
    if (value != 0) {
        strncpy(versionName, value, 0x100);
        versionName[0xff] = 0;
    } else {
        versionName[0] = 0;
    }
    value = ptaGetKeyValue(buffer, "\\\\dlurl\\\\");
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
    if (ghttpGetEx(ptaURL, "Pragma: no-cache\\r\\n", 0, 0, 0, 0, blocking, 0, ptaPatchCompletedCallback, data) == -1 &&
        blocking == 0) {
        return 0;
    }
    return 1;
}
''')
print('ok')
