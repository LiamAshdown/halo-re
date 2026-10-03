#pragma once

#include "halo/shell/platform.hpp"

namespace halo::shell {

/**
 * Splits the raw command line into an argv style array and answers flag queries against the parsed
 * result stored in shell_argv / shell_argc.
 */
class CommandLine {
public:
    static char **parse_to_argv(char *command_line, int32_t *out_count);
    static uint8_t has_flag(const char *flag_name, const char **out_value);
};

/**
 * Text clipboard access for the main window.
 */
class Clipboard {
public:
    static uint32_t get_text(char *buffer, uint32_t capacity);
};

/**
 * Lowercase hexadecimal parsing used for GUID and hardware id strings.
 */
class HexParser {
public:
    static int32_t to_uint(char *string);
    static void to_bytes(uint8_t *dest, const char *source);
};

/**
 * SHA-1 digests through the system's cryptographic provider (shared provider handle crypt_provider).
 */
class Sha1 {
public:
    static uint8_t hash(const uint8_t *data, uint32_t length, uint8_t *digest_out);
    static uint8_t hash_first_qword(const uint8_t *data, uint32_t length, uint32_t *output);
};

/**
 * Product id string built from the DigitalProductID setting for support and crash reports.
 */
class ProductId {
public:
    static int32_t extract_digits(const char *product_id);
    static char *build_string();
};

/**
 * Processor identification from the CPUID instruction, with the AMD sample code query interface:
 * vendor, model, feature bits and cache/TLB descriptors.
 */
class Cpu {
public:
    static int32_t query_identification();
    static int32_t get_type(int32_t mode);
};

/**
 * Operating system family detection, cached in os_platform_value.
 */
class OperatingSystem {
public:
    static void identify();
};

/**
 * Releases the handles and buffers the write access test allocates; every member is released
 * when the owner goes out of scope.
 */
class SecurityResources {
public:
    SecurityResources() : descriptor(0), acl(0), sid(0), thread_token(0), impersonation_token(0) {}
    ~SecurityResources() { release(descriptor, acl, sid, thread_token, impersonation_token, 0); }
    SecurityResources(const SecurityResources &) = delete;
    SecurityResources &operator=(const SecurityResources &) = delete;

    static void release(void *descriptor, void *acl, void *sid, void *thread_token, void *impersonation_token, void *sentinel);

    void *descriptor;
    void *acl;
    void *sid;
    void *thread_token;
    void *impersonation_token;
};

/**
 * Tests (once, cached) whether the running account may write under a synthetic ACL; always granted
 * on non-NT platforms.
 */
class WriteAccessCheck {
public:
    static int32_t run();
};

/**
 * Named mutex that keeps a second copy of the game from starting. Mode single takes one slot, mode
 * multiple takes the first free of eight.
 */
class SingleInstance {
public:
    static void check(int32_t mode);
    static void release();
};

/**
 * The ExitFlag setting: written "clean" on a normal exit, advanced "bad 1" / "bad 2" at start up so
 * repeated crashes can be detected.
 */
class ExitFlag {
public:
    static void set_clean();
    static int32_t previous_run_crashed();
};

}
