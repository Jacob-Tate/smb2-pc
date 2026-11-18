// Integration test for the relay server: simulated players over real UDP.
//   smb2-server-test [host] [port]
// Exits 0 if every check passes.

#include "netplay/protocol.h"

#include <enet/enet.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kDisc = 0xCAFEF00D;
constexpr int kDelay = 2;
int gFailures = 0;
int gChecks = 0;

#define CHECK(cond, ...)                                                                                              \
    do {                                                                                                              \
        gChecks++;                                                                                                    \
        if (!(cond)) {                                                                                                \
            std::printf("  FAIL line %d: ", __LINE__);                                                                \
            std::printf(__VA_ARGS__);                                                                                 \
            std::printf("\n");                                                                                        \
            gFailures++;                                                                                              \
        }                                                                                                             \
    } while (0)

typedef std::vector<uint8_t> Msg;

struct TestClient {
    ENetHost *host = nullptr;
    ENetPeer *peer = nullptr;
    std::deque<Msg> inbox, frames;
    uint8_t id = 0;
    uint8_t seq = 0; // gameSeq of the current game
    std::string name;

    bool connect(const char *hostname, uint16_t port) {
        host = enet_host_create(nullptr, 1, SMB_NET_CHANNEL_COUNT, 0, 0);
        ENetAddress address{};
        enet_address_set_host(&address, hostname);
        address.port = port;
        peer = enet_host_connect(host, &address, SMB_NET_CHANNEL_COUNT, 0);
        ENetEvent event;
        return enet_host_service(host, &event, 3000) > 0 && event.type == ENET_EVENT_TYPE_CONNECT;
    }
    void send(uint8_t channel, const SmbNetWriter &w) {
        enet_peer_send(peer, channel, enet_packet_create(w.data, w.pos, ENET_PACKET_FLAG_RELIABLE));
        enet_host_flush(host);
    }
    void pump(int ms) {
        ENetEvent event;
        while (enet_host_service(host, &event, ms) > 0) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) {
                Msg m(event.packet->data, event.packet->data + event.packet->dataLength);
                // Frames (channel 1) and control messages (channel 0) are not ordered against each other.
                (event.channelID == SMB_NET_CHANNEL_INPUT ? frames : inbox).push_back(std::move(m));
                enet_packet_destroy(event.packet);
            }
            ms = 0;
        }
    }
    // Waits for the next message of the given type, discarding others. Empty on timeout.
    Msg waitFor(uint8_t type, int timeoutMs = 2000) {
        std::deque<Msg> &queue = type == SMB_MSG_FRAME ? frames : inbox;
        for (int waited = 0; waited <= timeoutMs; waited += 10) {
            while (!queue.empty()) {
                Msg msg = std::move(queue.front());
                queue.pop_front();
                if (!msg.empty() && msg[0] == type) return msg;
            }
            pump(10);
        }
        return {};
    }
    // True if a message of this type shows up within ms (it is consumed).
    bool sees(uint8_t type, int ms) { return !waitFor(type, ms).empty(); }
    // Lets queued reliable messages be delivered and acknowledged first (a plain disconnect can drop them).
    void disconnectLater() {
        if (!peer) return;
        enet_peer_disconnect_later(peer, 0);
        ENetEvent event;
        for (int i = 0; i < 40 && enet_host_service(host, &event, 50) > 0;) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
            i++;
        }
        peer = nullptr;
    }
    void disconnect() {
        if (!peer) return;
        enet_peer_disconnect(peer, 0);
        ENetEvent event;
        for (int i = 0; i < 20 && enet_host_service(host, &event, 50) > 0; i++) {
            if (event.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(event.packet);
            if (event.type == ENET_EVENT_TYPE_DISCONNECT) break;
        }
        peer = nullptr;
    }
};

SmbNetSessionConfig makeConfig(uint32_t disc = kDisc, uint8_t locale = 1, const char *build = "test-build",
                               uint8_t delay = kDelay, uint32_t flags = 0) {
    SmbNetSessionConfig c;
    std::memset(&c, 0, sizeof(c));
    std::strncpy(c.buildId, build, sizeof(c.buildId) - 1);
    c.discHash = disc;
    c.locale = locale;
    c.inputDelay = delay;
    c.flags = flags;
    return c;
}

void sendHello(TestClient &c, const char *name, uint16_t version, const SmbNetSessionConfig &cfg) {
    uint8_t buf[128];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_HELLO);
    smbw_u16(&w, version);
    smbw_str(&w, name, SMB_NET_MAX_NAME);
    smbw_str(&w, cfg.buildId, SMB_NET_MAX_BUILD_ID);
    smbw_u32(&w, cfg.discHash);
    smbw_u8(&w, cfg.locale);
    c.send(SMB_NET_CHANNEL_CONTROL, w);
    c.name = name;
}

