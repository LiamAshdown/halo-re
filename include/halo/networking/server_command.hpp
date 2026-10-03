/**
 * @file include/halo/networking/server_command.hpp
 * Command objects for the server console commands (sv_ban, sv_kick, sv_map, ...).
 */
#pragma once

#include <stdint.h>

namespace halo::networking {

enum class ServerCommandId : uint8_t {
    ban,
    ban_penalty,
    banlist_file,
    friendly_fire,
    kick,
    map,
    map_reset,
    maxplayers,
    name,
    password,
    players,
    rcon_password,
    single_flag_force_reset,
    status,
    timelimit,
    tk_cooldown,
    tk_grace,
    count
};

/**
 * Returns the console name of a command (for example "sv_kick").
 */
const char *server_command_name(ServerCommandId id);

/**
 * Command interface for one server console command. The console hands every command an argument count and an
 * argument vector whose element type differs per command, so the vector travels as an opaque pointer and each
 * concrete command restores its own type.
 */
class ServerCommand {
public:
    virtual const char *name() const = 0;
    virtual void execute(uint32_t argument_count, void *arguments) const = 0;

protected:
    ~ServerCommand() = default;
};

/**
 * Command whose handler takes the argument count and a typed argument vector (the sv_ban family).
 */
template <typename Argument, void (*Handler)(uint32_t, Argument *)>
class ArgumentCommand final : public ServerCommand {
public:
    explicit constexpr ArgumentCommand(ServerCommandId command_id) : id(command_id) {}

    const char *name() const override { return server_command_name(id); }
    void execute(uint32_t argument_count, void *arguments) const override
    {
        Handler(argument_count, static_cast<Argument *>(arguments));
    }

private:
    ServerCommandId id;
};

/**
 * Command whose handler takes a single client name or index string (sv_kick); `arguments` is that string.
 */
template <void (*Handler)(char *)>
class NameCommand final : public ServerCommand {
public:
    explicit constexpr NameCommand(ServerCommandId command_id) : id(command_id) {}

    const char *name() const override { return server_command_name(id); }
    void execute(uint32_t, void *arguments) const override { Handler(static_cast<char *>(arguments)); }

private:
    ServerCommandId id;
};

/**
 * Command whose handler takes no arguments (sv_status, sv_players, sv_map_reset).
 */
template <void (*Handler)(void)>
class PlainCommand final : public ServerCommand {
public:
    explicit constexpr PlainCommand(ServerCommandId command_id) : id(command_id) {}

    const char *name() const override { return server_command_name(id); }
    void execute(uint32_t, void *) const override { Handler(); }

private:
    ServerCommandId id;
};

/**
 * Registry of all server commands, indexed by ServerCommandId and searchable by console name.
 */
class ServerCommandRegistry {
public:
    static const ServerCommand &get(ServerCommandId id);
    static const ServerCommand *find(const char *command_name);
};

}  // namespace halo::networking
