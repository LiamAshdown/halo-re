#include "halo/shell/hwreq.hpp"
#include "halo/shell/layout.hpp"
#include "halo/core/link.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"

static auto &hwreq_parser_vtable_instance = halo::link::ref<hwreq_parser_vtable>(halo::shell::vars().hwreq_parser_vtable_instance);
static auto &hwreq_open_error_text = halo::link::ref<char []>(halo::shell::vars().hwreq_open_error_text);
static auto &hwreq_cannot_find_format = halo::link::ref<const char []>(halo::shell::vars().hwreq_cannot_find_format);
static auto &hwreq_config_file_suffix = halo::link::ref<const char []>(halo::shell::vars().hwreq_config_file_suffix);
static auto &hwreq_version_root_block = halo::link::ref<const char []>(halo::shell::vars().hwreq_version_root_block);

namespace halo::shell {

/**
 * Default constructs the parser in place: installs the vtable, empties the five strings and
 * allocates the head sentinel of both maps.
 *
 * @address 0x579ef0
 */
hwreq_parser *HwreqParser::construct()
{
    hwreq_map_node *head;

    self->vtable = (uint32_t)&hwreq_parser_vtable_instance;

    self->error_message.capacity = k_msvc_string_inline_capacity;
    self->error_message.size = 0;
    self->error_message.buffer.inline_buffer[0] = 0;

    self->graphics_device_name.capacity = k_msvc_string_inline_capacity;
    self->graphics_device_name.size = 0;
    self->graphics_device_name.buffer.inline_buffer[0] = 0;

    self->graphics_vendor_name.capacity = k_msvc_string_inline_capacity;
    self->graphics_vendor_name.size = 0;
    self->graphics_vendor_name.buffer.inline_buffer[0] = 0;

    self->sound_device_name.capacity = k_msvc_string_inline_capacity;
    self->sound_device_name.size = 0;
    self->sound_device_name.buffer.inline_buffer[0] = 0;

    self->sound_vendor_name.capacity = k_msvc_string_inline_capacity;
    self->sound_vendor_name.size = 0;
    self->sound_vendor_name.buffer.inline_buffer[0] = 0;

    head = TreeNode::allocate_head();
    self->property_sets.head = (uint32_t)head;
    head->is_nil = 1;
    head->parent = (uint32_t)head;
    head->left = (uint32_t)head;
    head->right = (uint32_t)head;
    self->property_sets.size = 0;

    head = TreeNode::allocate_head();
    self->graphic_detail_sets.head = (uint32_t)head;
    head->is_nil = 1;
    head->parent = (uint32_t)head;
    head->left = (uint32_t)head;
    head->right = (uint32_t)head;
    self->graphic_detail_sets.size = 0;

    self->flags = 0;
    self->requirements = 0;

    return self;
}

/**
 * Allocates a hardware-requirements parser object and default-constructs it; returns NULL if the
 * allocation fails.
 *
 * @address 0x57b4c0
 */
hwreq_parser *HwreqParser::create()
{
    hwreq_parser *parser;

    parser = (hwreq_parser *)malloc(k_hwreq_parser_size);
    if (parser == 0) {
        return 0;
    }
    return HwreqParser(parser).construct();
}

/**
 * Destroys the parser: frees the flags and requirements sets, every property set owned by the
 * property set map, both maps and the five strings.
 *
 * @address 0x57a010
 */
void HwreqParser::destruct()
{
    hwreq_map_node *head;
    hwreq_map_node *node;
    hwreq_property_set *set;
    hwreq_string_pair *pair;
    hwreq_string_pair *pair_end;

    self->vtable = (uint32_t)&hwreq_parser_vtable_instance;

    if (self->flags != 0) {
        PropertySet((hwreq_property_set *)self->flags).destroy_flags();
        free((void *)self->flags);
    }
    if (self->requirements != 0) {
        PropertySet((hwreq_property_set *)self->requirements).destroy_flags();
        free((void *)self->requirements);
    }

    head = (hwreq_map_node *)self->property_sets.head;
    for (node = (hwreq_map_node *)head->left; node != head;) {
        set = (hwreq_property_set *)node->value;
        if (set != 0) {
            pair = (hwreq_string_pair *)set->flags.first;
            if (pair != 0) {
                pair_end = (hwreq_string_pair *)set->flags.last;
                for (; pair != pair_end; pair++) {
                    StringPair(pair).destroy();
                }
                free((void *)set->flags.first);
            }
            set->flags.first = 0;
            set->flags.last = 0;
            set->flags.end = 0;
            free(set);
        }
        TreeIterator(&node).increment();
    }

    head = (hwreq_map_node *)self->graphic_detail_sets.head;
    StdMap(&self->graphic_detail_sets).erase_range(&node, (hwreq_map_node *)head->left, head);
    free((void *)self->graphic_detail_sets.head);
    self->graphic_detail_sets.head = 0;
    self->graphic_detail_sets.size = 0;

    head = (hwreq_map_node *)self->property_sets.head;
    StdMap(&self->property_sets).erase_range(&node, (hwreq_map_node *)head->left, head);
    free((void *)self->property_sets.head);
    self->property_sets.head = 0;
    self->property_sets.size = 0;

    if (self->sound_vendor_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)self->sound_vendor_name.buffer.heap_buffer);
    }
    self->sound_vendor_name.capacity = k_msvc_string_inline_capacity;
    self->sound_vendor_name.size = 0;
    self->sound_vendor_name.buffer.inline_buffer[0] = 0;

    if (self->sound_device_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)self->sound_device_name.buffer.heap_buffer);
    }
    self->sound_device_name.capacity = k_msvc_string_inline_capacity;
    self->sound_device_name.size = 0;
    self->sound_device_name.buffer.inline_buffer[0] = 0;

    if (self->graphics_vendor_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)self->graphics_vendor_name.buffer.heap_buffer);
    }
    self->graphics_vendor_name.capacity = k_msvc_string_inline_capacity;
    self->graphics_vendor_name.size = 0;
    self->graphics_vendor_name.buffer.inline_buffer[0] = 0;

    if (self->graphics_device_name.capacity > k_msvc_string_inline_capacity) {
        free((void *)self->graphics_device_name.buffer.heap_buffer);
    }
    self->graphics_device_name.capacity = k_msvc_string_inline_capacity;
    self->graphics_device_name.size = 0;
    self->graphics_device_name.buffer.inline_buffer[0] = 0;

    if (self->error_message.capacity > k_msvc_string_inline_capacity) {
        free((void *)self->error_message.buffer.heap_buffer);
    }
    self->error_message.capacity = k_msvc_string_inline_capacity;
    self->error_message.size = 0;
    self->error_message.buffer.inline_buffer[0] = 0;
}