// Connects and says HELLO; returns false if the connection or WELCOME failed.
bool login(TestClient &c, const char *host, uint16_t port, const char *name,
           const SmbNetSessionConfig &cfg = makeConfig()) {
    if (!c.connect(host, port)) return false;
    sendHello(c, name, SMB_NET_PROTOCOL_VERSION, cfg);
    Msg m = c.waitFor(SMB_MSG_WELCOME);
    if (m.size() < 2) return false; // clientId, then (E8) a capability byte
    c.id = m[1];
    return true;
}

void simple(TestClient &c, uint8_t type, const void *payload = nullptr, size_t n = 0) {
    uint8_t buf[64];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, type);
    if (n) smbw_bytes(&w, payload, n);
    c.send(SMB_NET_CHANNEL_CONTROL, w);
}

void sendStart(TestClient &c, const SmbNetSessionConfig &cfg) {
    uint8_t buf[96];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_START_GAME);
    smbw_config(&w, &cfg);
    c.send(SMB_NET_CHANNEL_CONTROL, w);
}

void sendChoose(TestClient &c, uint8_t port) { simple(c, SMB_MSG_CHOOSE, &port, 1); }

void sendChecksum(TestClient &c, uint32_t poll, uint32_t hash) {
    uint8_t buf[16];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_CHECKSUM);
    smbw_u32(&w, poll);
    smbw_u32(&w, hash);
    c.send(SMB_NET_CHANNEL_CONTROL, w);
}

struct Err {
    bool ok = false;
    uint8_t code = 0;
    std::string text;
};
Err parseError(const Msg &m) {
    Err e;
    if (m.empty() || m[0] != SMB_MSG_ERROR) return e;
    SmbNetReader r;
    smbr_init(&r, m.data() + 1, m.size() - 1);
    e.code = smbr_u8(&r);
    char text[256];
    smbr_str(&r, text, sizeof(text));
    e.text = text;
    e.ok = !r.error;
    return e;
}
// True if the next ERROR (within the timeout) has this code.
bool expectError(TestClient &c, SmbNetError code, int ms = 1000) {
    Err e = parseError(c.waitFor(SMB_MSG_ERROR, ms));
    if (e.ok && e.code != code) std::printf("  (got error %u: %s)\n", e.code, e.text.c_str());
    return e.ok && e.code == code;
}

struct RoomState {
    bool ok = false;
    std::string code;
    uint8_t host = 0;
    uint8_t count = 0;
    uint8_t ids[SMB_NET_MAX_PLAYERS] = {};
    uint8_t owner[SMB_NET_MAX_PLAYERS] = {};
    uint16_t ping[SMB_NET_MAX_PLAYERS] = {};
    uint8_t phase = 0xFF;
};

RoomState parseRoomState(const Msg &msg) {
    RoomState st;
    if (msg.empty() || msg[0] != SMB_MSG_ROOM_STATE) return st;
    SmbNetReader r;
    smbr_init(&r, msg.data() + 1, msg.size() - 1);
    char code[SMB_NET_ROOM_CODE_LEN + 1] = {};
    smbr_bytes(&r, code, SMB_NET_ROOM_CODE_LEN);
    st.code = code;
    st.host = smbr_u8(&r);
    st.count = smbr_u8(&r);
    for (int i = 0; i < st.count && i < SMB_NET_MAX_PLAYERS; i++) {
        char name[SMB_NET_MAX_NAME + 1];
        st.ids[i] = smbr_u8(&r);
        smbr_str(&r, name, sizeof(name));
        st.ping[i] = smbr_u16(&r);
    }
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) st.owner[port] = smbr_u8(&r);
    st.phase = smbr_u8(&r);
    st.ok = !r.error && r.pos == r.size;
    return st;
}

// Latest ROOM_STATE within ms: waits for the first, then takes any that follow immediately.
RoomState latestRoomState(TestClient &c, int ms = 1500) {
    RoomState st = parseRoomState(c.waitFor(SMB_MSG_ROOM_STATE, ms));
    for (;;) {
        c.pump(30);
        bool more = false;
        for (auto it = c.inbox.begin(); it != c.inbox.end();) {
            if (!it->empty() && (*it)[0] == SMB_MSG_ROOM_STATE) {
                st = parseRoomState(*it);
                it = c.inbox.erase(it);
                more = true;
            } else {
                ++it;
            }
        }
        if (!more) return st;
    }
}

