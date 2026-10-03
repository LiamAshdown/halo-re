#pragma once
// Blam cache module (halo.exe 1.0.10 retail, 0x442290..0x444b30, 53 functions).
// The map (cache) file, the two external data files (bitmaps.map / sounds.map), the
// asynchronous cache-file IO queue and its worker thread, and the two runtime streaming
// caches (sound pages and texture handles) built on the generic cache container in
// types/memory.h. Offsets in comments are byte offsets from the struct base and were
// recovered from the decompiled module, from the raw instruction stream where Ghidra lost a
// register argument, and from the image data at 0x006869c0.
//
// Types this module operates on that already have a definition, and are therefore NOT
// redefined here (see out/phase4/cache_types_notes.md for which function proved which):
//   types/memory.h   data_array, datum_index, cache, cache_entry (sound/texture caches)
//   types/tags.h     BitmapData, SoundPermutation, SoundPitchRange, Sound,
//                    ScenarioBSP, ScenarioStructureBSP, ScenarioStructureBSPLightmap,
//                    ScenarioStructureBSPMaterial, ScenarioStructureBSPCompiledHeader,
//                    GBXModel, GBXModelGeometry, GBXModelGeometryPart, ModelShaderReference,
//                    PredictedResource, ModelAnimationsAnimation
// Cross-header references below are written as incomplete struct pointers
// (struct data_array *, struct BitmapData *, ...) so this header still parses on its own.
#pragma pack(push, 1)
typedef unsigned char uint8_t; typedef signed char int8_t; typedef unsigned short uint16_t; typedef short int16_t;
typedef unsigned int uint32_t; typedef int int32_t;

// Same declaration as types/memory.h: index in the low 16 bits, salt in the high 16 bits.
typedef uint32_t datum_index;

// A four-character tag group id, stored big-endian-as-written in the image, e.g.
// 0x6d6f6432 reads "mod2". Compared as a plain 32-bit value everywhere in this module.
typedef uint32_t tag_group;

// FILETIME as the Win32 ABI lays it out; declared here because no windows.h is available to
// the CParser. cache_file_slot carries one, stamped by SystemTimeToFileTime in
// cache_file_download_finish @0x443540 and ordered by CompareFileTime in
// cache_file_find_oldest_slot @0x4437b0.
typedef struct file_time {
    uint32_t low_date_time;    // 0x00
    uint32_t high_date_time;   // 0x04
} file_time;                   // size 0x08

// SYSTEMTIME as the Win32 ABI lays it out; declared here for the same reason file_time is.
// Used only as the scratch buffer between GetSystemTime and SystemTimeToFileTime in
// cache_file_download_finish @0x443540.
typedef struct system_time {
    uint16_t year;             // 0x00
    uint16_t month;            // 0x02
    uint16_t day_of_week;      // 0x04
    uint16_t day;              // 0x06
    uint16_t hour;             // 0x08
    uint16_t minute;           // 0x0a
    uint16_t second;           // 0x0c
    uint16_t milliseconds;     // 0x0e
} system_time;                 // size 0x10

// ---------------------------------------------------------------------------
// constants
// ---------------------------------------------------------------------------
typedef enum cache_file_constants {
    k_cache_file_head_signature = 0x68656164,  // "head" at header+0x00
    k_cache_file_foot_signature = 0x666f6f74,  // "foot" at header+0x7fc
    k_cache_file_version = 7,                  // retail PC; header+0x04 must equal this
    k_cache_file_header_size = 0x800,
    k_cache_file_maximum_size = 0x18000000,    // header+0x08 must be in [0, 0x18000000]
    k_cache_file_name_length = 0x20,           // strlen(header.name) must be < 0x20
    k_cache_file_slot_count = 6,               // cache_file_find_slot_by_name @0x443770
    k_cache_io_request_count = 0x200,          // 0x200 * 0x30 == the 0x6000 GlobalAlloc
    k_map_memory_base = 0x40000000,            // cache_reserve_map_memory @0x4448d0
    k_map_memory_size = 0x1b40000,
    k_tag_data_base = 0x40440000,              // where the tag data block is read to
    k_sound_cache_maximum_entries = 0x200,     // "pc sound" data_array maximum_count
    k_sound_cache_page_shift = 0xc,            // cache block_shift: 4096-byte sound pages
    k_texture_cache_maximum_entries = 0x1000,  // "pc texture" data_array maximum_count
    k_texture_cache_block_shift = 2            // cache block_shift: the texture cache
                                               // rations 4-byte slots, not pixel storage
} cache_file_constants;

