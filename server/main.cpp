// SMB2 PC netplay relay server.
//
// Clients create or join 4-letter rooms. When the host starts a game, the server sends the session
// config to everyone, then for every poll collects each human port's pad and broadcasts the bundle once
// all of them have arrived (lockstep). Clients report periodic checksums; the server compares them and
// announces a desync (the room then returns to the lobby). No game code runs here.

#include "netplay/protocol.h"

#include <enet/enet.h>

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <vector>

namespace {

constexpr uint32_t kMaxPollsAhead = 600; // reject input this far past the oldest pending poll
constexpr size_t kMaxClients = 256;
constexpr uint32_t kMaxPendingChecksums = 16;

void logf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
void logf(const char *fmt, ...) {
    char stamp[32];
    const std::time_t now = std::time(nullptr);
    std::strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", std::gmtime(&now));
    std::printf("%s ", stamp);
    va_list args;
    va_start(args, fmt);
    std::vprintf(fmt, args);
    va_end(args);
    std::printf("\n");
    std::fflush(stdout);
}

struct Room;

struct Client {
    uint8_t id = 0;
    ENetPeer *peer = nullptr;
    bool helloed = false;
    std::string name;
    SmbNetSessionConfig cfg{}; // buildId, discHash, locale from HELLO
    Room *room = nullptr;
    uint32_t lastSentRtt = 0; // the ping the last ROOM_STATE showed
    // Stats for NET_STATS.
    uint32_t lateMs = 0;      // EMA baseline of how long after the first input of a poll this client's arrives
    uint16_t lateCount = 0;
    uint16_t slowPolls = 0, stalls = 0, worstMs = 0;
    // E8 resume: the process is restarting for the session (RELAUNCHING); its seat is held until RESUME.
    bool relaunching = false;
    bool away = false;         // relaunching and disconnected: no peer
    uint32_t token = 0;
    uint32_t awayUntilMs = 0;
};

struct PendingPoll {
    uint8_t have = 0; // bit per port whose input arrived
    uint32_t firstMs = 0;
    std::array<uint32_t, SMB_NET_MAX_PLAYERS> arrivedMs{};
    std::array<std::array<uint8_t, SMB_NET_PAD_SIZE>, SMB_NET_MAX_PLAYERS> pads{};
};

struct Room {
    std::string code;
    SmbNetSessionConfig cfg{}; // the creator's buildId/discHash/locale: what everyone must match
    uint8_t hostId = 0;
    std::vector<uint8_t> members; // client ids, join order
    // Lobby seating: who plays each port. Every member owns exactly one port.
    std::array<uint8_t, SMB_NET_MAX_PLAYERS> slotOwner{SMB_NET_PORT_NONE, SMB_NET_PORT_NONE, SMB_NET_PORT_NONE,
                                                       SMB_NET_PORT_NONE};
    bool inGame = false;
    std::array<uint8_t, SMB_NET_MAX_PLAYERS> portOwner{}; // snapshot at game start
    SmbNetSessionConfig startCfg{}; // what GAME_START carried (epoch filled in), re-sent on RESUME
    uint8_t humanMask = 0;
    uint8_t lastSentMask = 0;
    uint32_t nextPoll = 0;
    uint8_t gameSeq = 0;
    uint32_t lastProgressMs = 0;   // when the last frame went out (or the game started)
    uint32_t lastWaitingMs = 0;    // when the last WAITING went out
    uint32_t lastLobbyStateMs = 0; // lobby: when ROOM_STATE (with fresh pings) last went out
    uint32_t lastStatsMs = 0;      // in game: when NET_STATS last went out
    std::map<uint32_t, PendingPoll> polls;
    std::map<uint32_t, std::map<uint8_t, uint32_t>> checksums;
};

std::map<uint8_t, std::unique_ptr<Client>> gClients;
std::map<std::string, std::unique_ptr<Room>> gRooms;
std::mt19937_64 gRng{std::random_device{}()};

void send(Client &c, uint8_t channel, const SmbNetWriter &w) {
    if (!c.peer) return; // away (restarting)
    if (w.overflow) {
        logf("BUG: outgoing message overflowed its buffer");
        return;
    }
    ENetPacket *packet = enet_packet_create(w.data, w.pos, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(c.peer, channel, packet);
}

void sendError(Client &c, SmbNetError code, const char *message) {
    uint8_t buf[256];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_ERROR);
    smbw_u8(&w, code);
    smbw_str(&w, message, 200);
    send(c, SMB_NET_CHANNEL_CONTROL, w);
}

void broadcast(Room &room, uint8_t channel, const SmbNetWriter &w) {
    for (uint8_t id : room.members) send(*gClients.at(id), channel, w);
}

int portOf(const Room &room, uint8_t id) {
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room.slotOwner[port] == id) return port;
    }
    return -1;
}

