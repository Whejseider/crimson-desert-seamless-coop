// cdmp_peer.exe - stands in for the second player so everything can be
// tested on a single PC, and relays between two real players.
//
//   cdmp_peer [--port 37015] [--mode relay|ghost|bot] [--delay 100] [--offset 2]
//             [--loss 0] [--jitter 0] [--public] [--quiet] [--seconds N]
//
// relay: co-op. With two players connected, everything one sends (state,
//        game events, reliable data) is forwarded to the other. With a single
//        player it is echoed back after --delay ms, so the game shows a remote
//        copy of you driven over the network (PlayerState also shifted by
//        --offset metres on X for the overlay marker).
// ghost: echoes only your PlayerState, shifted by --offset (overlay test).
// bot:   ignores your movement and walks a 3 m circle around your last
//        known position (overlay test).
// replay: plays a recording (--replay FILE) to the connected game as the other
//        player, in its original timing from the game's first player state, and
//        ignores what the game sends (Co-op+: a lively teammate for showcase
//        videos instead of an echo of your own moves).
// --record FILE (relay): saves what the first player's game sends (state, game
//        events, reliable data) with its timing, for --mode replay.
// --story FILE (relay, one player): Co-op+ story sync test. The tool ends the game's reliable stream itself
//        (echoing what it delivers) and answers a story ask with FILE (cdmp_savetool extract), as a host would.
// --quest HEX[@SEC] (relay, one player): Co-op+ live quests test. Like --story, and SEC seconds (default 90) after the
//        game's first reliable message sends it one RelKind::Quest message (HEX = its bytes, mod/net/quest_live.h),
//        as the partner's finished step; the game's own Quest messages are printed, not echoed.
// --loss / --jitter simulate a bad connection (percent / ms) on everything
// this tool sends. --public listens on all interfaces (for a friend).

#include <windows.h>

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <fstream>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include "common/clock.h"
#include "common/game_save.h"
#include "common/protocol.h"
#include "common/reliable.h"
#include "common/udp.h"

using namespace cdmp;

namespace {

struct Options {
    uint16_t port = 37015;
    std::string mode = "relay";
    int delay_ms = 100;
    float offset_m = 2.0f;
    int loss_pct = 0;
    int jitter_ms = 0;
    bool public_bind = false;
    bool quiet = false;
    int seconds = 0;  // 0 = run until Ctrl+C
    std::string record_path;  // Co-op+: --record FILE
    std::string replay_path;  // Co-op+: --replay FILE (mode replay)
    std::string story_path;   // Co-op+: --story FILE (relay): answers story asks
    std::string quest_hex;    // Co-op+: --quest HEX[@SEC] (relay): one live quest message to the game
    std::string name;  // Co-op+: the name a lone client sees ("" = "cdmp_peer (<mode>)"), e.g. for showcase videos
};

void usage() {
    std::printf(
        "Usage: cdmp_peer [--port N] [--mode relay|ghost|bot] [--delay MS] [--offset M]\n"
        "                 [--loss PCT] [--jitter MS] [--public] [--quiet] [--seconds N] [--name NAME]\n");
}

std::optional<Options> parse(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto next = [&]() -> const char* { return i + 1 < argc ? argv[++i] : nullptr; };
        const char* v = nullptr;
        if (a == "--public") o.public_bind = true;
        else if (a == "--quiet") o.quiet = true;
        else if ((a == "--port") && (v = next())) o.port = static_cast<uint16_t>(std::atoi(v));
        else if ((a == "--mode") && (v = next())) o.mode = v;
        else if ((a == "--delay") && (v = next())) o.delay_ms = std::atoi(v);
        else if ((a == "--offset") && (v = next())) o.offset_m = static_cast<float>(std::atof(v));
        else if ((a == "--loss") && (v = next())) o.loss_pct = std::atoi(v);
        else if ((a == "--jitter") && (v = next())) o.jitter_ms = std::atoi(v);
        else if ((a == "--seconds") && (v = next())) o.seconds = std::atoi(v);
        else if ((a == "--name") && (v = next())) o.name = v;
        else if ((a == "--record") && (v = next())) o.record_path = v;
        else if ((a == "--replay") && (v = next())) o.replay_path = v;
        else if ((a == "--story") && (v = next())) o.story_path = v;
        else if ((a == "--quest") && (v = next())) o.quest_hex = v;
        else {
            usage();
            return std::nullopt;
        }
    }
    if (o.mode != "relay" && o.mode != "ghost" && o.mode != "bot" && !(o.mode == "replay" && !o.replay_path.empty())) {
        usage();
        return std::nullopt;
    }
    return o;
}

