#include "halo/shell/runtime.hpp"
#include "halo/shell/layout.hpp"

extern "C" {
extern void *std_exception_vtable;
extern void *length_error_vtable;
extern void *out_of_range_vtable;
extern const char string_string_too_long[];
extern const char string_invalid_string_position[];
extern const char string_vector_too_long[];
extern const char string_invalid_vector_subscript[];
extern uint8_t length_error_throw_info[];
extern uint8_t out_of_range_throw_info[];
}

namespace halo::shell {

namespace {

/**
 * Builds a logic_error-family object around a copy of text, stamps the given vtable on it and throws it
 * with the matching throw info.
 */
[[noreturn]] void raise(const char *text, uint32_t count, void *vtable_address, uint8_t *throw_info)
{
    msvc_std_string message;
    hwreq_parse_exception exception;

    StdString(&message).init_empty();
    StdString(&message).assign_n(text, count);
    ParseException(&exception).construct(&message);
    exception.vtable = (uint32_t)vtable_address;
    _CxxThrowException(&exception, (_ThrowInfo *)throw_info);
}

}

/**
 * Copy constructor of std::exception: duplicates an owned message, shares a borrowed one.
 *
 * @address 0x627dd2
 */
void *StdException::copy_construct(const void *other)
{
    const std_exception *source = (const std_exception *)other;

    self->vftable = &std_exception_vtable;
    self->do_free = source->do_free;
    if (source->do_free != 0) {
        const char *message = source->what;
        char *copy = (char *)malloc(strlen(message) + 1);

        self->what = copy;
        if (copy != 0) {
            strcpy(copy, message);
        }
    } else {
        self->what = source->what;
    }
    return self;
}

/**
 * Destructor of std::exception: frees the message when the object owns it.
 *
 * @address 0x627e1c
 */
void StdException::destruct()
{
    self->vftable = &std_exception_vtable;
    if (self->do_free != 0) {
        free((void *)self->what);
    }
}

/**
 * std::runtime_error::what(): the message string's character data.
 */
const char *StdRuntimeError::what() const
{
    return self->message.capacity >= k_string_inline_capacity + 1 ? self->message.bx.pointer : self->message.bx.buffer;
}

/**
 * Copy constructor of the length_error flavour: copies as logic_error, then swaps in the length_error vtable.
 */
hwreq_parse_exception *ParseException::length_error_copy_construct(const hwreq_parse_exception *other)
{
    copy_construct(other);
    self->vtable = (uint32_t)&length_error_vtable;
    return self;
}

/**
 * Copy constructor of the out_of_range flavour: copies as logic_error, then swaps in the out_of_range vtable.
 */
hwreq_parse_exception *ParseException::out_of_range_copy_construct(const hwreq_parse_exception *other)
{
    copy_construct(other);
    self->vtable = (uint32_t)&out_of_range_vtable;
    return self;
}

/**
 * std::string _Xran: throws out_of_range("invalid string position").
 *
 * @address 0x638e74
 */
void StdThrow::string_out_of_range()
{
    raise(string_invalid_string_position, cstr_length(string_invalid_string_position), &out_of_range_vtable,
          out_of_range_throw_info);
}

/**
 * std::string _Xlen: throws length_error("string too long").
 *
 * @address 0x638eb4
 */
void StdThrow::string_too_long()
{
    raise(string_string_too_long, cstr_length(string_string_too_long), &length_error_vtable, length_error_throw_info);
}

/**
 * Throws length_error("vector<T> too long").
 *
 * @address 0x57c130
 */
void StdThrow::vector_too_long()
{
    raise(string_vector_too_long, 0x12, &length_error_vtable, length_error_throw_info);
}

/**
 * Throws out_of_range("invalid vector<T> subscript").
 *
 * @address 0x57b9e0
 */
void StdThrow::vector_subscript()
{
    raise(string_invalid_vector_subscript, 0x1b, &out_of_range_vtable, out_of_range_throw_info);
}

/**
 * Throws out_of_range("invalid map/set<T> iterator"), raised when the head sentinel is erased.
 */
void StdThrow::invalid_map_iterator()
{
    raise("invalid map/set<T> iterator", 0x1b, &out_of_range_vtable, out_of_range_throw_info);
}

/**
 * Throws length_error("map/set<T> too long"), raised when a map reaches its maximum size.
 */
void StdThrow::map_too_long()
{
    raise("map/set<T> too long", 0x13, &length_error_vtable, length_error_throw_info);
}

}