// cache_io_request::data_file_index, and the last argument of cache_io_request_new.
// Resolved to a file handle by cache_io_thread_proc_sync @0x443a10 / _async @0x443940.
typedef enum cache_io_data_file {
    _cache_io_data_file_cache = 0,    // the currently open map slot
    _cache_io_data_file_bitmaps = 1,  // bitmaps_data_file, matches data_file::file_id 1
    _cache_io_data_file_sounds = 2    // sounds_data_file,  matches data_file::file_id 2
} cache_io_data_file;

// The argument to cache_file_find_oldest_slot @0x4437b0 selects a slot range, and each slot
// carries a hard size limit the candidate map must fit under. The category names are
// inferred from the ranges and limits; the numbers are what the code computes.
typedef enum cache_file_slot_category {
    _cache_file_slot_category_single_player = 0, // slots 0..1, limit 0x18000000 (384 MB)
    _cache_file_slot_category_multiplayer = 1,   // slots 3..5, limit 0x08000000 (128 MB)
    _cache_file_slot_category_ui = 2             // slot  2,    limit 0x02300000 (35 MB)
} cache_file_slot_category;

// Return codes of cache_file_download_poll @0x442720 (raw) and the simplified codes
// cache_file_download_status @0x4434a0 maps them to.
typedef enum cache_file_download_status {
    _cache_file_download_failed = 0,
    _cache_file_download_cancelled = 1,
    _cache_file_download_reset = 2,
    _cache_file_download_idle = 3,
    _cache_file_download_running = 4
} cache_file_download_status;

// Tag group ids this module compares against literally.
typedef enum cache_tag_groups {
    _tag_group_gbxmodel = 0x6d6f6432,          // "mod2", the iterator filter at 0x442d10/0x442f00
    _tag_group_shader_environment = 0x73656e76, // "senv"
    _tag_group_shader_transparent_water = 0x73776174, // "swat"
    _tag_group_shader_transparent_glass = 0x73676c61, // "sgla"
    _tag_group_shader_model = 0x736f736f        // "soso"
} cache_tag_groups;

// ---------------------------------------------------------------------------
// cache_file_header  (validated identically in three places: cache_file_load @0x442290 on
// the copy at 0x006a8154, cache_file_exists @0x442bb0 on a stack buffer, and
// cache_file_slot_read_header @0x4435e0 on the slot copy)
// The validation is: head == "head", foot == "foot", 0 <= file_size <= 0x18000000,
// strlen(name) < 0x20, version == 7. Everything outside those five fields plus the two
// offsets the loader reads is untouched anywhere in the image.
// ---------------------------------------------------------------------------
typedef struct cache_file_header {
    uint32_t head;                 // 0x000 "head"
    int32_t version;               // 0x004 must be 7
    int32_t file_size;             // 0x008 must be in [0, 0x18000000]
    uint32_t unknown_00c;          // 0x00c
    uint32_t tag_data_offset;      // 0x010 file offset of the tag data block
    uint32_t tag_data_size;        // 0x014 bytes read to k_tag_data_base
    uint32_t unknown_018;          // 0x018
    uint32_t unknown_01c;          // 0x01c
    char name[32];                 // 0x020 scenario name, NUL terminated, < 0x20 chars
    uint8_t unknown_040[32];       // 0x040 the published retail layout puts build[32] here;
                                   //       nothing in the image reads it
    int16_t map_type;              // 0x060 0x060 header comment cites the published layout calling this map type;
                                   //    cache_file_open_by_name casts it to cache_file_slot_category to pick the slot
    int16_t unknown_062;           // 0x062
    uint32_t crc32;                // 0x064 the map checksum. Copied into the saved-game
                                   //       header and compared back in the game_state
                                   //       validator at 0x00538569 (header[0x4a]).
    uint8_t unknown_068[0x794];    // 0x068
    uint32_t foot;                 // 0x7fc "foot"
} cache_file_header;               // size 0x800