bool ownersAre(const RoomState &st, uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    const uint8_t want[4] = {a, b, c, d};
    return st.ok && std::memcmp(st.owner, want, 4) == 0;
}

struct GameStart {
    bool ok = false;
    SmbNetSessionConfig cfg{};
    uint8_t owners[SMB_NET_MAX_PLAYERS] = {};
};
GameStart waitGameStart(TestClient &c) {
    GameStart g;
    Msg m = c.waitFor(SMB_MSG_GAME_START);
    if (m.empty()) return g;
    SmbNetReader r;
    smbr_init(&r, m.data() + 1, m.size() - 1);
    smbr_config(&r, &g.cfg);
    smbr_bytes(&r, g.owners, SMB_NET_MAX_PLAYERS);
    c.seq = smbr_u8(&r);
    g.ok = !r.error && r.pos == r.size;
    return g;
}

// Deterministic pad content per (port, poll) so bundles can be verified.
SmbNetPad padFor(int port, uint32_t poll) {
    SmbNetPad p;
    p.buttons = static_cast<uint16_t>(poll * 7 + port * 1000);
    p.stickX = static_cast<int8_t>(poll + port * 30);
    p.stickY = static_cast<int8_t>(-static_cast<int>(poll) - port);
    p.substickX = static_cast<int8_t>(port * 40 - 60);
    p.substickY = static_cast<int8_t>(poll * 3);
    p.triggerL = static_cast<uint8_t>(poll * 5);
    p.triggerR = static_cast<uint8_t>(255 - poll);
    p.err = 0;
    return p;
}

void sendInput(TestClient &c, uint32_t poll, const std::vector<int> &ports) {
    uint8_t buf[64];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_INPUT);
    smbw_u32(&w, poll);
    smbw_u8(&w, c.seq);
    for (int port : ports) {
        const SmbNetPad p = padFor(port, poll);
        smbw_pad(&w, &p);
    }
    c.send(SMB_NET_CHANNEL_INPUT, w);
}

struct Frame {
    bool ok = false;
    uint32_t poll = 0;
    uint8_t mask = 0;
    uint8_t flags = 0;
    SmbNetPad pads[SMB_NET_MAX_PLAYERS];
};
Frame waitFrame(TestClient &c, int ms = 2000) {
    Frame f;
    for (;;) {
        Msg m = c.waitFor(SMB_MSG_FRAME, ms);
        if (m.empty()) return f;
        SmbNetReader r;
        smbr_init(&r, m.data() + 1, m.size() - 1);
        f.poll = smbr_u32(&r);
        const uint8_t seq = smbr_u8(&r);
        if (seq != c.seq) continue; // a frame of an older game
        f.mask = smbr_u8(&r);
        f.flags = smbr_u8(&r);
        for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
            smb_pad_neutral(&f.pads[port]);
            if (f.mask & (1 << port)) smbr_pad(&r, &f.pads[port]);
        }
        f.ok = !r.error && r.pos == r.size;
        return f;
    }
}

// True if the frame's pads equal the pattern for every port in expectMask and neutral for the rest.
bool frameMatches(const Frame &f, uint8_t expectMask) {
    if (!f.ok || f.mask != expectMask) return false;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        SmbNetPad want;
        if (expectMask & (1 << port)) want = padFor(port, f.poll);
        else smb_pad_neutral(&want);
        if (std::memcmp(&want, &f.pads[port], sizeof(want)) != 0) return false;
    }
    return true;
}

struct Player {
    TestClient *c;
    std::vector<int> ports;
};

// Lockstep exchange: each player sends the first kDelay polls up front, then per poll P sends P+kDelay and
// waits for the FRAME for P. Verifies order, mask and pad contents on every player. Returns true if all OK.
bool exchange(std::vector<Player> &players, uint32_t from, uint32_t to, uint8_t expectMask) {
    bool ok = true;
    if (from == 0) {
        for (auto &p : players)
            for (int d = 0; d < kDelay; d++) sendInput(*p.c, d, p.ports);
    }
    for (uint32_t poll = from; poll < to && ok; poll++) {
        for (auto &p : players) sendInput(*p.c, poll + kDelay, p.ports);
        for (auto &p : players) {
            const Frame f = waitFrame(*p.c);
            if (!f.ok || f.poll != poll || !frameMatches(f, expectMask)) {
                std::printf("  bad frame for %s: ok=%d poll=%u (want %u) mask=0x%x (want 0x%x)\n", p.c->name.c_str(),
                            f.ok, f.poll, poll, f.mask, expectMask);
                ok = false;
            }
        }
    }
    return ok;
}