// A newcomer takes the lowest free port.
void seat(Room &room, uint8_t id) {
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room.slotOwner[port] == SMB_NET_PORT_NONE) {
            room.slotOwner[port] = id;
            return;
        }
    }
}

void sendRoomState(Room &room) {
    room.lastLobbyStateMs = enet_time_get();
    uint8_t buf[512];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_ROOM_STATE);
    smbw_bytes(&w, room.code.data(), SMB_NET_ROOM_CODE_LEN);
    smbw_u8(&w, room.hostId);
    smbw_u8(&w, static_cast<uint8_t>(room.members.size()));
    for (uint8_t id : room.members) {
        Client &member = *gClients.at(id);
        smbw_u8(&w, id);
        smbw_str(&w, member.name.c_str(), SMB_NET_MAX_NAME);
        member.lastSentRtt = member.peer ? member.peer->roundTripTime : 0;
        smbw_u16(&w, static_cast<uint16_t>(std::min<enet_uint32>(member.lastSentRtt, 65535)));
    }
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) smbw_u8(&w, room.slotOwner[port]);
    smbw_u8(&w, room.inGame ? SMB_ROOM_IN_GAME : SMB_ROOM_LOBBY);
    broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
}

std::string newRoomCode() {
    static const char letters[] = "ABCDEFGHJKLMNPQRSTUVWXYZ"; // no I/O, easy to read aloud
    std::uniform_int_distribution<int> pick(0, sizeof(letters) - 2);
    for (;;) {
        std::string code;
        for (int i = 0; i < SMB_NET_ROOM_CODE_LEN; i++) code += letters[pick(gRng)];
        if (!gRooms.count(code)) return code;
    }
}

// Ends the running game: everyone goes back to the room's lobby.
void endGame(Room &room, SmbNetEndReason reason, uint8_t who) {
    uint8_t buf[8];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_GAME_ENDED);
    smbw_u8(&w, reason);
    smbw_u8(&w, who);
    broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
    room.inGame = false;
    room.humanMask = 0;
    for (uint8_t id : room.members) {
        Client &m = *gClients.at(id);
        m.relaunching = false;
        if (m.away) m.awayUntilMs = enet_time_get(); // reaped by tick()
    }
    room.polls.clear();
    room.checksums.clear();
    logf("room %s: game ended (reason %d)", room.code.c_str(), reason);
    if (!room.members.empty()) sendRoomState(room);
}

// Records which client sent the last input for each flushed poll, and flags clients that are late beyond
// their own usual lateness (EMA baseline, so a steady phase offset doesn't count).
void noteLateness(Room &room, const PendingPoll &pending) {
    int last = -1;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (!(room.humanMask & (1 << port))) continue;
        if (last < 0 || pending.arrivedMs[port] > pending.arrivedMs[last]) last = port;
    }
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (!(room.humanMask & (1 << port))) continue;
        Client &c = *gClients.at(room.portOwner[port]);
        const uint32_t late = pending.arrivedMs[port] - pending.firstMs;
        if (port == last && late >= c.lateMs + SMB_NET_LATE_MS && (room.humanMask & (room.humanMask - 1)) != 0) {
            c.lateCount++;
            if (c.lateCount == 1 || c.lateCount % 100 == 0) {
                logf("room %s: %s held the game up (%u ms behind its usual)", room.code.c_str(), c.name.c_str(),
                     late - c.lateMs);
            }
        }
        c.lateMs = (c.lateMs * 7 + late) / 8;
    }
}