// ---------------------------------------------------------------------------
// cache_file_tag_header
// The first bytes of the tag data block, read to k_tag_data_base by cache_file_load.
// cache_file_load takes tags from +0x00 into the tag_instances global, returns the
// scenario_tag at +0x04, tag_lookup and tag_iterator_next bound their scans with the
// tag_count at +0x0c, and model_load_vertex_buffers @0x442d10 streams the model data block
// using +0x14, +0x1c and +0x20. The remaining four dwords are never touched; the published
// retail layout has a checksum at +0x08, two model part counts at +0x10 and +0x18 and the
// "tags" signature at +0x24, which is also what fixes the size at 0x28.
// ---------------------------------------------------------------------------
typedef struct cache_file_tag_header {
    struct tag_instance *tags;         // 0x00 == the tag_instances global
    datum_index scenario_tag;          // 0x04 returned by cache_file_load
    uint32_t unknown_08;               // 0x08
    int32_t tag_count;                 // 0x0c
    uint32_t unknown_10;               // 0x10
    uint32_t model_data_file_offset;   // 0x14 map-file offset of the model data block
    uint32_t unknown_18;               // 0x18
    uint32_t model_index_data_offset;  // 0x1c offset of the index data inside that block
    uint32_t model_data_size;          // 0x20 bytes to read for the whole block
    uint32_t unknown_24;               // 0x24
} cache_file_tag_header;               // size 0x28

// ---------------------------------------------------------------------------
// tag_instance
// The fixed 0x20-byte records of the tag address table. tag_lookup @0x442550 matches
// group_tag then __stricmp on path and returns tag_id; tag_iterator_next @0x4425d0 matches
// its filter against all three group fields and returns tag_id; every consumer in the module
// reaches tag data through +0x14.
// ---------------------------------------------------------------------------
typedef struct tag_instance {
    tag_group group_tag;           // 0x00 primary group
    tag_group parent_group_tag;    // 0x04 secondary group, -1 when there is none
    tag_group grandparent_group_tag; // 0x08 tertiary group, -1 when there is none
    datum_index tag_id;            // 0x0c
    char *path;                    // 0x10 tag path without the group extension
    void *data;                    // 0x14 the tag structure itself
    uint32_t unknown_18;           // 0x18 never read or written by this module
    uint32_t unknown_1c;           // 0x1c never read or written by this module
} tag_instance;                    // size 0x20

// ---------------------------------------------------------------------------
// tag_iterator
// Stack object driven by tag_iterator_next @0x4425d0. Its two callers in this module
// (model_load_vertex_buffers @0x442d10 and model_dispose_vertex_buffers @0x442f00) write
// exactly two fields before the first call, an int16 zero at +0x04 and the group filter at
// +0x10, and reserve 0x14 bytes of stack for the object. The other three dwords are left
// uninitialized by both callers, so nothing in this module names them.
// ---------------------------------------------------------------------------
typedef struct tag_iterator {
    uint32_t unknown_00;           // 0x00
    int16_t next_index;            // 0x04 index the next call resumes from
    int16_t unknown_06;            // 0x06
    uint32_t unknown_08;           // 0x08
    uint32_t unknown_0c;           // 0x0c
    tag_group group_tag;           // 0x10 matched against all three group fields;
                                   //      -1 matches every tag
} tag_iterator;                    // size 0x14

