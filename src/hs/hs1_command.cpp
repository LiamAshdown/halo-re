#include "halo/hs/hs1_command.hpp"

#include <string.h>

namespace halo::hs {

/**
 * Linear lookup of a script function name in this group; returns null when the group does not implement it.
 */
const ScriptCommandEntry *ScriptCommandGroup::find(const char *script_name) const
{
    for (const ScriptCommandEntry *entry = begin(); entry != end(); ++entry) {
        if (strcmp(entry->script_name, script_name) == 0) {
            return entry;
        }
    }
    return nullptr;
}

}