// ---------------------------------------------------------------------------------------------------------

void testVersionMismatch(const char *host, uint16_t port) {
    std::printf("[version mismatch]\n");
    TestClient x;
    CHECK(x.connect(host, port), "connect");
    sendHello(x, "old", SMB_NET_PROTOCOL_VERSION + 1, makeConfig());
    CHECK(expectError(x, SMB_ERR_VERSION), "protocol mismatch gets ERR_VERSION");
    // Not welcomed: nothing else works.
    simple(x, SMB_MSG_CREATE_ROOM);
    CHECK(expectError(x, SMB_ERR_BAD_MESSAGE), "no HELLO accepted, no rooms");
    x.disconnect();
}

void testTwoPlayers(const char *host, uint16_t port) {
    std::printf("[two players: lobby, start, frames, checksum, desync, restart]\n");
    TestClient a, b;
    CHECK(login(a, host, port, "Alice"), "alice login");
    CHECK(login(b, host, port, "Bob"), "bob login");

    simple(a, SMB_MSG_CREATE_ROOM);
    RoomState st = latestRoomState(a);
    CHECK(st.ok && st.code.size() == 4 && st.count == 1 && st.host == a.id, "room created with alice as host");
    CHECK(ownersAre(st, a.id, 0xFF, 0xFF, 0xFF), "alice owns P1");
    CHECK(st.phase == SMB_ROOM_LOBBY, "lobby phase");
    const std::string code = st.code;

    // Mismatching joiners are rejected with a message naming the field.
    {
        TestClient x, y, z;
        CHECK(login(x, host, port, "WrongDisc", makeConfig(kDisc + 1)), "x login");
        simple(x, SMB_MSG_JOIN_ROOM, code.data(), 4);
        Err e = parseError(x.waitFor(SMB_MSG_ERROR));
        CHECK(e.ok && e.code == SMB_ERR_CONFIG_MISMATCH && e.text.find("disc") != std::string::npos,
              "disc mismatch rejected (%s)", e.text.c_str());
        CHECK(login(y, host, port, "WrongLocale", makeConfig(kDisc, 2)), "y login");
        simple(y, SMB_MSG_JOIN_ROOM, code.data(), 4);
        CHECK(expectError(y, SMB_ERR_CONFIG_MISMATCH), "locale mismatch rejected");
        CHECK(login(z, host, port, "WrongBuild", makeConfig(kDisc, 1, "other-build")), "z login");
        simple(z, SMB_MSG_JOIN_ROOM, code.data(), 4);
        CHECK(expectError(z, SMB_ERR_CONFIG_MISMATCH), "build mismatch rejected");
        simple(z, SMB_MSG_JOIN_ROOM, "ZZZZ", 4);
        CHECK(expectError(z, SMB_ERR_NO_SUCH_ROOM), "unknown code rejected");
        x.disconnect();
        y.disconnect();
        z.disconnect();
    }

    // Join with a lower-case code (case-insensitive).
    std::string lower = code;
    for (char &ch : lower) ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    simple(b, SMB_MSG_JOIN_ROOM, lower.data(), 4);
    st = latestRoomState(b);
    CHECK(st.ok && st.count == 2 && ownersAre(st, a.id, b.id, 0xFF, 0xFF), "bob joined on P2");
    a.pump(50);
    st = latestRoomState(a);
    CHECK(ownersAre(st, a.id, b.id, 0xFF, 0xFF), "alice sees bob");

    // Choose ports.
    sendChoose(b, 3);
    st = latestRoomState(a);
    CHECK(ownersAre(st, a.id, 0xFF, 0xFF, b.id), "bob moved to P4");
    sendChoose(b, 0);
    CHECK(expectError(b, SMB_ERR_TAKEN), "P1 is taken");
    sendChoose(b, 9);
    CHECK(expectError(b, SMB_ERR_BAD_MESSAGE), "no such port");
    sendChoose(b, 1);
    st = latestRoomState(a);
    CHECK(ownersAre(st, a.id, b.id, 0xFF, 0xFF), "bob back on P2");
    b.inbox.clear();

    // Start: only the host, only a matching config.
    sendStart(b, makeConfig());
    CHECK(expectError(b, SMB_ERR_NOT_HOST), "non-host can't start");
    sendStart(a, makeConfig(kDisc + 1));
    CHECK(expectError(a, SMB_ERR_CONFIG_MISMATCH), "start with a different disc hash rejected");
    sendStart(a, makeConfig(kDisc, 3));
    CHECK(expectError(a, SMB_ERR_CONFIG_MISMATCH), "start with a different locale rejected");
    sendStart(a, makeConfig(kDisc, 1, "test-build", SMB_NET_MAX_INPUT_DELAY + 1));
    CHECK(expectError(a, SMB_ERR_BAD_SETTINGS), "absurd input delay rejected");
    sendStart(a, makeConfig(kDisc, 1, "test-build", 2, 0x80));
    CHECK(expectError(a, SMB_ERR_BAD_SETTINGS), "unknown flag bits rejected");

    sendStart(a, makeConfig(kDisc, 1, "test-build", kDelay, SMB_CFG_FLAG_ALLOW_CROSS_ARCH));
    GameStart ga = waitGameStart(a), gb = waitGameStart(b);
    CHECK(ga.ok && gb.ok, "both got GAME_START");
    CHECK(ga.cfg.epoch != 0 && ga.cfg.epoch == gb.cfg.epoch, "server chose one epoch for both");
    CHECK(std::memcmp(&ga.cfg, &gb.cfg, sizeof(ga.cfg)) == 0, "identical session config");
    CHECK(ga.cfg.inputDelay == kDelay && ga.cfg.discHash == kDisc && ga.cfg.locale == 1 &&
              ga.cfg.flags == SMB_CFG_FLAG_ALLOW_CROSS_ARCH && std::strcmp(ga.cfg.buildId, "test-build") == 0,
          "config fields carried through");
    CHECK(std::memcmp(ga.owners, gb.owners, 4) == 0 && ga.owners[0] == a.id && ga.owners[1] == b.id &&
              ga.owners[2] == 0xFF,
          "port owners in GAME_START");
    st = latestRoomState(a);
    CHECK(st.ok && st.phase == SMB_ROOM_IN_GAME, "room is in game");

    // Mid-game join is refused.
    {
        TestClient late;
        CHECK(login(late, host, port, "Late"), "late login");
        simple(late, SMB_MSG_JOIN_ROOM, code.data(), 4);
        CHECK(expectError(late, SMB_ERR_IN_GAME), "joining a running game is refused");
        late.disconnect();
    }

    std::vector<Player> players = {{&a, {0}}, {&b, {1}}};
    CHECK(exchange(players, 0, 200, 0x03), "200 polls exchanged, bundles verified");

    // Checksums: matching ones pass silently.
    sendChecksum(a, 60, 0x1234);
    sendChecksum(b, 60, 0x1234);
    sendChecksum(a, 120, 0xAAAA);
    sendChecksum(b, 120, 0xAAAA);
    CHECK(!a.sees(SMB_MSG_DESYNC, 300) && !b.sees(SMB_MSG_DESYNC, 100), "matching checksums raise no desync");

    // Frames keep flowing, then a forced mismatch ends the game.
    CHECK(exchange(players, 200, 230, 0x03), "frames continue after checksums");
    sendChecksum(a, 180, 0x1111);
    sendChecksum(b, 180, 0x2222);
    Msg da = a.waitFor(SMB_MSG_DESYNC), db = b.waitFor(SMB_MSG_DESYNC);
    CHECK(da.size() == 5 && db.size() == 5, "DESYNC sent to both");
    if (da.size() == 5) {
        SmbNetReader r;
        smbr_init(&r, da.data() + 1, 4);
        CHECK(smbr_u32(&r) == 180, "DESYNC names poll 180");
    }
    Msg ea = a.waitFor(SMB_MSG_GAME_ENDED), eb = b.waitFor(SMB_MSG_GAME_ENDED);
    CHECK(ea.size() == 3 && ea[1] == SMB_END_DESYNC && eb.size() == 3 && eb[1] == SMB_END_DESYNC,
          "GAME_ENDED reason desync");
    st = latestRoomState(a);
    CHECK(st.ok && st.phase == SMB_ROOM_LOBBY && st.count == 2, "room returned to the lobby, members kept");
    st = latestRoomState(b, 300);
    CHECK(st.ok && st.phase == SMB_ROOM_LOBBY, "bob sees the lobby too");

    // The room persists: start again, polls restart at 0, new epoch.
    sendStart(a, makeConfig());
    GameStart g2a = waitGameStart(a), g2b = waitGameStart(b);
    CHECK(g2a.ok && g2b.ok && g2a.cfg.epoch != ga.cfg.epoch, "second game has a fresh epoch");
    CHECK(exchange(players, 0, 20, 0x03), "second game starts at poll 0");

    // Host ends the game: back to the lobby.
    simple(b, SMB_MSG_END_GAME);
    CHECK(expectError(b, SMB_ERR_NOT_HOST), "only the host can end the game");
    simple(a, SMB_MSG_END_GAME);
    ea = a.waitFor(SMB_MSG_GAME_ENDED);
    CHECK(ea.size() == 3 && ea[1] == SMB_END_HOST, "host ended the game");

    // Leaving: room persists while someone remains, closes when empty.
    simple(a, SMB_MSG_LEAVE_ROOM);
    st = latestRoomState(b);
    CHECK(st.ok && st.count == 1 && st.host == b.id && ownersAre(st, 0xFF, b.id, 0xFF, 0xFF), "host passed to bob");
    simple(b, SMB_MSG_LEAVE_ROOM);
    b.pump(100);
    simple(a, SMB_MSG_JOIN_ROOM, code.data(), 4);
    CHECK(expectError(a, SMB_ERR_NO_SUCH_ROOM), "empty room is gone");
    a.disconnect();
    b.disconnect();
}

