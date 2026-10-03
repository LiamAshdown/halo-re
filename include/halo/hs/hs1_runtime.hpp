#pragma once

#include <stddef.h>
#include <stdint.h>

#include "tags.h"
#include "memory.h"
#include "win32.h"
#include "crt.h"
#include "hs.h"

namespace halo::hs {

/**
 * Script runtime support: node table allocation, dynamic global disposal, calling scripts by name and the hs
 * documentation dump.
 */
class ScriptRuntime {
public:
    static void allocate_script_node_table(void);
    static char call_script_by_name(char *name);
    static void dispose_dynamic_globals(void);
    static void doc(void);
};

}