volatile bool g_running = true;

BOOL WINAPI on_ctrl(DWORD) {
    g_running = false;
    return TRUE;
}

constexpr size_t kMaxClients = 2;
constexpr uint32_t kClientTimeoutMs = 5000;

class Peer {
public:
    explicit Peer(const Options& opt) : opt_(opt), rng_(std::random_device{}()) {}

    bool open() {
        if (!opt_.replay_path.empty() && !load_replay()) return false;
        if (!opt_.story_path.empty()) {
            std::ifstream in(opt_.story_path, std::ios::binary);
            story_.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
            const auto c = save::story_count(story_);
            if (!c) {
                std::printf("Cannot use %s as story data\n", opt_.story_path.c_str());
                return false;
            }
            std::printf("Story: %zu bytes, %u missions, %u done\n", story_.size(), c->missions, c->done);
        }
        if (!opt_.quest_hex.empty()) {
            std::string h = opt_.quest_hex;
            if (const auto at = h.find('@'); at != std::string::npos) {
                quest_after_ms_ = static_cast<uint32_t>(std::atoi(h.c_str() + at + 1)) * 1000;
                h.resize(at);
            }
            for (size_t i = 0; i + 1 < h.size(); i += 2) {
                quest_msg_.push_back(static_cast<uint8_t>(std::strtoul(h.substr(i, 2).c_str(), nullptr, 16)));
            }
            std::printf("Quest: %zu bytes, sent %u s after the game's first reliable message\n", quest_msg_.size(),
                        quest_after_ms_ / 1000);
        }
        if (!opt_.record_path.empty()) {
            record_.open(opt_.record_path, std::ios::binary | std::ios::trunc);
            if (!record_) {
                std::printf("Cannot write %s\n", opt_.record_path.c_str());
                return false;
            }
        }
        if (!sock_.open(opt_.port, !opt_.public_bind)) {
            std::printf("Cannot bind UDP port %u: %s\n", opt_.port,
                        net::UdpSocket::last_error().c_str());
            return false;
        }
        std::printf("cdmp_peer listening on %s:%u, mode %s (delay %d ms, offset %.1f m, loss %d%%, "
                    "jitter %d ms)\n",
                    opt_.public_bind ? "0.0.0.0" : "127.0.0.1", opt_.port, opt_.mode.c_str(),
                    opt_.delay_ms, opt_.offset_m, opt_.loss_pct, opt_.jitter_ms);
        return true;
    }

    void run() {
        std::vector<uint8_t> buf(proto::kMaxPacketSize);
        const uint32_t started = now_ms();
        uint32_t last_bot = 0;
        uint32_t last_report = now_ms();

        while (g_running) {
            net::Endpoint from;
            if (auto n = sock_.recv_from(from, buf, 2)) handle({buf.data(), *n}, from);

            const uint32_t now = now_ms();
            flush_outbox(now);

            if (opt_.mode == "replay" && !clients_.empty()) replay_due(now);
            if (chan_) chan_->tick(now);
            if (chan_ && !quest_msg_.empty() && !quest_sent_ && quest_due_ms_ && now >= quest_due_ms_) {
                quest_sent_ = true;
                const bool ok = chan_->send(proto::RelKind::Quest, quest_msg_, now);
                std::printf("Quest: sent the partner's step%s\n", ok ? "" : " (refused)");
            }
            if (opt_.mode == "bot" && !clients_.empty() && now - last_bot >= 50) {
                last_bot = now;
                send_bot_state(clients_.front());
            }
            const size_t before = clients_.size();
            std::erase_if(clients_, [&](const Client& c) {
                if (now - c.last_recv_ms <= kClientTimeoutMs) return false;
                std::printf("Client '%s' %s timed out\n", c.name.c_str(), c.ep.str().c_str());
                return true;
            });
            if (clients_.size() != before) announce();
            if (!opt_.quiet && now - last_report >= 2000) {
                last_report = now;
                report();
            }
            if (opt_.seconds > 0 && now - started >= static_cast<uint32_t>(opt_.seconds) * 1000) {
                break;
            }
        }
        report();
    }

private:
    struct Client {
        net::Endpoint ep;
        std::string name;
        uint32_t last_recv_ms = 0;
        std::optional<proto::PlayerState> state;
        uint64_t received = 0, events = 0, reliable = 0;
    };
    struct Pending {
        uint32_t due_ms;
        net::Endpoint to;
        std::vector<uint8_t> bytes;
    };