void testThreePlayersDrop(const char *host, uint16_t port) {
    std::printf("[three players: full room, drop mid-game]\n");
    TestClient a, b, c, d, e;
    CHECK(login(a, host, port, "Alice"), "alice login");
    CHECK(login(b, host, port, "Bob"), "bob login");
    CHECK(login(c, host, port, "Carol"), "carol login");
    CHECK(login(d, host, port, "Dave"), "dave login");
    CHECK(login(e, host, port, "Eve"), "eve login");
    simple(a, SMB_MSG_CREATE_ROOM);
    const std::string code = latestRoomState(a).code;
    simple(b, SMB_MSG_JOIN_ROOM, code.data(), 4);
    simple(c, SMB_MSG_JOIN_ROOM, code.data(), 4);
    simple(d, SMB_MSG_JOIN_ROOM, code.data(), 4);
    RoomState st = latestRoomState(d);
    CHECK(st.ok && st.count == 4 && ownersAre(st, a.id, b.id, c.id, d.id), "four players seated P1-P4");
    simple(e, SMB_MSG_JOIN_ROOM, code.data(), 4);
    CHECK(expectError(e, SMB_ERR_ROOM_FULL), "fifth player refused: room full");
    simple(d, SMB_MSG_LEAVE_ROOM);
    st = latestRoomState(a);
    CHECK(st.ok && st.count == 3 && ownersAre(st, a.id, b.id, c.id, 0xFF), "dave left, P4 free");

    sendStart(a, makeConfig());
    GameStart ga = waitGameStart(a), gb = waitGameStart(b), gc = waitGameStart(c);
    CHECK(ga.ok && gb.ok && gc.ok && ga.cfg.epoch == gb.cfg.epoch && gb.cfg.epoch == gc.cfg.epoch,
          "three clients started with the same epoch");

    std::vector<Player> players = {{&a, {0}}, {&b, {1}}, {&c, {2}}};
    CHECK(exchange(players, 0, 100, 0x07), "100 polls with three players");

    // Carol stops after poll 100's frame: she has sent inputs up to poll 101 (delay). Alice and Bob go on
    // sending; the server waits on Carol -> WAITING, then she vanishes -> neutral from the next poll.
    const uint32_t kDrop = 102; // first poll Carol never supplied
    for (TestClient *cl : {&a, &b}) {
        const std::vector<int> ports = {cl == &a ? 0 : 1};
        for (uint32_t q = 100 + kDelay; q < kDrop + 40; q++) sendInput(*cl, q, ports);
    }
    for (uint32_t q = 100; q < kDrop; q++) {
        for (TestClient *cl : {&a, &b}) {
            Frame f = waitFrame(*cl);
            CHECK(f.ok && f.poll == q && frameMatches(f, 0x07), "frame %u still has all three (%s)", q,
                  cl->name.c_str());
        }
    }
    Msg waiting = a.waitFor(SMB_MSG_WAITING, 3000);
    CHECK(waiting.size() == 4 && waiting[1] == 0x04, "WAITING names carol's port (mask 0x%x)",
          waiting.size() == 4 ? waiting[1] : 0);
    CHECK(b.sees(SMB_MSG_NET_STATS, 2500), "NET_STATS arrives during a game");

    c.disconnect();
    Msg dropped = a.waitFor(SMB_MSG_PLAYER_DROPPED, 3000);
    CHECK(dropped.size() == 7 && dropped[1] == c.id && dropped[2] == 0x04, "PLAYER_DROPPED for carol's port");
    uint32_t dropPoll = 0;
    if (dropped.size() == 7) {
        SmbNetReader r;
        smbr_init(&r, dropped.data() + 3, 4);
        dropPoll = smbr_u32(&r);
    }
    CHECK(dropPoll == kDrop, "carol's port goes neutral at poll %u (got %u)", kDrop, dropPoll);
    b.waitFor(SMB_MSG_PLAYER_DROPPED, 1000);

    // From there on frames carry only alice and bob; carol's port reads neutral.
    bool ok = true, flagged = false;
    for (uint32_t q = dropPoll; q < dropPoll + 20; q++) {
        for (TestClient *cl : {&a, &b}) {
            Frame f = waitFrame(*cl);
            if (!f.ok || f.poll != q || !frameMatches(f, 0x03)) {
                std::printf("  bad frame after drop for %s: ok=%d poll=%u (want %u) mask=0x%x\n", cl->name.c_str(),
                            f.ok, f.poll, q, f.mask);
                ok = false;
            }
            if (q == dropPoll && (f.flags & SMB_FRAME_PORTS_CHANGED)) flagged = true;
        }
    }
    CHECK(ok, "alice and bob keep receiving frames with port 3 neutral");
    CHECK(flagged, "first frame after the drop has PORTS_CHANGED");

    // Checksums now only need the two remaining players.
    sendChecksum(a, 120, 7);
    sendChecksum(b, 120, 7);
    CHECK(!a.sees(SMB_MSG_DESYNC, 300), "checksum among remaining players matches");

    // Everyone gone: the game ends and the room closes.
    simple(a, SMB_MSG_LEAVE_ROOM);
    b.pump(100);
    b.inbox.clear();
    b.disconnect();
    a.pump(100);
    simple(a, SMB_MSG_JOIN_ROOM, code.data(), 4);
    CHECK(expectError(a, SMB_ERR_NO_SUCH_ROOM), "room closed after the last player left");
    a.disconnect();
    d.disconnect();
    e.disconnect();
}

