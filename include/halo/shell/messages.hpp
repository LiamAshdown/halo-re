/**
 * @file include/halo/shell/messages.hpp
 * The shell's user-visible English messages: start-up and graphics errors, the crash reporter captions and the
 * command-line help. The engine refers to a message by its id; the ids are the ones the game's code and network
 * messages have always used.
 */
#pragma once

#include <cstdint>

namespace halo::shell {

/**
 * The English text for message `id`, or nullptr when the id has no text. Ids that name help links in other
 * builds (web pages that no longer exist) have no text here.
 */
const char *shell_message(uint32_t id);

/** The command-line switch summary shown for -help and -?. */
const char *shell_usage_text();

}  // namespace halo::shell
