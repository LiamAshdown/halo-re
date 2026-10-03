#include "halo/shell/runtime.hpp"
#include "halo/shell/layout.hpp"
#include "halo/core/link.hpp"
#include "halo/shell/vars.hpp"
#include "halo/shell/api.hpp"

static auto &logic_error_vtable = halo::link::ref<void *>(halo::shell::vars().logic_error_vtable);
static auto &length_error_vtable = halo::link::ref<void *>(halo::shell::vars().length_error_vtable);
static auto &out_of_range_vtable = halo::link::ref<void *>(halo::shell::vars().out_of_range_vtable);

namespace halo::shell {

/**
 * Builds the logic_error object with a copy of message.
 *
 * @address 0x5782b0
 */
hwreq_parse_exception *ParseException::construct(const msvc_std_string *message)
{
    self->dofree = 0;
    self->legacy_what = 0;

    self->vtable = (uint32_t)&logic_error_vtable;
    self->message.size = 0;
    self->message.capacity = k_string_inline_capacity;
    self->message.buffer.inline_buffer[0] = 0;

    StdString(&self->message).assign_substr(message, 0, k_string_npos);

    return self;
}

/**
 * Copy constructor: copies the std::exception base and the message string.
 *
 * @address 0x57bc20
 */
hwreq_parse_exception *ParseException::copy_construct(const hwreq_parse_exception *other)
{
    StdException(self).copy_construct(other);
    self->vtable = (uint32_t)&logic_error_vtable;
    self->message.capacity = k_string_inline_capacity;
    self->message.size = 0;
    self->message.buffer.inline_buffer[0] = 0;
    StdString(&self->message).assign_substr(&other->message, 0, k_string_npos);
    return self;
}

/**
 * Destroys the message string and the std::exception base.
 *
 * @address 0x578310
 */
void ParseException::destruct()
{
    self->vtable = (uint32_t)&logic_error_vtable;

    if (self->message.capacity > k_string_inline_capacity) {
        free((void *)self->message.buffer.heap_buffer);
    }
    self->message.capacity = k_string_inline_capacity;
    self->message.size = 0;
    self->message.buffer.inline_buffer[0] = 0;

    StdException(self).destruct();
}

/**
 * Destroys the exception and frees its storage when bit 0 of free_flag is set.
 *
 * @address 0x578390
 */
void ParseException::scalar_deleting_destruct(uint8_t free_flag)
{
    destruct();
    if (free_flag & 1) {
        free(self);
    }
}

/**
 * Destructor of the length_error flavour: restores its vtable, then runs the logic_error
 * destructor.
 *
 * @address 0x5783b0
 */
void ParseException::length_error_destruct()
{
    *(void **)self = &length_error_vtable;
    destruct();
}

/**
 * Scalar deleting destructor of the length_error flavour.
 *
 * @address 0x5783c0
 */
void ParseException::length_error_scalar_deleting_destruct(uint8_t free_flag)
{
    length_error_destruct();
    if (free_flag & 1) {
        free(self);
    }
}

/**
 * Destructor of the out_of_range flavour: restores its vtable, then runs the logic_error
 * destructor.
 *
 * @address 0x5783e0
 */
void ParseException::out_of_range_destruct()
{
    *(void **)self = &out_of_range_vtable;
    destruct();
}

/**
 * Scalar deleting destructor of the out_of_range flavour.
 *
 * @address 0x5783f0
 */
void ParseException::out_of_range_scalar_deleting_destruct(uint8_t free_flag)
{
    out_of_range_destruct();
    if (free_flag & 1) {
        free(self);
    }
}

}