// ---------------------------------------------------------------------------
// cache_file_slot
// One of six open map files. The stride 0x80c and the field offsets come from the
// &DAT_006a9428 + index*0x80c address arithmetic in cache_file_load, cache_file_unload,
// data_file_close, cache_file_open_by_name @0x443360, cache_file_slot_read_header @0x4435e0,
// cache_file_find_slot_by_name @0x443770 (compares slot+0x0c+0x20, i.e. header.name) and
// cache_file_find_oldest_slot @0x4437b0 (CompareFileTime on slot+0x04).
// cache_file_unload zero-fills 0x203 dwords == 0x80c bytes, which fixes the total size.
// ---------------------------------------------------------------------------
typedef struct cache_file_slot {
    void *file;                    // 0x00 HANDLE, -1 when the open failed
    file_time last_write_time;     // 0x04 zeroed when the header fails validation
    cache_file_header header;      // 0x0c
} cache_file_slot;                 // size 0x80c

// ---------------------------------------------------------------------------
// cache_io_completion
// The three dwords cache_io_request_new @0x442b20 copies out of ESI into the request. Proved
// by disassembly: sound_cache_page_allocate @0x4440e0 builds one on the stack at
// {&entry->loaded, decode_thunk, entry} and does lea esi,[that], and
// texture_cache_page_allocate @0x444800 builds {&entry->loaded, 0, 0}. The callback receives
// a pointer to the copy that lives inside the request, which is why the 15-byte thunk at
// 0x00443e00 reads its argument at +0x08 to recover the entry.
// ---------------------------------------------------------------------------
typedef struct cache_io_completion {
    uint8_t *flag;                 // 0x00 cleared on submit, set to 1 when the read finishes
    void (*procedure)(struct cache_io_completion *); // 0x04 optional, run before flag is set
    void *data;                    // 0x08 callback context
} cache_io_completion;             // size 0x0c

// ---------------------------------------------------------------------------
// cache_io_request
// Element of the 0x200-entry queue GlobalAlloc-ed at 0x6000 bytes by data_file_open. The
// first 0x14 bytes are a Win32 OVERLAPPED, which is why ReadFileEx can be handed the request
// itself in cache_io_thread_proc_async @0x443940; cache_io_completion_routine @0x443b00 is
// the APC and reads the embedded completion record back out of the same block at +0x24.
// The worker picks the pending request with the lowest (priority, offset).
// ---------------------------------------------------------------------------
typedef struct cache_io_request {
    uint32_t internal;             // 0x00 OVERLAPPED.Internal
    uint32_t internal_high;        // 0x04 OVERLAPPED.InternalHigh
    uint32_t offset;               // 0x08 OVERLAPPED.Offset, the file offset to read from
    uint32_t offset_high;          // 0x0c OVERLAPPED.OffsetHigh, always 0
    void *event;                   // 0x10 OVERLAPPED.hEvent, always 0
    uint32_t size;                 // 0x14 bytes to read
    void *destination;             // 0x18
    uint8_t priority;              // 0x1c raised to 1 in place by the blocking waiters
    uint8_t pending;               // 0x1d 1 from submit until the read completes
    uint8_t started;               // 0x1e 1 while the worker has the read in flight
    uint8_t unknown_1f;            // 0x1f
    uint8_t data_file_index;       // 0x20 cache_io_data_file
    uint8_t unknown_21[3];         // 0x21
    cache_io_completion completion; // 0x24 copy of the caller record
} cache_io_request;                // size 0x30