void testHostDropsLast(const char *host, uint16_t port) {
    std::printf("[explicit leave mid-game by the host, last human ends the game]\n");
    TestClient a, b;
    CHECK(login(a, host, port, "Alice"), "alice login");
    CHECK(login(b, host, port, "Bob"), "bob login");
    simple(a, SMB_MSG_CREATE_ROOM);
    const std::string code = latestRoomState(a).code;
    simple(b, SMB_MSG_JOIN_ROOM, code.data(), 4);
    latestRoomState(b);
    sendStart(a, makeConfig());
    CHECK(waitGameStart(a).ok && waitGameStart(b).ok, "started");
    std::vector<Player> players = {{&a, {0}}, {&b, {1}}};
    CHECK(exchange(players, 0, 30, 0x03), "30 polls");
    // Alice leaves; Bob alone keeps going with his own input only.
    simple(a, SMB_MSG_LEAVE_ROOM);
    Msg dropped = b.waitFor(SMB_MSG_PLAYER_DROPPED);
    CHECK(dropped.size() == 7 && dropped[2] == 0x01, "bob told alice's port dropped");
    bool ok = true;
    uint32_t first = 0;
    if (dropped.size() == 7) {
        SmbNetReader r;
        smbr_init(&r, dropped.data() + 3, 4);
        first = smbr_u32(&r);
    }
    CHECK(first == 32, "drop takes effect at poll 32 (got %u)", first);
    for (uint32_t q = first; q < first + 40; q++) sendInput(b, q, {1}); // bob had sent up to poll 31
    for (uint32_t q = 30; q < first; q++) {
        Frame f = waitFrame(b); // flushed before alice left: still both
        if (!f.ok || f.poll != q || !frameMatches(f, 0x03)) ok = false;
    }
    for (uint32_t q = first; q < first + 10; q++) {
        Frame f = waitFrame(b);
        if (!f.ok || f.poll != q || !frameMatches(f, 0x02)) ok = false;
    }
    CHECK(ok, "bob alone: frames carry only port 2");
    simple(b, SMB_MSG_LEAVE_ROOM);
    b.pump(100);
    a.disconnect();
    b.disconnect();
}

