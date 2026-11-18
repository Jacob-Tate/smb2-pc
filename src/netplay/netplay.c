/* Netplay client (E6): ENet lockstep input relay.  See netplay.h and docs/handoff/E6.md.
 * Modelled on partyboard's src/port/netplay.c (lobby state machine, ring of frames,
 * blocking pad read that pumps the network).  All state is on the main thread:
 * the SDL event watch only runs there too (checked), so no locking. */
#include "netplay/netplay.h"
#include "netplay/netplay_active.h"
#include "netplay/inputsync.h"
#include "netplay/checksum.h"
#include "netplay/protocol.h"


#include <enet/enet.h>
#include <SDL3/SDL.h>
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
extern volatile long g_shim_frame_pulse;   /* stubs_manual.c hang watchdog */
#define BUMP_PULSE() (g_shim_frame_pulse++)
#else
#define BUMP_PULSE() ((void)0)
#endif

#define RING 128                    /* >= max delay (30) + 1, power of two */
#define WAIT_TIMEOUT_MS 30000       /* no FRAME for this long = give up */
#define WAIT_MSG_MS 1000            /* "waiting for <name>" log/title period */

typedef struct NetFrame {
    int valid;
    uint32_t poll;
    uint8_t mask, flags;
    SmbNetPad pads[SMB_NET_MAX_PLAYERS];
} NetFrame;

static NetplayOptions s_opt;
static int s_requested;
static char s_host_name[256];
static uint16_t s_server_port = SMB_NET_DEFAULT_PORT;
static char s_name[SMB_NET_MAX_NAME + 1];

static int s_enet_ready;
static ENetHost *s_enet;
static ENetPeer *s_peer;
static int s_atexit_set;

static int s_connected, s_welcomed, s_disconnected;
static uint8_t s_client_id = SMB_NET_CLIENT_ID_NONE;
static SmbNetSessionConfig s_hello_cfg;       /* build id / disc / locale we announced */

/* room (lobby) */
static char s_room_code[SMB_NET_ROOM_CODE_LEN + 1];
static uint8_t s_room_host = SMB_NET_CLIENT_ID_NONE;
static uint8_t s_room_count;
static uint8_t s_room_ids[SMB_NET_MAX_PLAYERS];
static char s_room_names[SMB_NET_MAX_PLAYERS][SMB_NET_MAX_NAME + 1];
static uint16_t s_room_pings[SMB_NET_MAX_PLAYERS];
static uint8_t s_slot_owner[SMB_NET_MAX_PLAYERS];
static uint8_t s_room_phase;
static int s_got_room;
static char s_error[256];
static int s_got_error;

/* game */
static int s_game_started;          /* GAME_START received */
static int s_active;                /* lockstep running (netplay_begin) */
static SmbNetSessionConfig s_game_cfg;
static uint8_t s_port_owner[SMB_NET_MAX_PLAYERS];
static char s_port_names[SMB_NET_MAX_PLAYERS][SMB_NET_MAX_NAME + 1];
static uint8_t s_game_seq;
static uint8_t s_owned_mask;
static uint8_t s_human_mask = 0xFF;      /* 0xFF = no frame seen yet */
static NetFrame s_ring[RING];
static uint32_t s_next_poll;        /* next poll the game will consume */
static int s_primed;
static uint32_t s_last_cs_sent;
static uint8_t s_wait_ports;        /* from WAITING */
static uint32_t s_wait_ms;
static int s_ended;                 /* GAME_ENDED / lost connection: set by the handlers */
static int s_end_reason;            /* SmbNetEndReason, or 0 = connection lost */
static uint8_t s_end_client = SMB_NET_CLIENT_ID_NONE;
static int s_desync;
static uint32_t s_desync_poll;

/* PERF accounting (reported every 60 polls) */
static uint32_t s_perf_last_ms, s_perf_polls, s_perf_slow, s_perf_stalls, s_perf_worst;