// Sends every poll whose inputs are complete, in order.
void flushFrames(Room &room) {
    for (;;) {
        auto it = room.polls.find(room.nextPoll);
        if (it == room.polls.end() || (it->second.have & room.humanMask) != room.humanMask) return;
        noteLateness(room, it->second);
        uint8_t buf[8 + SMB_NET_MAX_PLAYERS * SMB_NET_PAD_SIZE];
        SmbNetWriter w;
        smbw_init(&w, buf, sizeof(buf));
        smbw_u8(&w, SMB_MSG_FRAME);
        smbw_u32(&w, room.nextPoll);
        smbw_u8(&w, room.gameSeq);
        smbw_u8(&w, room.humanMask);
        smbw_u8(&w, room.humanMask != room.lastSentMask ? SMB_FRAME_PORTS_CHANGED : 0);
        for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
            if (room.humanMask & (1 << port)) smbw_bytes(&w, it->second.pads[port].data(), SMB_NET_PAD_SIZE);
        }
        room.lastSentMask = room.humanMask;
        broadcast(room, SMB_NET_CHANNEL_INPUT, w);
        room.polls.erase(it);
        room.lastProgressMs = enet_time_get();
        room.nextPoll++;
    }
}

// Mid-game disconnect: the player's ports stop being human from the next poll the server sends, so every
// client sees them neutral on the same poll. The game ends only if no humans remain.
void dropFromGame(Room &room, Client &c) {
    uint8_t ports = 0;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room.portOwner[port] == c.id) ports |= static_cast<uint8_t>(1 << port);
    }
    room.humanMask &= static_cast<uint8_t>(~ports);
    for (auto &entry : room.checksums) entry.second.erase(c.id);
    if (room.humanMask == 0) {
        endGame(room, SMB_END_PLAYER_LEFT, c.id);
        return;
    }
    uint8_t buf[8];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_PLAYER_DROPPED);
    smbw_u8(&w, c.id);
    smbw_u8(&w, ports);
    smbw_u32(&w, room.nextPoll);
    broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
    logf("room %s: %s dropped at poll %u; ports 0x%x go neutral", room.code.c_str(), c.name.c_str(), room.nextPoll,
         ports);
    flushFrames(room); // polls that were only waiting on the dropped player can go now
}

void leaveRoom(Client &c) {
    Room *room = c.room;
    if (!room) return;
    c.room = nullptr;
    room->members.erase(std::find(room->members.begin(), room->members.end(), c.id));
    // The game keeps its own snapshot (portOwner); only the lobby seat is freed.
    if (const int port = portOf(*room, c.id); port >= 0) room->slotOwner[port] = SMB_NET_PORT_NONE;
    logf("room %s: %s left (%zu remaining)", room->code.c_str(), c.name.c_str(), room->members.size());
    if (room->members.empty()) {
        logf("room %s: closed", room->code.c_str());
        gRooms.erase(room->code);
        return;
    }
    if (room->inGame) dropFromGame(*room, c);
    if (room->hostId == c.id) room->hostId = room->members.front();
    if (!room->members.empty()) sendRoomState(*room);
}

void sendWelcome(Client &c) {
    uint8_t buf[4];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_WELCOME);
    smbw_u8(&w, c.id);
    smbw_u8(&w, SMB_CAP_RESUME); // trailing capability byte (older clients ignore it)
    send(c, SMB_NET_CHANNEL_CONTROL, w);
}

