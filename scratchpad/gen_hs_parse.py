import os, textwrap
os.chdir(r'C:\Users\Liam-\halo-re')

COMMON = '''#include "tags.h"
#include "memory.h"
#include "hs.h"
%(extra_inc)s
extern data_array *hs_syntax_data; // 0x0087a474
extern hs_function_definition *hs_function_definitions[k_hs_function_count]; // 0x00688b58
extern char *hs_compile_error;          // 0x006b14d4
extern int32_t hs_compile_error_offset; // 0x006b14d8
extern char hs_parse(datum_index node_index, hs_type_t expected_type); // 0x00486420
%(externs)s
static hs_syntax_node *syntax_node(datum_index node_index)
{
    return (hs_syntax_node *)hs_syntax_data->data + (node_index & 0xffff);
}
'''

PRIM = ('extern char hs_parse_primitive(datum_index node_index); // 0x00486480, blam-cc: EDI node\n'
        'extern char hs_parse_nonprimitive(datum_index node_index); // 0x00486710\n')
ERRBUF = 'extern char hs_compile_error_buffer[0x100]; // 0x006b14dc\n'
PARAMS = ('extern char hs_get_parameter_indices(char *function_name, int16_t required_count, datum_index node_index,\n'
          '    datum_index *out_indices); // 0x00484fb0, ECX node, EBX out\n')

# the typed argument loop shared by and/or, the arithmetic operators and the rcon family (inlined in each)
TYPED_LOOP = '''
// Parses one argument as `type` unless it already has a type: primitives get the type in index_union too and go
// through hs_parse_primitive, lists through hs_parse_nonprimitive (the inlined hs_parse body).
static char parse_typed_argument(datum_index argument, hs_type_t type)
{
    hs_syntax_node *node = syntax_node(argument);

    if (node->type != 0) {
        return 1;
    }
    node->type = type;
    if ((node->flags & 1) != 0) {
        node->index_union = type;
        return hs_parse_primitive(argument);
    }
    return hs_parse_nonprimitive(argument);
}
'''