/**
 * Destructs the parser and frees its storage; null is ignored.
 *
 * @address 0x5786a0
 */
void HwreqParser::scalar_deleting_destruct()
{
    if (self != 0) {
        destruct();
        free(self);
    }
}

/**
 * Character data of the latched error message.
 *
 * @address 0x5788e0
 */
char *HwreqParser::error_message_text()
{
    if (self->error_message.capacity > k_string_inline_capacity) {
        return (char *)self->error_message.buffer.heap_buffer;
    }
    return self->error_message.buffer.inline_buffer;
}

/**
 * Number of flag entries in the flags property set the parser filled for this machine.
 *
 * @address 0x5786c0
 */
uint32_t HwreqParser::flag_count()
{
    hwreq_property_set *set = (hwreq_property_set *)self->flags;

    if (set->flags.first == 0) {
        return 0;
    }
    return (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair));
}

/**
 * Name of the flag at index. Raises the vector subscript out_of_range error when index is past the
 * end.
 *
 * @address 0x5786f0
 */
char *HwreqParser::flag_name(uint32_t index)
{
    hwreq_property_set *set = (hwreq_property_set *)self->flags;
    hwreq_string_pair *pairs = (hwreq_string_pair *)set->flags.first;

    if (pairs == 0 || (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair)) <= index) {
        StdThrow::vector_subscript();
    }
    return StdString(&pairs[index].first).data();
}

/**
 * Value of the flag at index. Raises the vector subscript out_of_range error when index is past the
 * end.
 *
 * @address 0x578740
 */