void onHello(Client &c, SmbNetReader &r) {
    char name[SMB_NET_MAX_NAME + 1];
    char build[SMB_NET_MAX_BUILD_ID + 1];
    const uint16_t version = smbr_u16(&r);
    smbr_str(&r, name, sizeof(name));
    smbr_str(&r, build, sizeof(build));
    const uint32_t discHash = smbr_u32(&r);
    const uint8_t locale = smbr_u8(&r);
    if (r.error) {
        sendError(c, SMB_ERR_BAD_MESSAGE, "malformed HELLO");
        return;
    }
    if (version != SMB_NET_PROTOCOL_VERSION) {
        char msg[160];
        std::snprintf(msg, sizeof(msg), "this server speaks protocol %d, your SMB2 protocol %u: %s",
                      SMB_NET_PROTOCOL_VERSION, version,
                      version < SMB_NET_PROTOCOL_VERSION ? "update your game" : "the server needs updating");
        sendError(c, SMB_ERR_VERSION, msg);
        return;
    }
    c.helloed = true;
    c.name = name[0] ? name : "Player";
    std::memset(&c.cfg, 0, sizeof(c.cfg));
    std::memcpy(c.cfg.buildId, build, std::strlen(build));
    c.cfg.discHash = discHash;
    c.cfg.locale = locale;
    sendWelcome(c);
    logf("client %u: hello from '%s' (build %s, disc %08x, locale %u)", c.id, c.name.c_str(), build, discHash, locale);
}

void onCreateRoom(Client &c) {
    leaveRoom(c);
    auto room = std::make_unique<Room>();
    room->code = newRoomCode();
    room->cfg = c.cfg;
    room->hostId = c.id;
    room->members.push_back(c.id);
    seat(*room, c.id);
    c.room = room.get();
    logf("room %s: created by %s", room->code.c_str(), c.name.c_str());
    Room &ref = *room;
    gRooms[room->code] = std::move(room);
    sendRoomState(ref);
}

void onJoinRoom(Client &c, SmbNetReader &r) {
    char code[SMB_NET_ROOM_CODE_LEN + 1] = {};
    smbr_bytes(&r, code, SMB_NET_ROOM_CODE_LEN);
    for (char &ch : code) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed JOIN_ROOM");
    auto it = gRooms.find(code);
    if (it == gRooms.end()) return sendError(c, SMB_ERR_NO_SUCH_ROOM, "no room with that code");
    Room &room = *it->second;
    if (c.room == &room) return;
    if (const char *field = smb_config_mismatch(&c.cfg, &room.cfg)) {
        char msg[160];
        std::snprintf(msg, sizeof(msg), "your %s differs from the host's: everyone needs the same %s", field, field);
        return sendError(c, SMB_ERR_CONFIG_MISMATCH, msg);
    }
    if (room.inGame) return sendError(c, SMB_ERR_IN_GAME, "that room's game is already running");
    if (room.members.size() >= SMB_NET_MAX_PLAYERS) return sendError(c, SMB_ERR_ROOM_FULL, "room is full");
    leaveRoom(c);
    room.members.push_back(c.id);
    seat(room, c.id);
    c.room = &room;
    logf("room %s: %s joined (%zu players)", room.code.c_str(), c.name.c_str(), room.members.size());
    sendRoomState(room);
}

// GAME_START to everyone (to == nullptr) or to one client (RESUME).
void sendGameStart(Room &room, Client *to) {
    uint8_t buf[96];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_GAME_START);
    smbw_config(&w, &room.startCfg);
    smbw_bytes(&w, room.portOwner.data(), SMB_NET_MAX_PLAYERS);
    smbw_u8(&w, room.gameSeq);
    if (to) send(*to, SMB_NET_CHANNEL_CONTROL, w);
    else broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
}

void startGame(Room &room, SmbNetSessionConfig cfg) {
    room.portOwner = room.slotOwner;
    uint8_t humanMask = 0;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room.slotOwner[port] != SMB_NET_PORT_NONE) humanMask |= static_cast<uint8_t>(1 << port);
    }
    cfg.epoch = gRng();
    room.startCfg = cfg;
    room.humanMask = humanMask;
    room.lastSentMask = humanMask;
    room.inGame = true;
    room.nextPoll = 0;
    room.gameSeq = room.gameSeq == 255 ? 1 : static_cast<uint8_t>(room.gameSeq + 1); // never 0
    room.lastProgressMs = room.lastStatsMs = enet_time_get();
    room.polls.clear();
    room.checksums.clear();
    for (uint8_t id : room.members) {
        Client &m = *gClients.at(id);
        m.relaunching = false;
        m.lateMs = 0;
        m.lateCount = m.slowPolls = m.stalls = m.worstMs = 0;
    }

    sendGameStart(room, nullptr);
    sendRoomState(room);
    logf("room %s: game started (delay %u, epoch %016llx, human ports 0x%x)", room.code.c_str(), cfg.inputDelay,
         static_cast<unsigned long long>(cfg.epoch), humanMask);
}

