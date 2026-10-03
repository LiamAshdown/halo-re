#pragma once

namespace halo {

/**
 * Hands a string literal to an engine entry point that takes a mutable `char *` but only reads it.
 *
 * The one place the const is dropped, so the callers carry no raw casts; it goes away when those entry points take
 * `const char *`.
 */
inline char *mutable_literal(const char *text) noexcept { return const_cast<char *>(text); }

}  // namespace halo
