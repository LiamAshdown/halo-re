/**
 * standalone/web_main.cpp -- the browser build's entry point (Emscripten), in place of standalone/loader.cpp.
 *
 * The page (web/shell.html) fetches /halo/manifest.json from the local server (tools/serve_web.py) before the module
 * starts. main() then builds the game's file tree from it: /halo on WasmFS's fetch backend, so each file is fetched
 * from the server as the game reads it (range requests, in chunks), and the home folder on the browser's private
 * file system (OPFS), so saves, profiles and settings survive a reload. Then it starts the engine as the Windows
 * loader does, from the Halo folder.
 *
 * Not built or run yet: it needs the Emscripten SDK (docs/BROWSER_PORT.md, milestone 5).
 */
#include "halo/shell/api.hpp"
#include "halo/shell/standalone.hpp"
#include "halo/shell/settings.hpp"

#include <emscripten.h>
#include <emscripten/wasmfs.h>

#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern "C" {

extern char *shell_module_path;
int standalone_data_layout_check(void);

/** The manifest the page fetched before starting ("" when there is none); main() runs on a worker, the page's
    Module is the main thread's, so the string is made there (the heap is shared). */
static char *web_halo_manifest()
{
    return reinterpret_cast<char *>(MAIN_THREAD_EM_ASM_INT({ return stringToNewUTF8(Module.haloManifest || ""); }));
}

/** The page's ?fps=N: frames on a timer instead of animation frames (which hidden pages do not get). */
static int web_fps()
{
    return MAIN_THREAD_EM_ASM_INT({ return parseInt(new URLSearchParams(location.search).get("fps") || "0", 10) || 0; });
}

/** The tab's generated player key (web/shell.html), as 25 key characters ("" when none). */
static char *web_cd_key()
{
    return reinterpret_cast<char *>(MAIN_THREAD_EM_ASM_INT({ return stringToNewUTF8(Module.haloCdKey || ""); }));
}

/** The game's command line: the page's ?args=... (for example -novideo). */
static char *web_command_line()
{
    return reinterpret_cast<char *>(MAIN_THREAD_EM_ASM_INT({
        return stringToNewUTF8(new URLSearchParams(location.search).get("args") || "");
    }));
}

void __cdecl standalone_log(const char *format, ...)
{
    va_list ap;

    va_start(ap, format);
    halo::standalone::log_formatted(format, ap);
    va_end(ap);
}

int standalone_devmode(void)
{
    return halo::standalone::devmode_enabled() ? 1 : 0;
}

const char standalone_halo_folder[] = "/halo";

}  // extern "C"