    std::vector<uint8_t> story_;
    std::vector<uint8_t> quest_msg_;
    uint32_t quest_after_ms_ = 90000, quest_due_ms_ = 0;
    bool quest_sent_ = false;
    std::optional<ReliableChannel> chan_;
    net::Endpoint chan_ep_;

    Client* find(const net::Endpoint& ep) {
        for (auto& c : clients_) {
            if (c.ep == ep) return &c;
        }
        return nullptr;
    }

    // Where a client's data goes: the other player, or back to itself.
    const Client& partner_of(const Client& c) const {
        for (const auto& o : clients_) {
            if (!(o.ep == c.ep)) return o;
        }
        return c;
    }

    void handle(std::span<const uint8_t> bytes, const net::Endpoint& from) {
        auto packet = proto::decode(bytes);
        if (!packet) {
            ++rejected_;
            return;
        }
        ++received_;

        if (auto* hello = std::get_if<proto::Hello>(&packet->body)) {
            Client* c = find(from);
            if (!c) {
                if (clients_.size() >= (opt_.mode == "relay" ? kMaxClients : 1)) {
                    ++rejected_;  // full: the client keeps retrying
                    return;
                }
                clients_.push_back({from, hello->name});
                c = &clients_.back();
                std::printf("Hello from '%s' (build %u) at %s [%zu player(s)]\n",
                            hello->name.c_str(), hello->mod_build, from.str().c_str(),
                            clients_.size());
            }
            c->last_recv_ms = now_ms();
            announce();
            return;
        }
        Client* c = find(from);
        if (!c) {
            ++rejected_;
            return;
        }
        c->last_recv_ms = now_ms();
        ++c->received;

        if (record_ && c == &clients_.front() &&
            (std::holds_alternative<proto::PlayerState>(packet->body) ||
             std::holds_alternative<proto::GameEvent>(packet->body) ||
             std::holds_alternative<proto::RelData>(packet->body))) {
            const uint32_t now = now_ms();
            if (!record_start_) record_start_ = now ? now : 1;
            const uint32_t t = now - record_start_;
            const auto n = static_cast<uint32_t>(bytes.size());
            record_.write(reinterpret_cast<const char*>(&t), 4);
            record_.write(reinterpret_cast<const char*>(&n), 4);
            record_.write(reinterpret_cast<const char*>(bytes.data()), n);
            ++recorded_;
        }
        if (opt_.mode == "replay" && !replay_start_ && std::holds_alternative<proto::PlayerState>(packet->body)) {
            replay_start_ = now_ms() ? now_ms() : 1;  // the game is in the world: the recording starts the same way
            std::printf("Replay: started\n");
        }
        if (opt_.mode == "replay" && !std::holds_alternative<proto::Ping>(packet->body) &&
            !std::holds_alternative<proto::Bye>(packet->body)) {
            return;
        }
        if (auto* ping = std::get_if<proto::Ping>(&packet->body)) {
            send(from, proto::Pong{ping->origin_ms});
        } else if (std::holds_alternative<proto::Bye>(packet->body)) {
            std::printf("Client '%s' %s said goodbye\n", c->name.c_str(), from.str().c_str());
            std::erase_if(clients_, [&](const Client& x) { return x.ep == from; });
            announce();
        } else if (auto* state = std::get_if<proto::PlayerState>(&packet->body)) {
            c->state = *state;
            if (opt_.mode == "bot") return;
            const Client& to = opt_.mode == "relay" ? partner_of(*c) : *c;
            proto::PlayerState out = *state;
            if (to.ep == c->ep) out.pos.x += opt_.offset_m;  // echo: keep the marker visible
            send(to.ep, out, opt_.delay_ms);
        } else if (opt_.mode == "relay" && (!story_.empty() || !quest_msg_.empty()) && clients_.size() == 1 &&
                   (std::holds_alternative<proto::RelData>(packet->body) ||
                    std::holds_alternative<proto::RelAck>(packet->body))) {
            story_link(*c, packet->body);
        } else if (opt_.mode == "relay") {
            // Game events and reliable fragments/acks: forwarded unchanged.
            if (std::holds_alternative<proto::GameEvent>(packet->body)) ++c->events;
            if (std::holds_alternative<proto::RelData>(packet->body)) ++c->reliable;
            send(partner_of(*c).ep, std::move(packet->body), opt_.delay_ms);
        }
    }