// ---------------------------------------------------------------------------
// data_file  (bitmaps.map / sounds.map)
// data_file_open @0x442840 zero-fills 0x40 bytes per instance, so the size is exact. The
// first 0x10 bytes are the on-disk header data_file_read_header @0x443b30 reads in one go
// and checks file_id against 1 for bitmaps and 2 for sounds.
// data_file_read_data_block @0x443ba0 fills +0x18..+0x23, data_file_read_offset_table
// @0x443c20 fills +0x10..+0x17.
// ---------------------------------------------------------------------------
typedef struct data_file {
    int32_t file_id;               // 0x00 1 = bitmaps, 2 = sounds (cache_io_data_file)
    int32_t data_offset;           // 0x04 file offset of the name block, the end of the payloads (DataFileHeader::names_offset)
    int32_t table_offset;          // 0x08 file offset of the reference table, and the end
                                   //      of the name block
    int32_t entry_count;           // 0x0c reference table entry count
    struct data_file_reference *references; // 0x10 GlobalAlloc of entry_count*0x0c
    int32_t reference_count;       // 0x14 copy of entry_count, written on success
    int32_t data_size;             // 0x18 table_offset - data_offset
    int32_t data_capacity;         // 0x1c the same value; written by the same function
    void *data;                    // 0x20 GlobalAlloc of data_size bytes
    uint32_t unknown_24;           // 0x24 zeroed by data_file_open, never read
    uint8_t unknown_28[16];        // 0x28
    const char *name;              // 0x38 "bitmaps" or "sounds", a literal, not copied
    void *file;                    // 0x3c HANDLE, -1 when the open failed
} data_file;                       // size 0x40

// One reference table entry of bitmaps.map / sounds.map (see halo/cache/data_map_file.hpp for the file layout).
typedef struct data_file_reference {
    uint32_t name_offset;          // 0x00 offset of the resource name inside the name block
    uint32_t size;                 // 0x04 payload size in bytes
    uint32_t file_offset;          // 0x08 file offset of the payload
} data_file_reference;             // size 0x0c

// ---------------------------------------------------------------------------
// sound_cache_entry
// The datum of the "pc sound" data_array (maximum_count 0x200, element size 0x10 from the
// index*0x10 + entries->data arithmetic in sound_cache_touch @0x443e10 and the two cache
// callbacks). It is the payload half of a page in the sound cache container; the page
// address and the page handle live on the SoundPermutation, not here.
// Field evidence: the in-use predicate at 0x00444060 reads +0x02, +0x05 and +0x06; the
// release procedure at 0x004440a0 reads +0x0c; sound_cache_page_allocate @0x4440e0 writes
// +0x08 and +0x0c; sound_cache_touch @0x443e10 reads +0x02, +0x03 and +0x05.
// ---------------------------------------------------------------------------
typedef struct sound_cache_entry {
    int16_t identifier;            // 0x00 datum_header
    uint8_t loaded;                // 0x02 the cache_io_completion flag for this page
    uint8_t decoded;               // 0x03 set once the samples have been decompressed
    uint8_t unknown_04;            // 0x04
    uint8_t lock_count;            // 0x05 nonzero pins the page; incremented by
                                   //      sound_cache_touch when asked to lock
    uint8_t playing;               // 0x06 nonzero pins the page
    uint8_t unknown_07;            // 0x07
    int16_t io_request_index;      // 0x08 index into the cache_io_requests queue
    int16_t unknown_0a;            // 0x0a
    struct SoundPermutation *permutation; // 0x0c owner, cleared out by the release procedure
} sound_cache_entry;               // size 0x10

