#pragma once

#include <stddef.h>
#include <stdint.h>

namespace halo::hs {

/**
 * Signature shared by every hs script-function evaluate handler: the function table index, the thread datum
 * index and the first-visit flag.
 */
using ScriptEvaluateFn = void (*)(int16_t function_index, uint32_t thread_index, char first);

/**
 * One row of a command registry: the script-visible function name and the handler that evaluates it.
 */
struct ScriptCommandEntry {
    const char *script_name;
    ScriptEvaluateFn evaluate;
};

/**
 * Command registry for one group of related hs script functions (Command pattern). Groups are built once in
 * static storage and only used for lookup; evaluation itself still goes through the engine's function table.
 */
class ScriptCommandGroup {
public:
    constexpr ScriptCommandGroup(const ScriptCommandEntry *entries, size_t count)
        : entries_(entries), count_(count) {}

    size_t size() const { return count_; }
    const ScriptCommandEntry *begin() const { return entries_; }
    const ScriptCommandEntry *end() const { return entries_ + count_; }

    const ScriptCommandEntry *find(const char *script_name) const;

private:
    const ScriptCommandEntry *entries_;
    size_t count_;
};

}