static int s_in_service;
static SDL_ThreadID s_main_tid;
static uint32_t s_watch_last_ms;
static int s_watch_added;

/* ---------------------------------------------------------------- send */

static void send_buf(int channel, const uint8_t *data, size_t n)
{
    ENetPacket *packet;
    if (!s_peer) return;
    packet = enet_packet_create(data, n, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(s_peer, (enet_uint8)channel, packet);
    enet_host_flush(s_enet);
}

static void send_input(uint32_t poll, const SmbNetPad local[INPUTSYNC_PORTS])
{
    uint8_t buf[8 + SMB_NET_MAX_PLAYERS * SMB_NET_PAD_SIZE];
    SmbNetWriter w;
    int port, k = 0;
    smbw_init(&w, buf, sizeof buf);
    smbw_u8(&w, SMB_MSG_INPUT);
    smbw_u32(&w, poll);
    smbw_u8(&w, s_game_seq);
    for (port = 0; port < SMB_NET_MAX_PLAYERS; port++)
        if (s_owned_mask & (1 << port))
            smbw_pad(&w, &local[k++]);   /* first owned port takes local pad 0, ... */
    send_buf(SMB_NET_CHANNEL_INPUT, buf, w.pos);
}

/* ------------------------------------------------------------- receive */

static const char *client_name(uint8_t id)
{
    int i;
    for (i = 0; i < s_room_count; i++)
        if (s_room_ids[i] == id) return s_room_names[i];
    return "?";
}

static void handle_frame(SmbNetReader *r)
{
    const uint32_t poll = smbr_u32(r);
    const uint8_t seq = smbr_u8(r);
    const uint8_t mask = smbr_u8(r);
    const uint8_t flags = smbr_u8(r);
    NetFrame *slot;
    int port;
    /* stale frames of an earlier game can arrive after GAME_START (E5 notes) */
    if (r->error || !s_active || seq != s_game_seq || poll < s_next_poll || poll >= s_next_poll + RING)
        return;
    slot = &s_ring[poll % RING];
    memset(slot, 0, sizeof *slot);
    slot->mask = mask;
    slot->flags = flags;
    for (port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if (mask & (1 << port)) smbr_pad(r, &slot->pads[port]);
    }
    if (!r->error) {
        slot->poll = poll;
        slot->valid = 1;
        s_wait_ports = 0;
    }
}

static void handle_room_state(SmbNetReader *r)
{
    int i;
    char code[SMB_NET_ROOM_CODE_LEN];
    smbr_bytes(r, code, sizeof code);
    s_room_host = smbr_u8(r);
    s_room_count = smbr_u8(r);
    if (r->error || s_room_count > SMB_NET_MAX_PLAYERS) return;
    for (i = 0; i < s_room_count; i++) {
        s_room_ids[i] = smbr_u8(r);
        smbr_str(r, s_room_names[i], sizeof s_room_names[i]);
        s_room_pings[i] = smbr_u16(r);
    }
    for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) s_slot_owner[i] = smbr_u8(r);
    s_room_phase = smbr_u8(r);
    if (r->error) return;
    memcpy(s_room_code, code, sizeof code);
    s_room_code[SMB_NET_ROOM_CODE_LEN] = 0;
    s_got_room = 1;
}