char *HwreqParser::flag_value(uint32_t index)
{
    hwreq_property_set *set = (hwreq_property_set *)self->flags;
    hwreq_string_pair *pairs = (hwreq_string_pair *)set->flags.first;

    if (pairs == 0 || (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair)) <= index) {
        StdThrow::vector_subscript();
    }
    return StdString(&pairs[index].second).data();
}

/**
 * The property set holding the flags that apply to this machine.
 *
 * @address 0x578860
 */
hwreq_property_set *HwreqParser::flags_set()
{
    return (hwreq_property_set *)self->flags;
}

/**
 * Character data of the graphics device name found by the vendor block.
 *
 * @address 0x578870
 */
char *HwreqParser::graphics_device_name_text()
{
    if (self->graphics_device_name.capacity > k_string_inline_capacity) {
        return (char *)self->graphics_device_name.buffer.heap_buffer;
    }
    return self->graphics_device_name.buffer.inline_buffer;
}

/**
 * Character data of the graphics vendor name found by the vendor block.
 *
 * @address 0x578880
 */
char *HwreqParser::graphics_vendor_name_text()
{
    return StdString(&self->graphics_vendor_name).data();
}

/**
 * Number of entries in the Requirements section property set.
 *
 * @address 0x578790
 */
uint32_t HwreqParser::requirement_count()
{
    hwreq_property_set *set = (hwreq_property_set *)self->requirements;

    if (set->flags.first == 0) {
        return 0;
    }
    return (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair));
}

/**
 * Name of the requirement at index. Raises the vector subscript out_of_range error when index is
 * past the end.
 *
 * @address 0x5787c0
 */
char *HwreqParser::requirement_name(uint32_t index)
{
    hwreq_property_set *set = (hwreq_property_set *)self->requirements;
    hwreq_string_pair *pairs = (hwreq_string_pair *)set->flags.first;

    if (pairs == 0 || (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair)) <= index) {
        StdThrow::vector_subscript();
    }
    return StdString(&pairs[index].first).data();
}

/**
 * Value of the requirement at index. Raises the vector subscript out_of_range error when index is
 * past the end.
 *
 * @address 0x578810
 */
char *HwreqParser::requirement_value(uint32_t index)
{
    hwreq_property_set *set = (hwreq_property_set *)self->requirements;
    hwreq_string_pair *pairs = (hwreq_string_pair *)set->flags.first;

    if (pairs == 0 || (uint32_t)((int32_t)(set->flags.last - set->flags.first) / (int32_t)sizeof(hwreq_string_pair)) <= index) {
        StdThrow::vector_subscript();
    }
    return StdString(&pairs[index].second).data();
}

/**
 * Character data of the sound device name found by the audiovendor block.
 *
 * @address 0x578890
 */
char *HwreqParser::sound_device_name_text()
{
    return StdString(&self->sound_device_name).data();
}

/**
 * Character data of the sound vendor name found by the audiovendor block.
 *
 * @address 0x5788b0
 */
char *HwreqParser::sound_vendor_name_text()
{
    return StdString(&self->sound_vendor_name).data();
}

/**
 * Whether the parser has latched an error message.
 *
 * @address 0x5788d0
 */
uint8_t HwreqParser::has_error()
{
    return self->error_reported;
}

/**
 * Looks name up in the parser's property_sets map ("propertyset" definitions) and returns the
 * registered set, or NULL when there is no set of that name.
 *
 * @address 0x5788f0
 */
hwreq_property_set *HwreqParser::find_property_set(const char *name)
{
    msvc_std_string key;
    hwreq_map_node *node;
    hwreq_property_set *result;
    uint32_t length = 0;

    key.capacity = k_msvc_string_inline_capacity;
    key.size = 0;
    key.buffer.inline_buffer[0] = 0;
    while (name[length] != 0) {
        length++;
    }
    StdString(&key).assign_n(name, length);

    node = StdMap(&self->property_sets).find(&key);
    if (node == (hwreq_map_node *)self->property_sets.head) {
        result = 0;
    } else {
        result = (hwreq_property_set *)node->value;
    }

    if (key.capacity >= k_string_inline_capacity + 1) {
        free((void *)key.buffer.heap_buffer);
    }
    return result;
}