FILES = [
 ('hs_parse_begin', 0x484600, 367, 'begin and begin_random',
  "every argument is parsed in order and the first failure returns 0. For begin (function index 0) every argument but the last is parsed as void and the last as the block's own type; begin_random parses all with the block's type. An untyped block takes the type of the argument that decided it (begin: the last, begin_random: each). No arguments gives \"a statement block must contain at least one argument.\" (formatted into the error buffer with the function name, which the format does not use); begin_random with more than 32 gives its own error. Both at the call's source offset.",
  ERRBUF + '#include <stdio.h>\n', '',
  '''char hs_parse_begin(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index argument = syntax_node(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;
    char ok = 1;

    while (argument != 0xffffffff) {
        hs_syntax_node *node = syntax_node(argument);
        datum_index next = node->next_node;

        if (function_index == 0) {
            ok = hs_parse(argument, next == 0xffffffff ? call->type : 4);
            if (next == 0xffffffff && call->type == 0 && ok) {
                call->type = syntax_node(argument)->type;
            }
        } else {
            ok = hs_parse(argument, call->type);
            if (call->type == 0 && ok) {
                call->type = syntax_node(argument)->type;
            }
        }
        argument = next;
        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 1) {
        sprintf(hs_compile_error_buffer, "a statement block must contain at least one argument.", // 0x00665cc4
            hs_function_definitions[function_index]->name);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    if (count > 0x20 && function_index == 1) {
        hs_compile_error = "begin_random can take a maximum of 32 arguments (matt can increase this.)"; // 0x00665c78
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    return 1;
}
'''),
 ('hs_parse_cond', 0x484b40, 149, 'cond',
  "rewrites the cond into a nested if chain (hs_parse_cond_recursive 0x4848f0 from the first clause); on success copies the new node over the call node, keeping the call's identifier and next link, and parses it with the call's original type. -1 from the rewrite returns 0.",
  'extern datum_index hs_parse_cond_recursive(datum_index cond_node_index, datum_index pair_index); // 0x004848f0\n', '',
  '''char hs_parse_cond(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index rewritten = hs_parse_cond_recursive(node_index, syntax_node(*(datum_index *)&call->data)->next_node);
    hs_syntax_node *replacement;
    datum_index next;
    hs_type_t type;
    int16_t identifier;

    if (rewritten == 0xffffffff) {
        return 0;
    }
    call = syntax_node(node_index);
    next = call->next_node;
    type = call->type;
    identifier = call->identifier;
    replacement = syntax_node(rewritten);
    replacement->next_node = next;
    *call = *replacement;
    call->identifier = identifier;
    return hs_parse(node_index, type);
}
'''),
 ('hs_parse_logical', 0x484db0, 225, 'and and or',
  "parses every argument as boolean (primitives through hs_parse_primitive, lists through hs_parse_nonprimitive; already typed ones are taken as parsed), stopping at the first failure; fewer than two arguments gives \"the %s call requires at least 2 arguments.\" (0x00665aa4) at the call's source offset.",
  PRIM + ERRBUF + '#include <stdio.h>\n', TYPED_LOOP,
  '''char hs_parse_logical(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index argument = syntax_node(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;

    for (; argument != 0xffffffff; argument = syntax_node(argument)->next_node) {
        char ok = parse_typed_argument(argument, 5);

        count++;
        if (!ok) {
            return 0;
        }
    }
    if (count < 2) {
        sprintf(hs_compile_error_buffer, "the %s call requires at least 2 arguments.", hs_function_definitions[function_index]->name);
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = syntax_node(node_index)->source_offset;
        return 0;
    }
    return 1;
}
'''),
 ('hs_parse_arithmetic', 0x484ea0, 268, '+ - * / min max',
  "parses every argument as real, stopping at the first failure. Fewer than two arguments, or (for /, function index 0xa) more than two -- checked even after a failed argument, overwriting its error -- gives \"the %s call requires %s2 arguments.\" (0x00665a74) with \"at least \" or, for /, \"\", at the call's source offset.",
  PRIM + ERRBUF + '#include <stdio.h>\n', TYPED_LOOP,
  '''char hs_parse_arithmetic(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index argument = syntax_node(*(datum_index *)&call->data)->next_node;
    int16_t count = 0;
    char ok = 1;

    for (; argument != 0xffffffff; argument = syntax_node(argument)->next_node) {
        ok = parse_typed_argument(argument, 6);
        count++;
        if (!ok) {
            break;
        }
    }
    if ((ok && count < 2) || (function_index == 0xa && count > 2)) {
        sprintf(hs_compile_error_buffer, "the %s call requires %s2 arguments.", hs_function_definitions[function_index]->name,
            function_index == 0xa ? "" : "at least ");
        hs_compile_error = hs_compile_error_buffer;
        hs_compile_error_offset = syntax_node(node_index)->source_offset;
        return 0;
    }
    return ok;
}
'''),
 ('hs_parse_sleep', 0x485280, 141, 'sleep',
  "the first argument (required: else \"the sleep call requires a time and, optionally, a script name.\" at the call's source offset) parses as short, an optional second as script.",
  '', '',
  '''char hs_parse_sleep(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index time = syntax_node(*(datum_index *)&call->data)->next_node;
    datum_index script;

    if (time == 0xffffffff) {
        hs_compile_error = "the sleep call requires a time and, optionally, a script name."; // 0x00665a10
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    if (!hs_parse(time, 7)) {
        return 0;
    }
    script = syntax_node(time)->next_node;
    if (script != 0xffffffff && !hs_parse(script, 0xa)) {
        return 0;
    }
    return 1;
}
'''),
 ('hs_parse_sleep_until', 0x485310, 170, 'sleep_until',
  "the condition (required: else \"the sleep_until call requires a condition and, optionally, a period.\" at the call's source offset) parses as boolean, an optional period as short and an optional third argument as long; returns the last parse result.",
  '', '',
  '''char hs_parse_sleep_until(int16_t function_index, datum_index node_index)
{
    hs_syntax_node *call = syntax_node(node_index);
    datum_index condition = syntax_node(*(datum_index *)&call->data)->next_node;
    datum_index period;
    datum_index timeout;
    char ok;

    if (condition == 0xffffffff) {
        hs_compile_error = "the sleep_until call requires a condition and, optionally, a period."; // 0x006659c8
        hs_compile_error_offset = call->source_offset;
        return 0;
    }
    period = syntax_node(condition)->next_node;
    ok = hs_parse(condition, 5);
    if (!ok || period == 0xffffffff) {
        return ok;
    }
    timeout = syntax_node(period)->next_node;
    ok = hs_parse(period, 7);
    if (!ok || timeout == 0xffffffff) {
        return ok;
    }
    return hs_parse(timeout, 8);
}
'''),
 ('hs_parse_wake', 0x4853c0, 160, 'wake',
  "one argument parsed as a script; a static or stub script (scenario script type 3 or 4) gives \"this static script cannot be awakened.\" at the argument's source offset.",
  PARAMS + 'extern Scenario *global_scenario; // 0x00746f8c\n', '',
  '''char hs_parse_wake(int16_t function_index, datum_index node_index)
{
    datum_index argument;
    hs_syntax_node *node;
    ScenarioScript *script;

    if (!hs_get_parameter_indices(hs_function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    node = syntax_node(argument);
    if (!hs_parse(argument, 0xa)) {
        return 0;
    }
    script = (ScenarioScript *)global_scenario->scripts.pointer + node->data.short_value;
    if (script->script_type == 3 || script->script_type == 4) {
        hs_compile_error = "this static script cannot be awakened."; // 0x006659a0
        hs_compile_error_offset = node->source_offset;
        return 0;
    }
    return 1;
}
'''),
 ('hs_parse_unit', 0x4854f0, 69, 'unit',
  "one argument parsed as hs type 0x25 (object list).",
  PARAMS, '',
  '''char hs_parse_unit(int16_t function_index, datum_index node_index)
{
    datum_index argument;

    if (!hs_get_parameter_indices(hs_function_definitions[function_index]->name, 1, node_index, &argument)) {
        return 0;
    }
    return hs_parse(argument, 0x25);
}
'''),
 ('hs_parse_string_arguments', 0x487530, 137, 'the 18 console-only string functions (rcon, sv_ban, sv_name, ai_debug_communication_*, ...)',
  "every argument is parsed as string (untyped ones only), stopping at the first failure; no argument count check.",
  PRIM, TYPED_LOOP,
  '''char hs_parse_string_arguments(int16_t function_index, datum_index node_index)
{
    datum_index argument = syntax_node(*(datum_index *)&syntax_node(node_index)->data)->next_node;

    for (; argument != 0xffffffff; argument = syntax_node(argument)->next_node) {
        if (!parse_typed_argument(argument, 9)) {
            return 0;
        }
    }
    return 1;
}
'''),
]

for name, addr, size, who, note, externs, helpers, body in FILES:
    lines = textwrap.wrap('WRITTEN 2026-09-28 from objdump 0x%x..0x%x: %s' % (addr, addr + size - 1, note), 113)
    wr = ''.join(('// ' if k == 0 else '//   ') + l + '\n' for k, l in enumerate(lines))
    extra_inc = ''
    if '#include <stdio.h>' in externs:
        externs = externs.replace('#include <stdio.h>\n', '')
        extra_inc = '#include <stdio.h>\n'
    src = ('// %s  (not a Ghidra function; the hs parse procedure of %s, reached through the function records\' +0x08\n'
           '//   slot; no C existed, so console input using it trapped as unlisted_%x)\n'
           '// address 0x%x, size %d bytes\n// name confidence: 0.7   rewrite confidence: 0.85\n%s'
           '// blam-cc: stack -> function_index, node_index (cdecl); returns AL\n\n' % (name, who, addr, addr, size, wr))
    src += COMMON % dict(extra_inc=extra_inc, externs=externs) + helpers + '\n' + body
    p = 'src/hs/%s.c' % name
    assert not os.path.exists(p), p
    open(p, 'w', encoding='utf-8').write(src)
    print('wrote', p)