static void handle_packet(const uint8_t *data, size_t size)
{
    SmbNetReader r;
    uint8_t type;
    smbr_init(&r, data, size);
    type = smbr_u8(&r);
    switch (type) {
    case SMB_MSG_WELCOME:
        s_client_id = smbr_u8(&r);
        if (!r.error) s_welcomed = 1;
        break;
    case SMB_MSG_ERROR: {
        uint8_t code = smbr_u8(&r);
        char msg[200];
        smbr_str(&r, msg, sizeof msg);
        snprintf(s_error, sizeof s_error, "server error %u: %s", (unsigned)code, r.error ? "?" : msg);
        s_got_error = 1;
        printf("[netplay] %s\n", s_error);
        fflush(stdout);
        break;
    }
    case SMB_MSG_ROOM_STATE:
        handle_room_state(&r);
        break;
    case SMB_MSG_GAME_START: {
        int i;
        smbr_config(&r, &s_game_cfg);
        for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) s_port_owner[i] = smbr_u8(&r);
        s_game_seq = smbr_u8(&r);
        if (r.error) break;
        s_owned_mask = 0;
        for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) {
            if (s_port_owner[i] == s_client_id) s_owned_mask |= (uint8_t)(1 << i);
            snprintf(s_port_names[i], sizeof s_port_names[i], "%s",
                     s_port_owner[i] == SMB_NET_PORT_NONE ? "" : client_name(s_port_owner[i]));
        }
        s_game_started = 1;
        break;
    }
    case SMB_MSG_FRAME:
        handle_frame(&r);
        break;
    case SMB_MSG_DESYNC:
        s_desync_poll = smbr_u32(&r);
        s_desync = 1;
        printf("[netplay] DESYNC reported by the server at poll %u\n", (unsigned)s_desync_poll);
        fflush(stdout);
        break;
    case SMB_MSG_GAME_ENDED:
        if (s_active) {
            s_end_reason = smbr_u8(&r);
            s_end_client = smbr_u8(&r);
            s_ended = 1;
        }
        break;
    case SMB_MSG_PLAYER_DROPPED: {
        uint8_t id = smbr_u8(&r);
        uint8_t ports = smbr_u8(&r);
        uint32_t poll = smbr_u32(&r);
        if (!r.error) {
            printf("[netplay] player '%s' dropped (ports %02x) from poll %u\n", client_name(id), ports, (unsigned)poll);
            fflush(stdout);
        }
        break;
    }
    case SMB_MSG_WAITING:
        s_wait_ports = smbr_u8(&r);
        s_wait_ms = smbr_u16(&r);
        break;
    case SMB_MSG_NET_STATS:
    default:
        break;
    }
}

/* One service pass.  Returns 0 once the connection is gone. */
static int net_service(int timeout_ms)
{
    ENetEvent ev;
    if (!s_enet) return 0;
    if (s_in_service) return 1;
    s_in_service = 1;
    while (enet_host_service(s_enet, &ev, (enet_uint32)timeout_ms) > 0) {
        if (ev.type == ENET_EVENT_TYPE_CONNECT) {
            s_connected = 1;
        } else if (ev.type == ENET_EVENT_TYPE_RECEIVE) {
            handle_packet(ev.packet->data, ev.packet->dataLength);
            enet_packet_destroy(ev.packet);
        } else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) {
            s_disconnected = 1;
            s_peer = NULL;
        }
        timeout_ms = 0;
    }
    s_in_service = 0;
    return !s_disconnected;
}

/* --------------------------------------------------------------- setup */

void netplay_set_options(const NetplayOptions *opt)
{
    s_opt = *opt;
    s_requested = opt->server && opt->server[0];
}

int netplay_requested(void) { return s_requested; }
/* --overlay-test-online (D3): pretend a session runs so the overlay acts online, no network. */
static int s_test_forced;
void netplay_test_force_active(int on) { s_test_forced = on; }
int netplay_active(void) { return s_active || s_test_forced; }
int netplay_input_delay(void) { return s_active ? s_game_cfg.inputDelay : 0; }

void netplay_shutdown(void)
{
    int i;
    ENetEvent ev;
    if (s_peer && s_enet) {
        enet_peer_disconnect(s_peer, 0);
        for (i = 0; i < 20 && !s_disconnected; i++) {
            if (enet_host_service(s_enet, &ev, 25) > 0) {
                if (ev.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(ev.packet);
                else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) break;
            }
        }
        enet_peer_reset(s_peer);
        s_peer = NULL;
    }
    if (s_enet) {
        enet_host_destroy(s_enet);
        s_enet = NULL;
    }
    if (s_enet_ready) {
        enet_deinitialize();
        s_enet_ready = 0;
    }
    s_active = 0;
}