// E8: a member restarts its program for the session (RELAUNCHING, disconnect, new connection, RESUME).
void testResume(const char *host, uint16_t port) {
    std::printf("[resume: restart for the session]\n");
    TestClient a, b;
    CHECK(login(a, host, port, "Alice"), "alice login");
    CHECK(login(b, host, port, "Bob"), "bob login");
    simple(a, SMB_MSG_CREATE_ROOM);
    const std::string code = latestRoomState(a).code;
    simple(b, SMB_MSG_JOIN_ROOM, code.data(), 4);
    latestRoomState(b);
    sendStart(a, makeConfig());
    GameStart ga = waitGameStart(a), gb = waitGameStart(b);
    CHECK(ga.ok && gb.ok, "both got GAME_START");

    // Both restart: RELAUNCHING(token), disconnect.  The game must not end or drop anyone.
    const uint32_t tokA = 0xA1B2C3D4u, tokB = 0x01020304u;
    const uint8_t aid = a.id, bid = b.id;
    uint8_t tb[4];
    for (int i = 0; i < 4; i++) tb[i] = static_cast<uint8_t>(tokA >> (8 * i));
    simple(a, SMB_MSG_RELAUNCHING, tb, 4);
    for (int i = 0; i < 4; i++) tb[i] = static_cast<uint8_t>(tokB >> (8 * i));
    simple(b, SMB_MSG_RELAUNCHING, tb, 4);
    a.disconnectLater();
    b.disconnectLater();

    TestClient a2, b2;
    CHECK(login(b2, host, port, "Bob"), "bob's new process connects");
    auto resume = [&](TestClient &c, uint8_t oldId, uint32_t token) {
        uint8_t p[9];
        std::memcpy(p, code.data(), 4);
        p[4] = oldId;
        for (int i = 0; i < 4; i++) p[5 + i] = static_cast<uint8_t>(token >> (8 * i));
        simple(c, SMB_MSG_RESUME, p, 9);
    };
    resume(b2, bid, 0xBADBADu);
    CHECK(expectError(b2, SMB_ERR_NO_SUCH_ROOM), "wrong token is refused");
    resume(b2, bid, tokB);
    Msg wb = b2.waitFor(SMB_MSG_WELCOME);
    CHECK(wb.size() >= 2 && wb[1] == bid, "resumed client gets its old id back");
    GameStart g2 = waitGameStart(b2);
    CHECK(g2.ok && std::memcmp(&g2.cfg, &ga.cfg, sizeof(ga.cfg)) == 0 && std::memcmp(g2.owners, ga.owners, 4) == 0,
          "GAME_START repeated unchanged (same epoch, same owners)");
    CHECK(login(a2, host, port, "Alice"), "alice's new process connects");
    resume(a2, aid, tokA);
    GameStart g3 = waitGameStart(a2);
    CHECK(g3.ok && std::memcmp(&g3.cfg, &ga.cfg, sizeof(ga.cfg)) == 0, "alice resumed too");
    std::vector<Player> players = {{&a2, {0}}, {&b2, {1}}};
    CHECK(exchange(players, 0, 100, 0x03), "lockstep runs from poll 0 after both resumed");
    // A second RESUME with the same token (old seat already taken over) is refused.
    TestClient thief;
    CHECK(login(thief, host, port, "Thief"), "thief login");
    resume(thief, aid, tokA);
    CHECK(expectError(thief, SMB_ERR_NO_SUCH_ROOM), "a seat that is back can't be taken again");
    thief.disconnect();
    // A plain disconnect (no RELAUNCHING) is still a drop.
    b2.disconnect();
    Msg dropped = a2.waitFor(SMB_MSG_PLAYER_DROPPED);
    CHECK(dropped.size() >= 2 && dropped[1] == bid, "a plain disconnect is a drop");
    a2.disconnect();
}

} // namespace

int main(int argc, char **argv) {
    const char *host = argc > 1 ? argv[1] : "127.0.0.1";
    const uint16_t port = argc > 2 ? static_cast<uint16_t>(std::atoi(argv[2])) : SMB_NET_DEFAULT_PORT;
    if (enet_initialize() != 0) {
        std::fprintf(stderr, "enet_initialize failed\n");
        return 2;
    }
    {
        TestClient probe;
        if (!probe.connect(host, port)) {
            std::fprintf(stderr, "cannot reach the server at %s:%u\n", host, port);
            return 2;
        }
        probe.disconnect();
    }
    testVersionMismatch(host, port);
    testTwoPlayers(host, port);
    testThreePlayersDrop(host, port);
    testHostDropsLast(host, port);
    testResume(host, port);
    std::printf("%d checks, %d failure(s)\n", gChecks, gFailures);
    std::printf(gFailures ? "FAILED\n" : "ALL PASSED\n");
    return gFailures ? 1 : 0;
}