    // --story: the game's reliable messages end here and go back on our own stream (as the echo would), except a
    // story ask, which gets the story in parts (the format of mod/net/story_sync.cpp).
    void story_link(const Client& c, const proto::Body& body) {
        if (!chan_ || !(chan_ep_ == c.ep)) {
            chan_ep_ = c.ep;
            chan_.emplace([this](proto::Body b) { send(chan_ep_, std::move(b), opt_.delay_ms); });
            quest_due_ms_ = now_ms() + quest_after_ms_;
        }
        if (auto* ack = std::get_if<proto::RelAck>(&body)) {
            chan_->on_ack(*ack);
            return;
        }
        for (auto& m : chan_->on_data(std::get<proto::RelData>(body))) {
            if (m.kind == proto::RelKind::Quest) {
                std::printf("Quest: the game sent");
                for (const uint8_t b : m.bytes) std::printf(" %02x", b);
                std::printf("\n");
            } else if (m.kind == proto::RelKind::Story && m.bytes.size() >= 5 && m.bytes[0] == 1) {
                const auto comp = save::lz4_compress(story_.data(), story_.size());
                const size_t part = 200 * 1024, count = (comp.size() + part - 1) / part;
                for (size_t i = 0; i < count; ++i) {
                    std::vector<uint8_t> b(m.bytes.begin(), m.bytes.begin() + 5);
                    b[0] = 2;
                    const auto u16 = [&](size_t v) { b.push_back(static_cast<uint8_t>(v)); b.push_back(static_cast<uint8_t>(v >> 8)); };
                    u16(i);
                    u16(count);
                    const auto n = static_cast<uint32_t>(story_.size());
                    for (int k = 0; k < 4; ++k) b.push_back(static_cast<uint8_t>(n >> (k * 8)));
                    b.insert(b.end(), comp.begin() + i * part, comp.begin() + std::min(comp.size(), (i + 1) * part));
                    if (!chan_->send(proto::RelKind::Story, b, now_ms())) std::printf("Story: part %zu refused\n", i);
                }
                std::printf("Story: asked; sent %zu KB in %zu parts\n", comp.size() / 1024, count);
            } else {
                chan_->send(m.kind, m.bytes, now_ms());
            }
        }
    }

    struct Recorded {
        uint32_t t;
        std::vector<uint8_t> bytes;
    };

    bool load_replay() {
        std::ifstream in(opt_.replay_path, std::ios::binary);
        if (!in) {
            std::printf("Cannot read %s\n", opt_.replay_path.c_str());
            return false;
        }
        uint32_t t = 0, n = 0;
        while (in.read(reinterpret_cast<char*>(&t), 4) && in.read(reinterpret_cast<char*>(&n), 4)) {
            Recorded r{t, std::vector<uint8_t>(n)};
            if (!in.read(reinterpret_cast<char*>(r.bytes.data()), n)) break;
            replay_.push_back(std::move(r));
        }
        std::printf("Replay: %zu packets, %.1f s\n", replay_.size(), replay_.empty() ? 0.0 : replay_.back().t / 1000.0);
        return !replay_.empty();
    }

    // From the moment the game connected: every recorded packet that is due, re-numbered by send().
    void replay_due(uint32_t now) {
        if (!replay_start_) return;
        while (replay_next_ < replay_.size() && now - replay_start_ >= replay_[replay_next_].t) {
            if (auto pkt = proto::decode(replay_[replay_next_].bytes)) send(clients_.front().ep, std::move(pkt->body));
            ++replay_next_;
        }
    }