static void parse_server(const char *server)
{
    const char *colon = strrchr(server, ':');
    size_t n = colon ? (size_t)(colon - server) : strlen(server);
    if (n >= sizeof s_host_name) n = sizeof s_host_name - 1;
    memcpy(s_host_name, server, n);
    s_host_name[n] = 0;
    s_server_port = SMB_NET_DEFAULT_PORT;
    if (colon && atoi(colon + 1) > 0 && atoi(colon + 1) < 65536) s_server_port = (uint16_t)atoi(colon + 1);
}

static int lobby_fail(const char *why)
{
    fprintf(stderr, "[netplay] %s\n", why);
    fflush(stderr);
    netplay_shutdown();
    return 0;
}

int netplay_lobby_run(SessionConfig *out)
{
    ENetAddress address;
    uint8_t buf[256];
    SmbNetWriter w;
    uint32_t t0, now;
    enum { CONNECTING, HELLO, JOINING, IN_ROOM } st = CONNECTING;
    int sent_start = 0, delay = s_opt.delay >= 0 && s_opt.delay <= SMB_NET_MAX_INPUT_DELAY ? s_opt.delay : 2;
    int players = s_opt.players > 0 ? s_opt.players : 2;
    int timeout_s = s_opt.lobby_timeout_s > 0 ? s_opt.lobby_timeout_s : 120;
    char code[SMB_NET_ROOM_CODE_LEN + 1];
    SessionConfig dc;
    int k;

    if (!s_opt.server || !s_opt.server[0]) return lobby_fail("--server <host[:port]> is required");
    if (!s_opt.create) {
        if (!s_opt.join_code || strlen(s_opt.join_code) != SMB_NET_ROOM_CODE_LEN)
            return lobby_fail("--join needs a 4-letter room code (or use --create)");
        for (k = 0; k < SMB_NET_ROOM_CODE_LEN; k++) {
            char ch = s_opt.join_code[k];
            code[k] = (char)((ch >= 'a' && ch <= 'z') ? ch - 32 : ch);
        }
        code[SMB_NET_ROOM_CODE_LEN] = 0;
    }
    parse_server(s_opt.server);
    snprintf(s_name, sizeof s_name, "%s", s_opt.name && s_opt.name[0] ? s_opt.name : "Player");

    /* what we announce: the same defaults the session uses */
    pc_session_default_config(&dc);
    memset(&s_hello_cfg, 0, sizeof s_hello_cfg);
    snprintf(s_hello_cfg.buildId, sizeof s_hello_cfg.buildId, "%s", dc.build_id);
    s_hello_cfg.discHash = dc.disc_hash;
    s_hello_cfg.locale = dc.locale;

    if (enet_initialize() != 0) return lobby_fail("ENet init failed");
    s_enet_ready = 1;
    if (!s_atexit_set) { atexit(netplay_shutdown); s_atexit_set = 1; }
    s_enet = enet_host_create(NULL, 1, SMB_NET_CHANNEL_COUNT, 0, 0);
    if (!s_enet || enet_address_set_host(&address, s_host_name) != 0) {
        char why[300];
        snprintf(why, sizeof why, "can't find server '%s'", s_host_name);
        return lobby_fail(why);
    }
    address.port = s_server_port;
    printf("[netplay] connecting to %s:%u as '%s' (%s)\n", s_host_name, s_server_port, s_name,
           s_opt.create ? "creating a room" : "joining a room");
    fflush(stdout);
    s_peer = enet_host_connect(s_enet, &address, SMB_NET_CHANNEL_COUNT, 0);
    if (!s_peer) return lobby_fail("couldn't start connecting");
    t0 = enet_time_get();

    for (;;) {
        net_service(20);
        now = enet_time_get();
        if (s_disconnected) return lobby_fail("disconnected from the server");
        if (s_got_error) return lobby_fail(s_error);
        if (now - t0 > (uint32_t)timeout_s * 1000u) {
            return lobby_fail(st == CONNECTING ? "no answer from the server (is UDP open?)"
                                               : "timed out in the lobby");
        }
        switch (st) {
        case CONNECTING:
            if (s_connected) {
                enet_peer_timeout(s_peer, 0, 10000, 30000);
                smbw_init(&w, buf, sizeof buf);
                smbw_u8(&w, SMB_MSG_HELLO);
                smbw_u16(&w, SMB_NET_PROTOCOL_VERSION);
                smbw_str(&w, s_name, SMB_NET_MAX_NAME);
                smbw_str(&w, s_hello_cfg.buildId, SMB_NET_MAX_BUILD_ID);
                smbw_u32(&w, s_hello_cfg.discHash);
                smbw_u8(&w, s_hello_cfg.locale);
                send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
                st = HELLO;
            } else if (now - t0 > 5000) {
                char why[300];
                snprintf(why, sizeof why, "no answer from %s:%u (is UDP port %u open?)", s_host_name,
                         s_server_port, s_server_port);
                return lobby_fail(why);
            }
            break;
        case HELLO:
            if (s_welcomed) {
                smbw_init(&w, buf, sizeof buf);
                if (s_opt.create) {
                    smbw_u8(&w, SMB_MSG_CREATE_ROOM);
                } else {
                    smbw_u8(&w, SMB_MSG_JOIN_ROOM);
                    smbw_bytes(&w, code, SMB_NET_ROOM_CODE_LEN);
                }
                send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
                st = JOINING;
            }
            break;
        case JOINING:
            if (s_got_room) {
                st = IN_ROOM;
                printf("[netplay] in room %s as client %u (host: client %u)\n", s_room_code,
                       (unsigned)s_client_id, (unsigned)s_room_host);
                if (s_opt.create && s_opt.code_file && s_opt.code_file[0]) {
                    FILE *f = fopen(s_opt.code_file, "wb");
                    if (f) { fprintf(f, "%s\n", s_room_code); fclose(f); }
                    else fprintf(stderr, "[netplay] cannot write %s\n", s_opt.code_file);
                }
                fflush(stdout);
            }
            break;
        case IN_ROOM:
            if (s_opt.create && s_room_host == s_client_id && !sent_start && s_room_count >= players &&
                s_room_phase == SMB_ROOM_LOBBY) {
                SmbNetSessionConfig cfg = s_hello_cfg;
                int i;
                cfg.epoch = 0;
                cfg.inputDelay = (uint8_t)delay;
                cfg.flags = 0;
                smbw_init(&w, buf, sizeof buf);
                smbw_u8(&w, SMB_MSG_START_GAME);
                smbw_config(&w, &cfg);
                send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
                sent_start = 1;
                printf("[netplay] starting the game with %d players, input delay %d:", s_room_count, delay);
                for (i = 0; i < s_room_count; i++) printf(" %s", s_room_names[i]);
                printf("\n");
                fflush(stdout);
            }
            break;
        }
        if (s_game_started) break;
    }

    /* safety net: the server already checks this at JOIN/START */
    {
        const char *bad = smb_config_mismatch(&s_game_cfg, &s_hello_cfg);
        if (bad) {
            char why[200];
            snprintf(why, sizeof why, "the session's %s doesn't match ours", bad);
            return lobby_fail(why);
        }
    }
    memset(out, 0, sizeof *out);
    snprintf(out->build_id, sizeof out->build_id, "%.*s", SMB_NET_MAX_BUILD_ID - 1, s_game_cfg.buildId);
    out->disc_hash = s_game_cfg.discHash;
    out->locale = s_game_cfg.locale;
    out->epoch = s_game_cfg.epoch;
    out->flags = s_game_cfg.flags;
    {
        int i;
        printf("[netplay] GAME_START: seq %u, epoch %016llx, delay %u, ports:", (unsigned)s_game_seq,
               (unsigned long long)s_game_cfg.epoch, (unsigned)s_game_cfg.inputDelay);
        for (i = 0; i < SMB_NET_MAX_PLAYERS; i++)
            printf(" P%d=%s", i + 1, s_port_owner[i] == SMB_NET_PORT_NONE ? "-" : s_port_names[i]);
        printf(" (we own mask %x)\n", s_owned_mask);
        fflush(stdout);
    }
    return 1;
}