void onStartGame(Client &c, SmbNetReader &r) {
    Room *room = c.room;
    if (!room) return sendError(c, SMB_ERR_NO_SUCH_ROOM, "not in a room");
    if (room->hostId != c.id) return sendError(c, SMB_ERR_NOT_HOST, "only the host can start");
    if (room->inGame) return sendError(c, SMB_ERR_IN_GAME, "already playing");
    SmbNetSessionConfig cfg;
    smbr_config(&r, &cfg);
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed START_GAME");
    if (const char *field = smb_config_mismatch(&cfg, &room->cfg)) {
        char msg[160];
        std::snprintf(msg, sizeof(msg), "the session's %s doesn't match the room's", field);
        return sendError(c, SMB_ERR_CONFIG_MISMATCH, msg);
    }
    if (!smb_config_valid(&cfg)) return sendError(c, SMB_ERR_BAD_SETTINGS, "invalid session settings");
    startGame(*room, cfg);
}

void onChoose(Client &c, SmbNetReader &r) {
    Room *room = c.room;
    const uint8_t wantPort = smbr_u8(&r);
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed CHOOSE");
    if (!room) return sendError(c, SMB_ERR_NO_SUCH_ROOM, "not in a room");
    if (room->inGame) return sendError(c, SMB_ERR_IN_GAME, "the game has started");
    if (wantPort != SMB_NET_KEEP && wantPort >= SMB_NET_MAX_PLAYERS) {
        return sendError(c, SMB_ERR_BAD_MESSAGE, "no such port");
    }
    const int port = portOf(*room, c.id);
    if (port < 0) return sendError(c, SMB_ERR_BAD_MESSAGE, "you have no port"); // can't happen
    if (wantPort == SMB_NET_KEEP || wantPort == port) return;
    if (room->slotOwner[wantPort] != SMB_NET_PORT_NONE) {
        char msg[96];
        std::snprintf(msg, sizeof(msg), "P%d is taken by %s", wantPort + 1,
                      gClients.at(room->slotOwner[wantPort])->name.c_str());
        return sendError(c, SMB_ERR_TAKEN, msg);
    }
    room->slotOwner[port] = SMB_NET_PORT_NONE;
    room->slotOwner[wantPort] = c.id;
    logf("room %s: %s is P%d", room->code.c_str(), c.name.c_str(), wantPort + 1);
    sendRoomState(*room);
}

void onInput(Client &c, SmbNetReader &r) {
    Room *room = c.room;
    if (!room || !room->inGame) return; // stale input after a game ended
    const uint32_t poll = smbr_u32(&r);
    const uint8_t seq = smbr_u8(&r);
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed INPUT");
    if (seq != room->gameSeq) return; // from a previous game
    if (poll < room->nextPoll || poll >= room->nextPoll + kMaxPollsAhead) return;
    PendingPoll &pending = room->polls[poll];
    const uint32_t now = enet_time_get();
    if (pending.have == 0) pending.firstMs = now;
    uint8_t got = 0;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room->portOwner[port] != c.id || !(room->humanMask & (1 << port))) continue;
        smbr_bytes(&r, pending.pads[port].data(), SMB_NET_PAD_SIZE);
        pending.arrivedMs[port] = now;
        got |= static_cast<uint8_t>(1 << port);
    }
    if (r.error || r.pos != r.size) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed INPUT");
    pending.have |= got;
    flushFrames(*room);
}