// ---------------------------------------------------------------------------
// texture_cache_entry
// The datum of the "pc texture" data_array (maximum_count 0x1000, element size 0x10 from the
// index*0x10 + entries->data arithmetic in texture_cache_get @0x444550 and
// texture_cache_page_allocate @0x444800). Note the field order differs from
// sound_cache_entry: the request index sits at +0x02 and the completion flag at +0x04.
// Field evidence: the in-use predicate at 0x00444700 and the release procedure at
// 0x00444730 read +0x04 and +0x08; texture_cache_page_allocate writes +0x02, +0x08 and +0x0c.
// ---------------------------------------------------------------------------
typedef struct texture_cache_entry {
    int16_t identifier;            // 0x00 datum_header
    int16_t io_request_index;      // 0x02 index into the cache_io_requests queue
    uint8_t loaded;                // 0x04 the cache_io_completion flag for this read
    uint8_t converted;             // 0x05 set once the D3D texture has been created
    uint8_t unknown_06;            // 0x06
    uint8_t unknown_07;            // 0x07
    struct BitmapData *bitmap;     // 0x08 owner
    void *texture;                 // 0x0c the D3D texture object; texture_cache_get returns
                                   //      the address of this field, and the release
                                   //      procedure calls its vtable slot +0x08 to free it
} texture_cache_entry;             // size 0x10

// ---------------------------------------------------------------------------
// map_download_state
// The multiplayer map downloader, reached only through the pointer at 0x006869c0, which the
// image initializes to 0x006a8960 (bytes 60 89 6a 00 at file offset 0x2869c0). Four
// functions here touch it: cache_file_request_map @0x442640, cache_file_download_poll
// @0x442720, cache_file_download_stop @0x443510 and cache_file_download_finish @0x443540.
// Nothing else in the image references the object, by pointer or by address, so the eight
// fields below are all the module can establish; the object is owned elsewhere. Its extent
// is bounded above by the next global, cache_file_slots at 0x006a9428.
// ---------------------------------------------------------------------------
typedef struct map_download_state {
    uint8_t unknown_000[0x110];    // 0x000
    int32_t queued_file_count;     // 0x110 a value below 1 means nothing is downloading
    uint8_t unknown_114[0x7f4];    // 0x114
    uint32_t status_flags;         // 0x908 nonzero ends the download; bit 1 means cancelled
    uint8_t unknown_90c[0x48];     // 0x90c
    void *stop_event;              // 0x954 HANDLE, SetEvent asks the thread to stop
    void *finished_event;          // 0x958 HANDLE, signalled once the thread has stopped
    void *progress_event;          // 0x95c HANDLE, signalled while progress is readable
    void *thread;                  // 0x960 HANDLE, 0 when no download thread exists
    uint8_t unknown_964[0x28];     // 0x964
    uint8_t thread_busy;           // 0x98c cleared before a request, polled with Sleep(0x10)
    uint8_t unknown_98d[0x117];    // 0x98d
    float progress;                // 0xaa4 fraction in [0, 1]; clamped by the poller
    uint8_t unknown_aa8[0x20];     // 0xaa8
} map_download_state;              // size 0xac8

#pragma pack(pop)

