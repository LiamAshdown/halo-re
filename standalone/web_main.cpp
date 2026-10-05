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
 * The install's DigitalProductID (the product key check reads it from the registry on Windows): the server hands it
 * over as /halo/digital_product_id.bin, and it goes into the settings store the first time.
 */
void import_product_id()
{
    const halo::shell::SettingsStore &settings = halo::shell::SettingsStore::current();
    uint8_t value[0x100];
    uint32_t size = sizeof(value);
    FILE *file;

    if (settings.read_value(halo::shell::SettingsScope::machine, "DigitalProductID", nullptr, value, &size)) {
        return;
    }
    file = fopen("/halo/digital_product_id.bin", "rb");
    if (file == nullptr) {
        return;
    }
    size = static_cast<uint32_t>(fread(value, 1, sizeof(value), file));
    fclose(file);
    if (size != 0) {
        settings.write_string(halo::shell::SettingsScope::machine, "DigitalProductID", reinterpret_cast<const char *>(value), size);
    }
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