void onChecksum(Client &c, SmbNetReader &r) {
    Room *room = c.room;
    if (!room || !room->inGame) return;
    const uint32_t poll = smbr_u32(&r);
    const uint32_t hash = smbr_u32(&r);
    if (r.error) return;
    auto &reports = room->checksums[poll];
    reports[c.id] = hash;
    // Drop stale entries (a client that never reports would otherwise grow this forever).
    while (room->checksums.size() > kMaxPendingChecksums) room->checksums.erase(room->checksums.begin());
    size_t playing = 0;
    for (int port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (room->humanMask & (1 << port)) playing++;
    }
    if (reports.size() < playing) return;
    bool agree = true;
    for (const auto &entry : reports) agree &= entry.second == reports.begin()->second;
    room->checksums.erase(poll);
    if (agree) return;

    logf("room %s: DESYNC at poll %u", room->code.c_str(), poll);
    uint8_t buf[8];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_DESYNC);
    smbw_u32(&w, poll);
    broadcast(*room, SMB_NET_CHANNEL_CONTROL, w);
    endGame(*room, SMB_END_DESYNC, SMB_NET_CLIENT_ID_NONE);
}

void onPerf(Client &c, SmbNetReader &r) {
    smbr_u32(&r); // poll
    smbr_u16(&r); // polls
    const uint16_t slow = smbr_u16(&r);
    const uint16_t stalls = smbr_u16(&r);
    const uint16_t worst = smbr_u16(&r);
    if (r.error) return;
    c.slowPolls = static_cast<uint16_t>(std::min<uint32_t>(c.slowPolls + slow, 65535));
    c.stalls = static_cast<uint16_t>(std::min<uint32_t>(c.stalls + stalls, 65535));
    c.worstMs = std::max(c.worstMs, worst);
}

void onEndGame(Client &c) {
    Room *room = c.room;
    if (!room) return sendError(c, SMB_ERR_NO_SUCH_ROOM, "not in a room");
    if (room->hostId != c.id) return sendError(c, SMB_ERR_NOT_HOST, "only the host can end the game");
    if (!room->inGame) return;
    endGame(*room, SMB_END_HOST, c.id);
}

// E8: the client got GAME_START and is restarting its program for the session; hold its seat.
void onRelaunching(Client &c, SmbNetReader &r) {
    const uint32_t token = smbr_u32(&r);
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed RELAUNCHING");
    if (!c.room || !c.room->inGame) return; // only meaningful after GAME_START
    c.relaunching = true;
    c.token = token;
    logf("room %s: %s is restarting for the session", c.room->code.c_str(), c.name.c_str());
}

// E8: the restarted process takes over the old member's seat (same client id, so nothing else changes).
void onResume(Client &c, SmbNetReader &r) {
    char code[SMB_NET_ROOM_CODE_LEN + 1] = {};
    smbr_bytes(&r, code, SMB_NET_ROOM_CODE_LEN);
    for (char &ch : code) ch = static_cast<char>(std::toupper(static_cast<unsigned char>(ch)));
    const uint8_t oldId = smbr_u8(&r);
    const uint32_t token = smbr_u32(&r);
    if (r.error) return sendError(c, SMB_ERR_BAD_MESSAGE, "malformed RESUME");
    const auto room = gRooms.find(code);
    const auto old = gClients.find(oldId);
    if (room == gRooms.end() || !room->second->inGame || old == gClients.end() || old->second.get() == &c ||
        old->second->room != room->second.get() || !old->second->relaunching || old->second->token != token) {
        return sendError(c, SMB_ERR_NO_SUCH_ROOM, "can't resume: the game is gone, or the seat was given up");
    }
    Client &o = *old->second;
    Room &rm = *room->second;
    if (o.peer) { // the old process's disconnect hasn't been seen yet
        o.peer->data = nullptr;
        enet_peer_disconnect_now(o.peer, 0);
    }
    o.peer = c.peer;
    o.peer->data = &o;
    o.away = false;
    o.relaunching = false;
    o.helloed = true;
    c.peer = nullptr;
    logf("room %s: %s is back (client %u)", rm.code.c_str(), o.name.c_str(), o.id);
    sendWelcome(o);
    sendRoomState(rm);
    sendGameStart(rm, &o);
    gClients.erase(c.id); // c is gone: the caller must not touch it
}