// ---------------------------------------------------------------------------
// globals owned by this module
// ---------------------------------------------------------------------------
// The block from 0x006a8150 to 0x006ac554 is contiguous and, apart from the download object,
// entirely this module. Sizes chain exactly: the header ends at 0x006a8954, the download
// object ends at 0x006a9428, and six 0x80c-byte slots end at 0x006ac470.
//
// global 0x006a8150: uint8_t cache_file_loaded          set by cache_file_load, the guard in tag_lookup
// global 0x006a8154: cache_file_header cache_file_current_header   copy of the active slot header
// global 0x006a8954: cache_file_tag_header *tag_header  == the tag data base once loaded
// global 0x006a8958: void *structure_bsp_data           the ScenarioStructureBSPCompiledHeader of the resident bsp
// global 0x006869c0: map_download_state *map_download   initialized to 0x006a8960
// global 0x006a8960: map_download_state map_download_storage
// global 0x006a9428: cache_file_slot cache_file_slots[6]
// global 0x006ac470: uint8_t map_download_in_progress
// global 0x006ac472: int16_t map_download_slot_index    0xffff when idle
// global 0x006ac474: char map_download_name[0x20]       basename compared by cache_file_download_matches
// global 0x006ac494: int16_t cache_file_index           active slot, -1 when none
// global 0x006ac498: void *cache_io_event               HANDLE, auto-reset, wakes the worker
// global 0x006ac49c: void *cache_io_thread              HANDLE, 0x4000-byte stack
// global 0x006ac4a0: cache_io_request *cache_io_requests   GlobalAlloc of 0x6000 bytes
// global 0x006ac4a4: uint32_t unknown_006ac4a4          untouched gap
// global 0x006ac4a8: data_file sounds_data_file         file_id 2
// global 0x006ac4e8: data_file bitmaps_data_file        file_id 1
// global 0x006ac528: struct data_array *sound_cache_entries    "pc sound", 0x200 x 0x10
// global 0x006ac52c: void *sound_cache_base             == sound_cache_memory
// global 0x006ac530: struct cache *sound_cache          GlobalAlloc 0x387c == 0x7c + 0x200*0x1c
// global 0x006ac534: uint8_t sound_cache_initialized
// global 0x006ac538: struct data_array *texture_cache_entries  "pc texture", 0x1000 x 0x10
// global 0x006ac53c: void *texture_cache_base           == texture_cache_memory
// global 0x006ac540: struct cache *texture_cache        GlobalAlloc 0x1c07c == 0x7c + 0x1000*0x1c
// global 0x006ac544: uint32_t unknown_006ac544          untouched gap
// global 0x006ac548: void *map_memory                   VirtualAlloc at 0x40000000, 0x1b40000 bytes
// global 0x006ac54c: void *tag_data_base                constant 0x40440000
// global 0x006ac550: void *texture_cache_memory         VirtualAlloc 0x4000 bytes
// global 0x006ac554: void *sound_cache_memory           VirtualAlloc sound_cache_size_megabytes << 20
// global 0x006f17e4: uint32_t sound_cache_page_count    (megabytes << 20) >> 12
// global 0x006f17ec: void *sound_decode_buffer          grown on demand, freed by sound_cache_dispose
// global 0x006f17f0: int32_t sound_decode_buffer_size
// global 0x0087bc14: tag_instance *tag_instances        == tag_header->tags, 1049 references image-wide
//
// Globals this module reads but does not own:
// global 0x006869c4: int32_t sound_cache_size_megabytes  8 in the image; sizes both the sound
//                    cache pages and the VirtualAlloc behind them
// global 0x006ac900: char profile_directory[0x105]       k_profile_directory_storage_size
//                    (types/cseries.h): the shell zeroes 0x41 dwords + 1 byte (0x540ef9..
//                    0x540f05), then profile_path_initialize (0x449390, called at 0x540f06)
//                    fills it; formatted with "%s\\cache%03d.map"
// global 0x006f16d8: char map_path_prefix[]              formatted with "%s%s%s.map"
// global 0x006f17f6: uint8_t debug_texture_cache_prints  console toggle, only read here
// global 0x00721ef0: int32_t os_platform                 os_platform_identify @0x5427e0; < 3
//                    selects synchronous IO and the non-overlapped CreateFile flags
// global 0x0071d174: void *d3d_device                   rasterizer; guards every Release call
// 0x007c117c is not a global of its own: it is rasterizer_caps.max_streams (D3DCAPS9.MaxStreams,
//                    +0xbc of the d3d_caps9 at 0x007c10c0, types/rasterizer.h); the bsp vertex
//                    buffer loader compares it signed against 2 (0x4430b5 cmp ..,1 / jg)
// 0x007c118c likewise is rasterizer_caps.pixel_shader_version (D3DCAPS9 +0xcc), compared
//                    unsigned against 0xffff0101 (ps_1_1; 0x44308f cmp / jae)
// global 0x0065de00: int16_t rasterizer_vertex_sizes[]   38 00 20 00 14 00 08 00 44 00 ...
// global 0x0072520c: int32_t last_frame_milliseconds     main loop watchdog
// global 0x00722bbc: char *fatal_error_argument          shell_display_fatal_error_dialog
// global 0x00719cd4: uint32_t random_seed                math/random, see the note on 0x444af0