/**
 * Parses the config.txt at path against the given sound device, display adapter, caps and memory
 * figures: Requirements section first, then propertysets, the matching vendor and audiovendor
 * blocks and the applytoall blocks. Returns false and leaves a message in the error string on
 * failure.
 *
 * @address 0x579bd0
 */
uint8_t HwreqParser::parse(const char *path, const shell_sound_device *sound_device, const d3d_adapter_identifier9 *adapter, const d3d_caps9 *caps, uint32_t memory, uint32_t video_memory, uint32_t cpu_speed)
{
    uint32_t *driver_version = (uint32_t *)((uint8_t *)&self->adapter + 0x420);
    char directory[0x104];
    char *end;
    void *file;
    uint32_t size;
    uint32_t bytes_read;
    char *buffer;
    int32_t i;

    if (self->flags == 0) {
        self->flags = (uint32_t)PropertySet::create(self);
    }
    if (self->requirements == 0) {
        self->requirements = (uint32_t)PropertySet::create(self);
    }
    self->sound_device = *sound_device;
    self->memory = memory;
    self->cpu_speed = cpu_speed;
    self->video_memory = video_memory;
    self->adapter = *adapter;
    self->caps = *caps;
    self->error_reported = 0;

    if ((driver_version[0] | driver_version[1]) == 0) {
        uint32_t handle;
        uint32_t info_size = GetFileVersionInfoSizeA((const char *)&self->adapter, (LPDWORD)&handle);

        if (info_size != 0) {
            void *info = malloc(info_size);
            void *fixed;
            uint32_t fixed_length;

            if (GetFileVersionInfoA((const char *)&self->adapter, handle, info_size, info) &&
                VerQueryValueA(info, hwreq_version_root_block, &fixed, &fixed_length)) {
                uint32_t fixed_info[0xd];

                for (i = 0; i < 0xd; i++) {
                    fixed_info[i] = ((uint32_t *)fixed)[i];
                }
                driver_version[1] = fixed_info[2];
                driver_version[0] = fixed_info[3];
            }
            free(info);
        }
    }

    file = CreateFileA(path, win32::k_generic_read, win32::k_file_share_read, 0, win32::k_open_existing, 0,
                       0);
    if (file == win32::invalid_handle()) {
        GetCurrentDirectoryA(sizeof(directory), directory);
        for (end = directory; *end; end++) {
        }
        for (i = 0; i < 12; i++) {
            end[i] = hwreq_config_file_suffix[i];
        }
        sprintf(hwreq_open_error_text, hwreq_cannot_find_format, directory);
        for (end = hwreq_open_error_text; *end; end++) {
        }
        StdString(&self->error_message).assign_n(hwreq_open_error_text, (uint32_t)(end - hwreq_open_error_text));
        return 0;
    }

    size = GetFileSize(file, 0);
    buffer = (char *)malloc(size + 0x10);
    self->file_buffer = (uint32_t)buffer;
    ReadFile(file, buffer, size, (LPDWORD)&bytes_read, 0);
    CloseHandle(file);
    buffer[size] = '\r';
    self->cursor = (uint32_t)buffer;
    self->line_start = (uint32_t)buffer;
    self->line_number = 1;
    self->end = (uint32_t)(buffer + size);

    if (!find_requirements_section()) {
        free((void *)self->file_buffer);
        return 0;
    }
    rewind();
    if (!parse_propertyset_directive() || !scan_for_applytoall_or_vendor()) {
        free((void *)self->file_buffer);
        return 0;
    }
    rewind();
    if (!parse_vendor_block()) {
        free((void *)self->file_buffer);
        return 0;
    }
    rewind();
    if (!parse_audiovendor_block() || !scan_for_applytoall()) {
        free((void *)self->file_buffer);
        return 0;
    }
    free((void *)self->file_buffer);
    return 1;
}

/**
 * Moves the cursor back to the start of the file buffer and resets the line counters.
 */
void HwreqParser::rewind()
{
    self->cursor = self->file_buffer;
    self->line_start = self->file_buffer;
    self->line_number = 1;
}

}
