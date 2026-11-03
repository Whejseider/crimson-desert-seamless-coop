#pragma once
// Co-op+ (roadmap T43): a lasting scoreboard and titles.
//
// Every challenge round won or lost (duel, rock-paper-scissors, hunting contest, race, arm wrestling) is counted per
// friend in cdmp_scores.ini next to cdmp.ini; the F8 panel shows the record against the friend you play with. Total
// wins give a title ("the Sparring Partner" from 3, "the Duelist" from 10 duel wins, ...), which the friend sees after
// your name on their partner frame (Activity type 25: the title, UTF-8).

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace cdmp::net {

void scoreboard_init(const std::wstring& scores_ini_path);

// A round ended. `game`: "duel", "rps", "hunt", "race", "wrestle".
void scoreboard_record(const std::string& friend_name, const std::string& game, bool won);

// "Record vs Ana: duels 3-1, races 1-0" lines for the panel ("" friend = none).
std::vector<std::string> scoreboard_lines(const std::string& friend_name);

// Our title from our wins ("" = none yet), and the friend's as they sent it.
std::string scoreboard_title();
void scoreboard_on_event(const uint8_t* data, size_t n);
std::string scoreboard_partner_title();

// Network loop: sends our title when connected (again when it changes or after a reconnect).
void scoreboard_tick(bool connected);

}  // namespace cdmp::net
