/**
 * @file include/halo/networking/net2_server_commands.hpp
 * Server console commands.
 *
 * The engine type headers (types/*.h) are not include-guarded: include this header after them.
 */
#pragma once

namespace halo::networking {

/**
 * Server console commands.
 *
 * Static behaviour facade; the original C entry points forward to these members.
 */
class ServerCommands {
public:
    /**
     * 0x496b50, EAX color (NULL = default) Console command: bans (and disconnects) the client named or indexed by the first argument, for the duration given by an optional second argument (parsed as d/h/m/s, default minutes) or the escalating penalty table if omitted. Refuses on a client or against the local client.
     *
     * @address 0x4e3990
     */
    static void ban(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50 Console command: with no arguments, prints the current sv_ban_penalty escalating-duration table (up to 4 tiers, each formatted as "Nd Nh Nm Ns", "---" for zero, or "Infinite" past the table's end); with 1-4 arguments, parses each as a d/h/m/s duration, stopping (and leaving any remaining tiers at their previous values) the moment one argument parses to exactly 0, which instead marks that one tier indefinite (-1); any other parse failure restores the whole table to what it was before the command ran.
     *
     * @address 0x4e3a80
     */
    static void ban_penalty(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: with no arguments, reports the current ban-list filename suffix; with one, validates it is non-empty, at most 0xf5 characters, and alphanumeric, then rebuilds network_banlist_full_path as "<install dir>\bannedSUFFIX.txt" and reloads the list.
     *
     * @address 0x4e3db0
     */
    static void banlist_file(uint32_t argument_count, int32_t *arguments);

    /**
     * foreign (< this batch), validates one player row Resolves a console-command argument to a player row of the local session: either a 1-based client slot number (matched against each row's slot_index) or a player name (matched case-sensitively, UTF-16). Returns 0 if nothing matches.
     *
     * @address 0x4e3f70
     */
    static network_player_entry * find_client_by_name_or_index(char *name_or_index);

    /**
     * 0x496b50 Console command: gets or sets the friendly-fire mode (0/default, 1/off, 2/shields, 3/on), accepting either the numeric or word form of each value.
     *
     * @address 0x4e4810
     */
    static void friendly_fire(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: disconnects (kicks) the client named or indexed by the console argument. Refuses if this machine is not hosting, if no matching client is found, or if the resolved machine is the local (listen-server) client.
     *
     * @address 0x4e3910
     */
    static void kick(char *name_or_index);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: validates the requested map/variant, then either restarts the current dedicated-server game with the new variant (when hosting) or starts a brand-new dedicated server with it (when not yet in a game).
     *
     * @address 0x4e2b20
     */
    static void map(uint32_t argument_count, uint16_t **arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: restarts the current map, refusing when not hosting or when the round is already ending/over.
     *
     * @address 0x4e2aa0
     */
    static void map_reset(void);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: gets or sets the maximum number of players (1-16) for a hosted server.
     *
     * @address 0x4e4ac0
     */
    static void maxplayers(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: with no arguments, reports the current server name; with one, validates it is 1-63 characters of printable ASCII and, if so, converts and stores it as the new server name, resetting the "is default" flag and syncing the live server object if hosting.
     *
     * @address 0x4e2ed0
     */
    static void name(uint32_t argument_count, char **arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: with no arguments, reports the current server password; with one, validates it is at most 8 characters, accepts it as-is if empty (clearing the password) or validates it as printable ASCII otherwise, then stores it and syncs the live server object if hosting.
     *
     * @address 0x4e2ff0
     */
    static void password(uint32_t argument_count, char **arguments);

    /**
     * Console command: prints a formatted scoreboard header, then one row per valid player entry in the session's player table, including the ping, engine score text and team-kill statistics of the matching live player.
     *
     * @address 0x4e2c70
     */
    static void players(void);

    /**
     * 0x4d05d0, memory module Walks every live player looking for one whose team_index_desired matches, returning its datum_index, or k_datum_index_none if no player matches.
     *
     * @address 0x4e2c10
     */
    static uint32_t players_find_by_team_index_desired(int8_t team_index_desired);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: gets or sets the server's rcon password (empty string disables rcon), enforcing an 8-character maximum.
     *
     * @address 0x4e4b50
     */
    static void rcon_password(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: gets or sets the "single flag force reset" boolean via the shared boolean-command helper; if the stored value actually changes while a game is in progress (current_game_engine != 0), warns that the change only takes effect next game.
     *
     * @address 0x4e3100
     */
    static void single_flag_force_reset(uint32_t argument_count, char **arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: prints the current map and player count, and whether the game is ending, or reports that this is a server-only command.
     *
     * @address 0x4e2e50
     */
    static void status(void);

    /**
     * 0x496b50 Console command: gets or sets the game time limit in minutes (-1 default, 0 infinite, else 1..599), accepting "default"/"infinite" as synonyms for -1/0.
     *
     * @address 0x4e49a0
     */
    static void timelimit(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: gets or sets the team-kill punishment cooldown, stored internally in ticks (30/sec) but reported/parsed in seconds (default unit 's' when a bare number is given).
     *
     * @address 0x4e3d40
     */
    static void tk_cooldown(uint32_t argument_count, int32_t *arguments);

    /**
     * 0x496b50, EAX color (NULL = default) Console command: gets or sets the team-kill grace period, stored internally in ticks (30/sec) but reported/parsed in seconds (default unit 's' when a bare number is given).
     *
     * @address 0x4e3cd0
     */
    static void tk_grace(uint32_t argument_count, int32_t *arguments);

};

}  // namespace halo::networking