    // Welcome every client with the name of whoever is on the other side
    // (the other player in a relay, this tool otherwise). Sent again when
    // that changes; ClientSession picks up the new name.
    void announce() {
        for (const auto& c : clients_) {
            const Client& other = partner_of(c);
            const bool alone = opt_.mode != "relay" || other.ep == c.ep;
            send(c.ep, proto::Welcome{alone ? (opt_.name.empty() ? "cdmp_peer (" + opt_.mode + ")" : opt_.name) : other.name});
        }
    }

    void send_bot_state(const Client& c) {
        const proto::Vec3 center = c.state ? c.state->pos : proto::Vec3{};
        const float angle = static_cast<float>(now_ms() % 6000) / 6000.0f * 6.2831853f;
        proto::PlayerState s;
        s.pos = {center.x + 3.0f * std::cos(angle), center.y, center.z + 3.0f * std::sin(angle)};
        s.rot = {0.0f, std::sin(angle * 0.5f), 0.0f, std::cos(angle * 0.5f)};
        s.flags = proto::kStateSynthetic;
        send(c.ep, s);
    }

    void send(const net::Endpoint& to, proto::Body body, int extra_delay_ms = 0) {
        proto::Packet p;
        p.seq = next_seq_++;
        p.send_ms = now_ms();
        p.body = std::move(body);
        if (opt_.loss_pct > 0 && static_cast<int>(rng_() % 100) < opt_.loss_pct) {
            ++dropped_;
            return;
        }
        int delay = extra_delay_ms;
        if (opt_.jitter_ms > 0) delay += static_cast<int>(rng_() % (opt_.jitter_ms + 1));
        outbox_.push_back({now_ms() + static_cast<uint32_t>(delay), to, proto::encode(p)});
    }

    void flush_outbox(uint32_t now) {
        // Jitter can reorder packets on purpose, so scan the whole queue.
        for (auto it = outbox_.begin(); it != outbox_.end();) {
            if (static_cast<int32_t>(now - it->due_ms) >= 0) {
                if (find(it->to)) {
                    sock_.send_to(it->to, it->bytes);
                    ++sent_;
                }
                it = outbox_.erase(it);
            } else {
                ++it;
            }
        }
    }

    void report() {
        std::printf("recv %llu rejected %llu | sent %llu dropped %llu | %zu player(s)\n",
                    static_cast<unsigned long long>(received_),
                    static_cast<unsigned long long>(rejected_),
                    static_cast<unsigned long long>(sent_),
                    static_cast<unsigned long long>(dropped_), clients_.size());
        for (const auto& c : clients_) {
            std::string pos = "-";
            if (c.state) {
                char b[96];
                std::snprintf(b, sizeof b, "(%.2f, %.2f, %.2f) hp %.1f/%.1f%s", c.state->pos.x,
                              c.state->pos.y, c.state->pos.z, c.state->health,
                              c.state->health_max,
                              (c.state->flags & proto::kStateSynthetic) ? " synthetic" : "");
                pos = b;
            }
            std::printf("  '%s' %s: recv %llu, game events %llu, reliable fragments %llu, pos %s\n",
                        c.name.c_str(), c.ep.str().c_str(),
                        static_cast<unsigned long long>(c.received),
                        static_cast<unsigned long long>(c.events),
                        static_cast<unsigned long long>(c.reliable), pos.c_str());
        }
        std::fflush(stdout);
    }

    Options opt_;
    net::UdpSocket sock_;
    std::mt19937 rng_;
    std::vector<Client> clients_;
    std::deque<Pending> outbox_;
    uint32_t next_seq_ = 1;
    uint64_t received_ = 0, rejected_ = 0, sent_ = 0, dropped_ = 0;
    std::ofstream record_;
    uint32_t record_start_ = 0;
    uint64_t recorded_ = 0;
    std::vector<Recorded> replay_;
    size_t replay_next_ = 0;
    uint32_t replay_start_ = 0;
};

}  // namespace

int main(int argc, char** argv) {
    std::setvbuf(stdout, nullptr, _IONBF, 0);  // Co-op+: output reaches a redirected file even if the tool is stopped
    auto opt = parse(argc, argv);
    if (!opt) return 2;
    if (!net::startup()) {
        std::printf("Winsock startup failed\n");
        return 1;
    }
    SetConsoleCtrlHandler(on_ctrl, TRUE);

    int rc = 1;
    {
        Peer peer(*opt);
        if (peer.open()) {
            peer.run();
            rc = 0;
        }
    }
    net::cleanup();
    return rc;
}
