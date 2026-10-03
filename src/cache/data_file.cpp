#include "tags.h"

#include "halo/cache/cache.hpp"

#include "win32.h"
#include "crt.h"
#include "halo/cache/globals.hpp"

extern "C" {
extern int32_t printf(const char *format, ...);
extern void os_platform_identify(void);
extern int32_t os_platform;
}

namespace halo::cache {

/**
 * Reads the raw payload block (data_offset up to table_offset) into a newly allocated buffer. Prints a
 * diagnostic and returns 0 on a seek or read failure.
 *
 * @address 0x443ba0
 */
uint8_t data_file_view::read_data_block()
{
    uint32_t block_size;
    void *buffer;
    uint32_t bytes_read;

    if (SetFilePointer(this->file, this->data_offset, (PLONG)((void *)0), 0) != 0xffffffff) {
        block_size = this->table_offset - this->data_offset;
        buffer = GlobalAlloc(0, block_size);
        this->data = buffer;
        if (ReadFile(this->file, buffer, block_size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 && bytes_read == block_size) {
            this->data_capacity = block_size;
            this->data_size = block_size;
            return 1;
        }
    }
    printf("Invalid format in data this %s\n", this->name);
    return 0;
}

/**
 * Reads the 16 byte on-disk header into the record and checks file_id against expected_file_id. On a
 * failed read or an id mismatch it zeroes those fields and prints a diagnostic.
 *
 * @address 0x443b30
 */
int32_t data_file_view::read_header(int32_t expected_file_id)
{
    uint32_t bytes_read;

    if (ReadFile(this->file, this, 0x10, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 && bytes_read == 0x10) {
        if (this->file_id != expected_file_id) {
            this->file_id = 0;
            this->data_offset = 0;
            this->table_offset = 0;
            this->entry_count = 0;
            printf("Invalid data this id in data this %s\n", this->name);
            return 0;
        }
        return 1;
    }
    printf("Failed to read data this header %s\n", this->name);
    return 0;
}

/**
 * Reads the table of 12 byte reference entries at table_offset into a newly allocated array sized by
 * entry_count. Prints a diagnostic and returns 0 on failure.
 *
 * @address 0x443c20
 */
uint8_t data_file_view::read_offset_table()
{
    uint32_t table_size;
    void *buffer;
    uint32_t bytes_read;

    if (SetFilePointer(this->file, this->table_offset, (PLONG)((void *)0), 0) != 0xffffffff) {
        table_size = this->entry_count * 0xc;
        buffer = GlobalAlloc(0, table_size);
        this->references = (data_file_reference *)buffer;
        if (ReadFile(this->file, buffer, table_size, (LPDWORD)(&bytes_read), (LPOVERLAPPED)((void *)0)) != 0 && bytes_read == table_size) {
            this->reference_count = this->entry_count;
            return 1;
        }
    }
    printf("Invalid format in data this %s\n", this->name);
    return 0;
}

/**
 * Opens bitmaps.map and sounds.map, validating each header, data block and reference table. A failing
 * file is released with a diagnostic and does not stop the other one. Finally allocates the shared
 * 0x6000 byte request queue and starts the IO worker thread.
 *
 * @address 0x442840
 */
void data_files::open()
{
    char path[260];
    uint32_t flags;

    halo::cache::data_files::zero(&globals().bitmaps_data_file);
    globals().cache_file_index = (int16_t)0xffff;
    globals().bitmaps_data_file.name = (char *)"bitmaps";
    globals().bitmaps_data_file.unknown_24 = 0;
    sprintf(path, "maps\\%s.map", "bitmaps");

    flags = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags = 0x8000080;
    }
    globals().bitmaps_data_file.file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 4, flags, (void *)0);
    if (globals().bitmaps_data_file.file == (void *)0xffffffff) {
        printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
    } else {

        if (halo::cache::view(&globals().bitmaps_data_file)->read_header(1) == 0 ||
            halo::cache::view(&globals().bitmaps_data_file)->read_data_block() == 0 ||
            halo::cache::view(&globals().bitmaps_data_file)->read_offset_table() == 0) {
            if (globals().bitmaps_data_file.data != 0) {
                GlobalFree(globals().bitmaps_data_file.data);
                globals().bitmaps_data_file.data = 0;
            }
            if (globals().bitmaps_data_file.references != 0) {
                GlobalFree(globals().bitmaps_data_file.references);
                globals().bitmaps_data_file.references = 0;
            }
            printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");
        } else {
            SetFilePointer(globals().bitmaps_data_file.file, globals().bitmaps_data_file.data_offset, (PLONG)((void *)0), 0);
        }
    }

    halo::cache::data_files::zero(&globals().sounds_data_file);
    globals().sounds_data_file.name = (char *)"sounds";
    globals().sounds_data_file.unknown_24 = 0;
    sprintf(path, "maps\\%s.map", "sounds");

    flags = 0x48000080;
    if (os_platform == 0) {
        os_platform_identify();
    }
    if (os_platform < 3) {
        flags = 0x8000080;
    }
    globals().sounds_data_file.file = CreateFileA(path, 0x80000000, 1, (LPSECURITY_ATTRIBUTES)((void *)0), 4, flags, (void *)0);
    if (globals().sounds_data_file.file != (void *)0xffffffff) {
        if (halo::cache::view(&globals().sounds_data_file)->read_header(2) != 0 &&
            halo::cache::view(&globals().sounds_data_file)->read_data_block() != 0 &&
            halo::cache::view(&globals().sounds_data_file)->read_offset_table() != 0) {
            SetFilePointer(globals().sounds_data_file.file, globals().sounds_data_file.data_offset, (PLONG)((void *)0), 0);
            goto allocate_io_queue;
        }
        if (globals().sounds_data_file.data != 0) {
            GlobalFree(globals().sounds_data_file.data);
            globals().sounds_data_file.data = 0;
        }
        if (globals().sounds_data_file.references != 0) {
            GlobalFree(globals().sounds_data_file.references);
            globals().sounds_data_file.references = 0;
        }
    }
    printf("### FAILED TO OPEN DATA-CACHE FILE.\n\n");

allocate_io_queue:
    globals().cache_io_requests = (cache_io_request *)GlobalAlloc(0, 0x6000);
    halo::cache::cache_io::thread_start();
}

/**
 * Closes both data files, freeing their buffers and the shared request queue, and closes and clears
 * the active cache file slot.
 *
 * @address 0x442a50
 */
void data_files::close()
{
    uint32_t *destination;
    int32_t i;

    if (globals().cache_file_index != -1) {
        halo::cache::cache_io::wait_all_requests();
        CloseHandle(globals().cache_file_slots[globals().cache_file_index].file);
        destination = (uint32_t *)&globals().cache_file_slots[globals().cache_file_index];
        for (i = 0x203; i != 0; i--) {
            *destination++ = 0;
        }
        globals().cache_file_index = -1;
    }

    CloseHandle(globals().bitmaps_data_file.file);
    if (globals().bitmaps_data_file.data != 0) {
        GlobalFree(globals().bitmaps_data_file.data);
    }
    if (globals().bitmaps_data_file.references != 0) {
        GlobalFree(globals().bitmaps_data_file.references);
    }
    globals().bitmaps_data_file.file_id = 0;
    globals().bitmaps_data_file.data_offset = 0;
    globals().bitmaps_data_file.table_offset = 0;
    globals().bitmaps_data_file.entry_count = 0;

    CloseHandle(globals().sounds_data_file.file);
    if (globals().sounds_data_file.data != 0) {
        GlobalFree(globals().sounds_data_file.data);
    }
    if (globals().sounds_data_file.references != 0) {
        GlobalFree(globals().sounds_data_file.references);
    }
    globals().sounds_data_file.file_id = 0;
    globals().sounds_data_file.data_offset = 0;
    globals().sounds_data_file.table_offset = 0;
    globals().sounds_data_file.entry_count = 0;

    GlobalFree(globals().cache_io_requests);
}

/**
 * Clears the first 0x40 bytes of a data_file record before it is opened.
 */
void data_files::zero(data_file *file)
{
    uint32_t *word;
    int32_t i;

    word = (uint32_t *)file;
    for (i = 0x10; i != 0; i--) {
        *word++ = 0;
    }
}

} // namespace halo::cache
