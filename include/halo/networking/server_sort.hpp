/**
 * @file include/halo/networking/server_sort.hpp
 * Sort strategies for the server browser result list.
 *
 * Include after types/networking.h (the engine type headers are not include-guarded).
 */
#pragma once

namespace halo::networking {

/**
 * Strategy for one server browser sort column: yields the qsort comparator that orders the result list for that
 * column.
 */
class ServerSortStrategy {
public:
    virtual server_browser_sort_comparator comparator() const = 0;

protected:
    ~ServerSortStrategy() = default;
};

/**
 * Orders by ping, then host name (column 3).
 */
class PingThenHostnameSort final : public ServerSortStrategy {
public:
    server_browser_sort_comparator comparator() const override;
};

/**
 * Orders by game type (column 2).
 */
class GametypeSort final : public ServerSortStrategy {
public:
    server_browser_sort_comparator comparator() const override;
};

/**
 * Orders by player count (column 4).
 */
class PlayersSort final : public ServerSortStrategy {
public:
    server_browser_sort_comparator comparator() const override;
};

/**
 * Orders by host name (the default column).
 */
class HostnameSort final : public ServerSortStrategy {
public:
    server_browser_sort_comparator comparator() const override;
};

/**
 * Column 1 of the original selection returns the scroll-clamp entry point as its comparator; this strategy keeps
 * that behaviour.
 */
class ScrollClampSort final : public ServerSortStrategy {
public:
    server_browser_sort_comparator comparator() const override;
};

/**
 * Maps a browser sort column number to its strategy object.
 */
class ServerSortRegistry {
public:
    static const ServerSortStrategy &for_column(int32_t column);
};

}  // namespace halo::networking