/* ------------------------------------------------------------ lockstep */

static void die(int code, const char *why)
{
    printf("[netplay] %s\n", why);
    fflush(NULL);
    netplay_shutdown();
    exit(code);
}

/* Keeps the window and network alive while the main thread sits inside the
 * OS's modal move/size loop (no frames run then). */
static bool SDLCALL net_event_watch(void *user, SDL_Event *event)
{
    uint32_t now;
    (void)user; (void)event;
    if (!s_active || s_in_service || SDL_GetCurrentThreadID() != s_main_tid) return true;
    now = enet_time_get();
    if (now - s_watch_last_ms < 5) return true;
    s_watch_last_ms = now;
    net_service(0);
    BUMP_PULSE();
    return true;
}

static void send_perf(uint32_t poll)
{
    uint8_t buf[16];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof buf);
    smbw_u8(&w, SMB_MSG_PERF);
    smbw_u32(&w, poll);
    smbw_u16(&w, (uint16_t)s_perf_polls);
    smbw_u16(&w, (uint16_t)s_perf_slow);
    smbw_u16(&w, (uint16_t)s_perf_stalls);
    smbw_u16(&w, (uint16_t)(s_perf_worst > 65535 ? 65535 : s_perf_worst));
    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
    s_perf_polls = s_perf_slow = s_perf_stalls = s_perf_worst = 0;
}