void onReceive(Client &c, ENetPacket *packet) {
    SmbNetReader r;
    smbr_init(&r, packet->data, packet->dataLength);
    const uint8_t type = smbr_u8(&r);
    if (!c.helloed && type != SMB_MSG_HELLO) return sendError(c, SMB_ERR_BAD_MESSAGE, "say HELLO first");
    switch (type) {
    case SMB_MSG_HELLO: return onHello(c, r);
    case SMB_MSG_CREATE_ROOM: return onCreateRoom(c);
    case SMB_MSG_JOIN_ROOM: return onJoinRoom(c, r);
    case SMB_MSG_LEAVE_ROOM: return leaveRoom(c);
    case SMB_MSG_START_GAME: return onStartGame(c, r);
    case SMB_MSG_INPUT: return onInput(c, r);
    case SMB_MSG_CHECKSUM: return onChecksum(c, r);
    case SMB_MSG_CHOOSE: return onChoose(c, r);
    case SMB_MSG_PERF: return onPerf(c, r);
    case SMB_MSG_END_GAME: return onEndGame(c);
    case SMB_MSG_RELAUNCHING: return onRelaunching(c, r);
    case SMB_MSG_RESUME: return onResume(c, r);
    default: return sendError(c, SMB_ERR_BAD_MESSAGE, "unknown message type");
    }
}

void sendNetStats(Room &room) {
    uint8_t buf[8 + SMB_NET_MAX_PLAYERS * 16];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof(buf));
    smbw_u8(&w, SMB_MSG_NET_STATS);
    smbw_u8(&w, static_cast<uint8_t>(room.members.size()));
    for (uint8_t id : room.members) {
        const Client &m = *gClients.at(id);
        const ENetPeer *p = m.peer;
        smbw_u8(&w, id);
        smbw_u16(&w, static_cast<uint16_t>(p ? std::min<enet_uint32>(p->roundTripTime, 65535) : 0));
        smbw_u16(&w, static_cast<uint16_t>(p ? std::min<enet_uint32>(p->roundTripTimeVariance, 65535) : 0));
        smbw_u8(&w, static_cast<uint8_t>(p ? std::min<enet_uint32>(p->packetLoss * 100 / ENET_PEER_PACKET_LOSS_SCALE, 100) : 0));
        smbw_u16(&w, m.lateCount);
        smbw_u16(&w, m.slowPolls);
        smbw_u16(&w, m.stalls);
        smbw_u16(&w, m.worstMs);
    }
    broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
}

// Called often from the main loop: tells players who a stalled game is waiting for, sends NET_STATS and
// refreshes lobby pings.
void tick() {
    const uint32_t now = enet_time_get();
    {   // E8: members that restarted for a session and never came back
        std::vector<uint8_t> expired;
        for (auto &entry : gClients) {
            const Client &m = *entry.second;
            if (m.away && static_cast<int32_t>(now - m.awayUntilMs) >= 0) expired.push_back(m.id);
        }
        for (uint8_t id : expired) {
            Client &m = *gClients.at(id);
            logf("client %u: did not come back from its restart", id);
            leaveRoom(m);
            gClients.erase(id);
        }
    }
    for (auto &entry : gRooms) {
        Room &room = *entry.second;
        if (room.inGame) {
            const uint32_t waited = now - room.lastProgressMs;
            if (waited >= 500 && now - room.lastWaitingMs >= 500) {
                const auto it = room.polls.find(room.nextPoll);
                const uint8_t have = it == room.polls.end() ? 0 : it->second.have;
                const uint8_t missing = static_cast<uint8_t>(room.humanMask & ~have);
                uint8_t buf[8];
                SmbNetWriter w;
                smbw_init(&w, buf, sizeof(buf));
                smbw_u8(&w, SMB_MSG_WAITING);
                smbw_u8(&w, missing);
                smbw_u16(&w, static_cast<uint16_t>(std::min<uint32_t>(waited, 65535)));
                broadcast(room, SMB_NET_CHANNEL_CONTROL, w);
                room.lastWaitingMs = now;
            }
            if (now - room.lastStatsMs >= 1000) {
                room.lastStatsMs = now;
                sendNetStats(room);
            }
        } else if (now - room.lastLobbyStateMs >= 2000) {
            // Lobby: refresh everyone's ping when one has moved noticeably.
            bool changed = false;
            for (uint8_t id : room.members) {
                const Client &m = *gClients.at(id);
                const int rtt = m.peer ? static_cast<int>(m.peer->roundTripTime) : 0;
                changed |= std::abs(rtt - static_cast<int>(m.lastSentRtt)) >= 5;
            }
            if (changed) sendRoomState(room);
            else room.lastLobbyStateMs = now;
        }
    }
}

