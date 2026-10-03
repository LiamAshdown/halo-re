#include "halo/networking/net1_banlist.hpp"
#include <stdio.h>
#include <time.h>
#include "halo/networking/api.hpp"

extern "C" {
extern growable_array ban_list;
extern char network_summary_log_mode_string[];
extern char network_banlist_full_path[];
}

namespace halo::networking {

/**
 * fopen: <stdio.h>, resolved to the game CRT at 0x624186 // 0x624186, fopen-shaped CRT wrapper
 * Writes the whole in-memory ban list back out to banned<suffix>.txt as CSV: name, cd key hash,
 * ban count, then either "--" for an indefinite ban or a "YYYY-MM-DD HH:MM:SS" expiry date.
 *
 * @address 0x4e3380
 */
void Banlist::save()
{
    FILE *file;
    ban_list_entry *entries;
    int32_t i;
    int32_t row;
    struct tm zero_tm;
    struct tm *tm_now;
    char time_buf[31];
    char date_buf[31];
    time_t expiry;

    file = (FILE *)fopen(halo::networking::network_log_path_resolve(network_banlist_full_path),
                                 network_summary_log_mode_string);
    if (file != 0) {
        fprintf(file, "# Name, CD key hash, ban count, ban end date\r\n");
        entries = (ban_list_entry *)ban_list.data;
        row = 0;
        if (0 < ban_list.count) {
            i = 0;
            do {
                ban_list_entry *entry = &entries[i];
                fprintf(file, "%s,%s,%d,", entry->name, entry->cd_key_hash, entry->ban_count);
                if (entry->indefinite == 0) {
                    expiry = entry->expiry_time;
                    zero_tm.tm_min = 0;
                    zero_tm.tm_hour = 0;
                    zero_tm.tm_mday = 0;
                    zero_tm.tm_mon = 0;
                    zero_tm.tm_year = 0;
                    zero_tm.tm_wday = 0;
                    zero_tm.tm_yday = 0;
                    zero_tm.tm_sec = 0;
                    zero_tm.tm_isdst = 0;
                    tm_now = localtime(&expiry);
                    if (tm_now == 0) {
                        tm_now = &zero_tm;
                    }
                    snprintf(time_buf, 0x1f, "%02d:%02d:%02d", tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);
                    time_buf[0x1f] = 0;
                    snprintf(date_buf, 0x1f, "%04d-%02d-%02d", tm_now->tm_year + 0x76c, tm_now->tm_mon + 1, tm_now->tm_mday);
                    date_buf[0x1f] = 0;
                    fprintf(file, "%s %s\r\n", date_buf, time_buf);
                } else {
                    fprintf(file, "--\r\n");
                }
                row = row + 1;
                i = i + 1;
            } while (row < ban_list.count);
        }
        fclose(file);
    }
}

}
