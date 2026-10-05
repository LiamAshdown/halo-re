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

/** The manifest the page fetched before starting the module ("" when there is none). */
EM_JS(char *, web_halo_manifest, (), {
    return stringToNewUTF8(Module.haloManifest || "");
});

/** The game's command line: the page's ?args=... (for example -novideo). */
EM_JS(char *, web_command_line, (), {
    const args = new URLSearchParams(location.search).get("args") || "";
    return stringToNewUTF8(args);
});

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

/** Creates /halo/<path> for every "path" in the manifest; returns how many. */
int build_halo_tree(const char *manifest)
{
    const char *cursor = manifest;
    int count = 0;

    while ((cursor = strstr(cursor, "\"path\": \"")) != nullptr) {
        char path[1024];
        const char *start = cursor + 9;
        const char *end = strchr(start, '"');
        int fd;

        if (end == nullptr || end - start > 900) {
            break;
        }
        snprintf(path, sizeof(path), "/halo/%.*s", static_cast<int>(end - start), start);
        make_parents(path);
        fd = open(path, O_CREAT | O_WRONLY, 0444);  // a fetch-backed file: its contents come from the server on first read
        if (fd >= 0) {
            close(fd);
            count++;
        }
        cursor = end;
    }
    return count;
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
    wasmfs_create_directory("/halo", 0777, server);
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
    shell_module_path = g_module_path;
    chdir("/halo");
    return halo::shell::shell_winmain(nullptr, nullptr, command_line, 1);
}