namespace halo::standalone {

void log_formatted(const char *format, va_list ap)
{
    vprintf(format, ap);
    putchar('\n');
}

bool devmode_enabled()
{
    return false;
}

namespace {

char g_module_path[] = "\\halo\\halo.exe";

/** Creates every missing directory of path (a file path: its last component is left alone). */
void make_parents(const char *path)
{
    char partial[1024];

    for (size_t i = 1; path[i] != '\0' && i < sizeof(partial) - 1; i++) {
        if (path[i] == '/') {
            memcpy(partial, path, i);
            partial[i] = '\0';
            mkdir(partial, 0777);
        }
    }
}

/**
 * Creates every "path" in the manifest as a fetch-backed file under /.server and links /halo/<path> to it; returns how
 * many. The /halo folders are ordinary memory folders, so a file the game creates there (logs, dumps) is a writable
 * memory file rather than a fetch file, which cannot be written.
 */
int build_halo_tree(const char *manifest)
{
    const char *cursor = manifest;
    int count = 0;

    while ((cursor = strstr(cursor, "\"path\": \"")) != nullptr) {
        char path[1024];
        char link[1024];
        const char *start = cursor + 9;
        const char *end = strchr(start, '"');
        int fd;

        if (end == nullptr || end - start > 900) {
            break;
        }
        snprintf(path, sizeof(path), "/.server/%.*s", static_cast<int>(end - start), start);
        snprintf(link, sizeof(link), "/halo/%.*s", static_cast<int>(end - start), start);
        make_parents(path);
        make_parents(link);
        fd = open(path, O_CREAT | O_WRONLY, 0444);  // a fetch-backed file: its contents come from the server on first read
        if (fd >= 0) {
            close(fd);
            if (symlink(path, link) == 0) {
                count++;
            }
        }
        cursor = end;
    }
    return count;
}

/**
 * The DigitalProductID record for the page's generated player key: the key's 25 characters read as a base-24 number (the
 * product key alphabet) fill the 15 key bytes, which is all the game uses of it (hashed into the CD key string the
 * host checks for duplicates). The product id digits stay zero: they come from the installer's key check, which the
 * page does not have; nothing checks a key's authenticity. False when the key is not 25 key characters.
 */
struct digital_product_id {  // the fields ProductId::build_string reads (types/shell.h has the whole record)
    uint32_t size;              // 0x00 0xa4
    uint16_t major_version;     // 0x04 3
    uint16_t minor_version;     // 0x06 0
    char product_id[0x18];      // 0x08
    uint8_t unknown_20[0x18];   // 0x20
    uint8_t hashed_key[0xf];    // 0x38
    uint8_t unknown_47[0x5d];   // 0x47
};
static_assert(sizeof(digital_product_id) == 0xa4, "DigitalProductID record size");

bool product_id_from_cd_key(const char *key, digital_product_id *data)
{
    static const char alphabet[] = "BCDFGHJKMPQRTVWXY2346789";
    int count = 0;

    memset(data, 0, sizeof(*data));
    data->size = sizeof(*data);
    data->major_version = 3;
    memcpy(data->product_id, "00000-000-0000000-00000", 23);
    for (; *key != 0; key++) {
        const char *digit = strchr(alphabet, *key);
        uint32_t carry;

        if (*key == '-' || *key == ' ') {
            continue;
        }
        if (digit == nullptr || ++count > 25) {
            return false;
        }
        carry = static_cast<uint32_t>(digit - alphabet);
        for (uint8_t &byte : data->hashed_key) {  // little-endian: key = key * 24 + digit
            carry += byte * 24u;
            byte = static_cast<uint8_t>(carry);
            carry >>= 8;
        }
    }
    return count == 25;
}

void import_product_id()
{
    char *key = web_cd_key();
    digital_product_id typed;

    if (product_id_from_cd_key(key, &typed)) {
        halo::shell::SettingsStore::current().write_string(halo::shell::SettingsScope::machine, "DigitalProductID",
            reinterpret_cast<const char *>(&typed), sizeof(typed));
    }
    free(key);
}

}  // namespace

}  // namespace halo::standalone

int main()
{
    using namespace halo::standalone;
    char *manifest = web_halo_manifest();
    char *command_line = web_command_line();
    backend_t persistent = wasmfs_create_opfs_backend();
    backend_t server = wasmfs_create_fetch_backend("halo", 1 << 20);
    int files;

    wasmfs_create_directory("/home", 0777, persistent);
    setenv("HOME", "/home", 1);
    if (strstr(command_line, "-coop join") != nullptr) {
        mkdir("/home/coop-join", 0777);  // a joiner in a second tab keeps its own profile and saves
        setenv("HOME", "/home/coop-join", 1);
    }
    if (int fps = web_fps()) {
        char text[16];

        snprintf(text, sizeof(text), "%d", fps);
        setenv("HALO_WEB_FPS", text, 1);  // read by MainLoop::loop
    }
    wasmfs_create_directory("/.server", 0777, server);
    mkdir("/halo", 0777);
    files = build_halo_tree(manifest);
    printf("halo: %d game files from the server\n", files);
    free(manifest);
    if (files == 0) {
        printf("halo: no manifest; start the page through tools/serve_web.py\n");
        return 1;
    }
    if (int misplaced = standalone_data_layout_check()) {
        printf("halo: %d engine globals are not laid out as in the original image\n", misplaced);
    }
    import_product_id();
    shell_module_path = g_module_path;
    chdir("/halo");
    return halo::shell::shell_winmain(nullptr, nullptr, command_line, 1);
}