static void set_wait_title(const char *text)
{
    SDL_Window *win = (SDL_Window *)platform_sdl_window();
    static char original[256];
    static int saved;
    if (!win) return;
    if (text) {
        if (!saved) { snprintf(original, sizeof original, "%s", SDL_GetWindowTitle(win)); saved = 1; }
        SDL_SetWindowTitle(win, text);
    } else if (saved) {
        SDL_SetWindowTitle(win, original);
        saved = 0;
    }
}

static void describe_waiting(char *dst, size_t cap, uint32_t waited_ms)
{
    char who[160] = "";
    int port;
    for (port = 0; port < SMB_NET_MAX_PLAYERS; port++) {
        if ((s_wait_ports & (1 << port)) && s_port_owner[port] != SMB_NET_PORT_NONE) {
            size_t n = strlen(who);
            snprintf(who + n, sizeof who - n, "%s%s", n ? ", " : "", s_port_names[port]);
        }
    }
    snprintf(dst, cap, "waiting for %s (%u.%us)", who[0] ? who : "the other players", (unsigned)(waited_ms / 1000),
             (unsigned)(waited_ms % 1000 / 100));
}

static void check_end(void)
{
    if (s_ended) {
        const char *why = s_end_reason == SMB_END_DESYNC ? "the game ended: DESYNC"
                        : s_end_reason == SMB_END_HOST ? "the game ended: the host ended it"
                        : s_end_reason == SMB_END_PLAYER_LEFT ? "the game ended: everyone else left"
                                                              : "the game ended";
        die(s_end_reason == SMB_END_DESYNC ? 4 : 0, why);
    }
    if (s_disconnected) die(3, "lost the connection to the server");
}