int allocateId() {
    for (int id = 1; id < 255; id++) {
        if (!gClients.count(static_cast<uint8_t>(id))) return id;
    }
    return -1;
}

} // namespace

int main(int argc, char **argv) {
    uint16_t port = SMB_NET_DEFAULT_PORT;
    for (int i = 1; i < argc; i++) {
        if (std::strcmp(argv[i], "--port") == 0 && i + 1 < argc) {
            port = static_cast<uint16_t>(std::atoi(argv[++i]));
        } else {
            std::fprintf(stderr, "usage: %s [--port N]   (default %d)\n", argv[0], SMB_NET_DEFAULT_PORT);
            return 2;
        }
    }
    if (const char *env = std::getenv("SMB_SERVER_PORT")) port = static_cast<uint16_t>(std::atoi(env));

    if (enet_initialize() != 0) {
        std::fprintf(stderr, "enet_initialize failed\n");
        return 1;
    }
    ENetAddress address{};
    address.host = ENET_HOST_ANY;
    address.port = port;
    ENetHost *host = enet_host_create(&address, kMaxClients, SMB_NET_CHANNEL_COUNT, 0, 0);
    if (!host) {
        std::fprintf(stderr, "could not listen on UDP port %u\n", port);
        return 1;
    }
    logf("SMB2 relay server, protocol %d, listening on UDP %u", SMB_NET_PROTOCOL_VERSION, port);

    for (;;) {
        ENetEvent event;
        tick();
        while (enet_host_service(host, &event, 50) > 0) {
            tick();
            switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT: {
                const int id = allocateId();
                if (id < 0) {
                    enet_peer_disconnect_now(event.peer, 0);
                    break;
                }
                auto client = std::make_unique<Client>();
                client->id = static_cast<uint8_t>(id);
                client->peer = event.peer;
                event.peer->data = client.get();
                enet_peer_timeout(event.peer, 0, 5000, 15000);
                char ip[64];
                enet_address_get_host_ip(&event.peer->address, ip, sizeof(ip));
                logf("client %d: connected from %s", id, ip);
                gClients[client->id] = std::move(client);
                break;
            }
            case ENET_EVENT_TYPE_RECEIVE: {
                if (auto *c = static_cast<Client *>(event.peer->data)) onReceive(*c, event.packet);
                enet_packet_destroy(event.packet);
                break;
            }
            case ENET_EVENT_TYPE_DISCONNECT: {
                if (auto *c = static_cast<Client *>(event.peer->data)) {
                    if (c->relaunching && c->room && c->room->inGame) {
                        // E8: it is restarting for the session; keep the seat until RESUME or the grace runs out
                        c->away = true;
                        c->peer = nullptr;
                        c->awayUntilMs = enet_time_get() + SMB_NET_RESUME_GRACE_MS;
                        logf("client %u: disconnected (restarting, seat held)", c->id);
                        event.peer->data = nullptr;
                        break;
                    }
                    logf("client %u: disconnected", c->id);
                    leaveRoom(*c);
                    gClients.erase(c->id);
                }
                event.peer->data = nullptr;
                break;
            }
            default:
                break;
            }
        }
    }
}