static int net_exchange(uint32_t poll, const SmbNetPad local[INPUTSYNC_PORTS], SmbNetPad out[INPUTSYNC_PORTS],
                        void *user)
{
    NetFrame *slot;
    uint32_t t_start, waited = 0, last_msg = 0;
    int port, stalled = 0;
    (void)user;

    if (!s_primed) {
        /* the server expects every human port to supply polls 0..D-1: neutral */
        SmbNetPad neutral[INPUTSYNC_PORTS];
        uint32_t f;
        memset(neutral, 0, sizeof neutral);
        for (f = 0; f < s_game_cfg.inputDelay; f++) send_input(f, neutral);
        s_primed = 1;
        s_perf_last_ms = enet_time_get();
    }
    s_next_poll = poll;

    {   /* checksum E3 computed for the poll just completed (every 60) */
        uint32_t h, cp = checksum_latest(&h);
        if (cp > s_last_cs_sent) {
            uint8_t b[16];
            SmbNetWriter w;
            smbw_init(&w, b, sizeof b);
            smbw_u8(&w, SMB_MSG_CHECKSUM);
            smbw_u32(&w, cp);
            smbw_u32(&w, h);
            send_buf(SMB_NET_CHANNEL_CONTROL, b, w.pos);
            s_last_cs_sent = cp;
        }
    }
    send_input(poll + s_game_cfg.inputDelay, local);

    net_service(0);
    check_end();
    slot = &s_ring[poll % RING];
    t_start = enet_time_get();
    while (!(slot->valid && slot->poll == poll)) {
        uint32_t now;
        stalled = 1;
        net_service(1);
        check_end();
        platform_poll_events();
        BUMP_PULSE();
        if (platform_should_close()) {
            printf("[netplay] window closed, leaving\n");
            return 1;
        }
        now = enet_time_get();
        waited = now - t_start;
        if (waited >= WAIT_MSG_MS && waited - last_msg >= WAIT_MSG_MS) {
            char msg[256], title[300];
            last_msg = waited;
            describe_waiting(msg, sizeof msg, waited);
            printf("[netplay] poll %u: %s\n", (unsigned)poll, msg);
            fflush(stdout);
            snprintf(title, sizeof title, "SMB2 - %s", msg);
            set_wait_title(title);
        }
        if (waited > WAIT_TIMEOUT_MS) die(3, "timed out waiting for the other players");
    }
    if (last_msg) set_wait_title(NULL);

    if (slot->mask != s_human_mask) {
        /* E9: a player joined/dropped; the mask is per poll and identical everywhere */
        if (s_human_mask != 0xFF) {
            printf("[netplay] poll %u: human mask %02x -> %02x\n", (unsigned)poll, (unsigned)s_human_mask,
                   (unsigned)slot->mask);
            fflush(stdout);
        }
        s_human_mask = slot->mask;
    }
    for (port = 0; port < INPUTSYNC_PORTS; port++) {
        if (slot->mask & (1 << port)) {
            out[port] = slot->pads[port];
        } else {
            smb_pad_neutral(&out[port]);
            out[port].err = -1;   /* no controller on this port (same on every peer) */
        }
    }
    slot->valid = 0;

    /* perf */
    {
        uint32_t now = enet_time_get(), dt = now - s_perf_last_ms;
        s_perf_last_ms = now;
        s_perf_polls++;
        if (dt > 25) s_perf_slow++;
        if (stalled && waited > 10) s_perf_stalls++;
        if (dt > s_perf_worst) s_perf_worst = dt;
        if ((poll + 1) % 60 == 0) send_perf(poll + 1);
    }
    return 0;
}

int netplay_begin(void)
{
    if (!s_game_started || !s_peer) return 0;
    s_active = 1;
    s_main_tid = SDL_GetCurrentThreadID();
    inputsync_set_netplay(net_exchange, NULL);
    /* the watch needs SDL initialised: the event subsystem comes up with the window,
     * so add it lazily if it isn't yet (AddEventWatch itself needs no init) */
    if (!s_watch_added) {
        SDL_AddEventWatch(net_event_watch, NULL);
        s_watch_added = 1;
    }
    printf("[netplay] lockstep on: input delay %u polls, ports owned %x\n", (unsigned)s_game_cfg.inputDelay,
           s_owned_mask);
    fflush(stdout);
    return 1;
}
