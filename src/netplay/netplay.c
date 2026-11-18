/* Netplay client: ENet lockstep input relay (E6) + non-blocking in-game lobby and
 * session start by restarting the program (E8).  See netplay.h and docs/handoff/E6.md,
 * docs/handoff/E8.md.  Modelled on partyboard's src/port/netplay.c (lobby state machine,
 * ring of frames, blocking pad read that pumps the network, relaunch).  All state is on
 * the main thread: the SDL event watch only runs there too (checked), so no locking. */
#include "netplay/netplay.h"
#include "netplay/netplay_active.h"
#include "netplay/inputsync.h"
#include "netplay/checksum.h"
#include "netplay/protocol.h"


#include <enet/enet.h>
#include <SDL3/SDL.h>
#include "platform.h"
#include "iso.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
extern volatile long g_shim_frame_pulse;   /* stubs_manual.c hang watchdog */
#define BUMP_PULSE() (g_shim_frame_pulse++)
#else
#define BUMP_PULSE() ((void)0)
#endif

#ifdef SMB2_LINK_GAME
extern void pc_quit_request(const char *reason);
#else
static void pc_quit_request(const char *reason) { (void)reason; exit(0); }
#endif

#define RING 128                    /* >= max delay (30) + 1, power of two */
#define WAIT_TIMEOUT_MS 30000       /* no FRAME for this long = give up */
#define FIRST_POLL_TIMEOUT_MS 90000 /* the first polls wait for the slowest process to restart */
#define WAIT_MSG_MS 1000            /* "waiting for <name>" log/title period */
#define CONNECT_TIMEOUT_MS 8000
#define EVENT_RING 16

typedef struct NetFrame {
    int valid;
    uint32_t poll;
    uint8_t mask, flags;
    SmbNetPad pads[SMB_NET_MAX_PLAYERS];
} NetFrame;

static NetplayOptions s_opt;
static int s_requested;
static int s_argc;
static char **s_argv;
static char s_server_str[300];        /* as given: "host" or "host:port" */
static char s_host_name[256];
static uint16_t s_server_port = SMB_NET_DEFAULT_PORT;
static char s_name[SMB_NET_MAX_NAME + 1];

static int s_enet_ready;
static ENetHost *s_enet;
static ENetPeer *s_peer;
static int s_atexit_set;

static NetplayLobbyState s_state = NP_LOBBY_OFFLINE;
static int s_connected, s_welcomed, s_disconnected;
static uint32_t s_connect_ms;
static uint8_t s_client_id = SMB_NET_CLIENT_ID_NONE;
static uint8_t s_caps;                        /* from WELCOME's trailing byte (0 on servers older than E8) */
static SmbNetSessionConfig s_hello_cfg;       /* build id / disc / locale we announced */
static int s_locale;
static int s_hello_sent;
static int s_blocking;                        /* a command-line wrapper owns the lobby flow */

/* what the user asked for before the connection was ready */
static enum { WANT_NONE, WANT_CREATE, WANT_JOIN } s_want;
static char s_want_code[SMB_NET_ROOM_CODE_LEN + 1];
static int s_delay = -1;                      /* host: input delay for START_GAME, -1 = auto */

/* room (lobby) */
static char s_room_code[SMB_NET_ROOM_CODE_LEN + 1];
static uint8_t s_room_host = SMB_NET_CLIENT_ID_NONE;
static uint8_t s_room_count;
static uint8_t s_room_ids[SMB_NET_MAX_PLAYERS];
static char s_room_names[SMB_NET_MAX_PLAYERS][SMB_NET_MAX_NAME + 1];
static uint16_t s_room_pings[SMB_NET_MAX_PLAYERS];
static uint8_t s_slot_owner[SMB_NET_MAX_PLAYERS];
static uint8_t s_room_phase;
static int s_got_room;                        /* a ROOM_STATE for the current room arrived */
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

/* restart / resume (E8) */
static int s_resume_set;            /* this process was started by a restart: --np-resume given */
static char s_resume_code[SMB_NET_ROOM_CODE_LEN + 1];
static int s_resume_id;
static uint32_t s_resume_token;
static int s_resumed;               /* a resumed session is (or was) running in this process */
static int s_automated;
static const char *s_exit_file;
static int s_test_return;           /* --np-lobby-return: test, go back to the lobby even in automated runs */
static char s_return_script[512];   /* --np-return-script: lobby test script of the process that returns to the lobby */

/* E9 test hooks (environment, read once in netplay_begin; they only change what THIS client does on the wire) */
static long s_hook_drop = -1;       /* SMB_NETPLAY_TEST_DROP=<poll>[:quit|:leave]: die at that poll (hard kill by default) */
static int s_hook_drop_mode;        /* 0 = hard kill (no goodbye), 1 = quit (pc_quit_request: the pause menu's Quit to Desktop), 2 = leave (LEAVE_ROOM + exit) */
static long s_hook_desync = -1;     /* SMB_NETPLAY_TEST_DESYNC=<poll>: from the first checksum at/after it, send a wrong hash */
static long s_hook_stall = -1;      /* SMB_NETPLAY_TEST_STALL=<poll>:<ms>[:net]: sleep in the main loop at that poll */
static long s_hook_stall_ms;
static int s_hook_stall_net;        /* 1 = keep servicing ENet while stalled (connection stays up) */
static long s_hook_endgame = -1;    /* SMB_NETPLAY_TEST_ENDGAME=<poll>: the host sends END_GAME at that poll */

/* toast queue */
static NetplayEvent s_events[EVENT_RING];
static int s_ev_head, s_ev_count;

/* PERF accounting (reported every 60 polls) */
static uint32_t s_perf_last_ms, s_perf_polls, s_perf_slow, s_perf_stalls, s_perf_worst;

static int s_in_service;
static SDL_ThreadID s_main_tid;
static uint32_t s_watch_last_ms;
static int s_watch_added;

static void lobby_close(void);
static void session_end(int code, const char *why, int room_alive);

/* ---------------------------------------------------------------- events */

void netplay_push_notice(int kind, const char *text)
{
    NetplayEvent *e;
    if (!text || !text[0]) return;
    if (s_ev_count == EVENT_RING) {          /* drop the oldest */
        s_ev_head = (s_ev_head + 1) % EVENT_RING;
        s_ev_count--;
    }
    e = &s_events[(s_ev_head + s_ev_count) % EVENT_RING];
    s_ev_count++;
    e->kind = kind;
    snprintf(e->text, sizeof e->text, "%s", text);
    printf("[netplay] %s%s\n", kind == NP_EV_ERROR ? "ERROR: " : kind == NP_EV_WARN ? "warning: " : "", e->text);
    fflush(stdout);
}

int netplay_lobby_next_event(NetplayEvent *out)
{
    if (!s_ev_count) return 0;
    *out = s_events[s_ev_head];
    s_ev_head = (s_ev_head + 1) % EVENT_RING;
    s_ev_count--;
    return 1;
}

/* ---------------------------------------------------------------- send */

static void send_buf(int channel, const uint8_t *data, size_t n)
{
    ENetPacket *packet;
    if (!s_peer) return;
    packet = enet_packet_create(data, n, ENET_PACKET_FLAG_RELIABLE);
    enet_peer_send(s_peer, (enet_uint8)channel, packet);
    enet_host_flush(s_enet);
}

static void send_simple(uint8_t type)
{
    send_buf(SMB_NET_CHANNEL_CONTROL, &type, 1);
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
    int i, j;
    char code[SMB_NET_ROOM_CODE_LEN];
    uint8_t host, count, ids[SMB_NET_MAX_PLAYERS], owner[SMB_NET_MAX_PLAYERS], phase;
    char names[SMB_NET_MAX_PLAYERS][SMB_NET_MAX_NAME + 1];
    uint16_t pings[SMB_NET_MAX_PLAYERS];
    smbr_bytes(r, code, sizeof code);
    host = smbr_u8(r);
    count = smbr_u8(r);
    if (r->error || count > SMB_NET_MAX_PLAYERS) return;
    for (i = 0; i < count; i++) {
        ids[i] = smbr_u8(r);
        smbr_str(r, names[i], sizeof names[i]);
        pings[i] = smbr_u16(r);
    }
    for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) owner[i] = smbr_u8(r);
    phase = smbr_u8(r);
    if (r->error) return;
    /* a ROOM_STATE that trails behind a LEAVE_ROOM / disconnect is not ours any more */
    if (s_state == NP_LOBBY_READY || s_state == NP_LOBBY_CONNECTING || s_state == NP_LOBBY_OFFLINE) return;

    if (s_got_room && s_state == NP_LOBBY_IN_ROOM) {   /* toasts for joins and leaves */
        for (i = 0; i < count; i++) {
            int known = 0;
            for (j = 0; j < s_room_count; j++) known |= s_room_ids[j] == ids[i];
            if (!known && ids[i] != s_client_id) {
                char t[96];
                snprintf(t, sizeof t, "%s joined the room", names[i]);
                netplay_push_notice(NP_EV_INFO, t);
            }
        }
        for (j = 0; j < s_room_count; j++) {
            int still = 0;
            for (i = 0; i < count; i++) still |= s_room_ids[j] == ids[i];
            if (!still) {
                char t[96];
                snprintf(t, sizeof t, "%s left the room", s_room_names[j]);
                netplay_push_notice(NP_EV_INFO, t);
            }
        }
    }
    s_room_host = host;
    s_room_count = count;
    for (i = 0; i < count; i++) {
        s_room_ids[i] = ids[i];
        memcpy(s_room_names[i], names[i], sizeof s_room_names[i]);
        s_room_pings[i] = pings[i];
    }
    memcpy(s_slot_owner, owner, sizeof s_slot_owner);
    s_room_phase = phase;
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
        if (!r.error) {
            s_caps = r.pos < r.size ? smbr_u8(&r) : 0;
            s_welcomed = 1;
        }
        break;
    case SMB_MSG_ERROR: {
        uint8_t code = smbr_u8(&r);
        char msg[200];
        smbr_str(&r, msg, sizeof msg);
        snprintf(s_error, sizeof s_error, "server error %u: %s", (unsigned)code, r.error ? "?" : msg);
        s_got_error = 1;
        netplay_push_notice(NP_EV_ERROR, r.error ? s_error : msg);
        break;
    }
    case SMB_MSG_ROOM_STATE:
        handle_room_state(&r);
        break;
    case SMB_MSG_GAME_START: {
        int i;
        SmbNetSessionConfig cfg;
        uint8_t owner[SMB_NET_MAX_PLAYERS], seq;
        smbr_config(&r, &cfg);
        for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) owner[i] = smbr_u8(&r);
        seq = smbr_u8(&r);
        if (r.error || s_active) break;
        s_game_cfg = cfg;
        memcpy(s_port_owner, owner, sizeof owner);
        s_game_seq = seq;
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
            char who[SMB_NET_MAX_NAME + 8] = "", t[96];
            int port;
            for (port = 0; port < SMB_NET_MAX_PLAYERS; port++)
                if ((ports & (1 << port)) && s_port_names[port][0]) {
                    snprintf(who, sizeof who, "%s", s_port_names[port]);
                    break;
                }
            if (!who[0]) snprintf(who, sizeof who, "%s", client_name(id));
            printf("[netplay] player '%s' dropped (ports %02x) from poll %u\n", who, ports, (unsigned)poll);
            fflush(stdout);
            snprintf(t, sizeof t, "%s left the game", who);
            netplay_push_notice(NP_EV_INFO, t);
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

/* ----------------------------------------------------- options / globals */

void netplay_set_options(const NetplayOptions *opt)
{
    s_opt = *opt;
    s_requested = opt->server && opt->server[0];
}

int netplay_requested(void) { return s_requested; }
void netplay_set_argv(int argc, char **argv) { s_argc = argc; s_argv = argv; }
void netplay_set_automated(int on) { s_automated = on; }
void netplay_set_exit_file(const char *path) { s_exit_file = path; }
void netplay_lobby_set_locale(int locale) { s_locale = locale >= 0 && locale <= 5 ? locale : 0; }
int netplay_is_resume(void) { return s_resume_set; }

/* --np-resume CODE,ID,TOKEN */
int netplay_set_resume(const char *spec)
{
    unsigned id = 0, token = 0;
    if (!spec || strlen(spec) < SMB_NET_ROOM_CODE_LEN + 1 || spec[SMB_NET_ROOM_CODE_LEN] != ',' ||
        sscanf(spec + SMB_NET_ROOM_CODE_LEN + 1, "%u,%x", &id, &token) != 2 || id > 254)
        return 0;
    memcpy(s_resume_code, spec, SMB_NET_ROOM_CODE_LEN);
    s_resume_code[SMB_NET_ROOM_CODE_LEN] = 0;
    s_resume_id = (int)id;
    s_resume_token = token;
    s_resume_set = 1;
    return 1;
}

/* --overlay-test-online (D3): pretend a session runs so the overlay acts online, no network. */
static int s_test_forced;
void netplay_test_force_active(int on) { s_test_forced = on; }
int netplay_active(void) { return s_active || s_test_forced; }
int netplay_input_delay(void) { return s_active ? s_game_cfg.inputDelay : 0; }

static void write_exit_file(void)
{
    FILE *f;
    if (!s_exit_file || !s_resumed) return;
    f = fopen(s_exit_file, "wb");
    if (f) { fprintf(f, "exited\n"); fclose(f); }
}

/* Drops the connection at once (no goodbye) and forgets the room. */
static void lobby_close(void)
{
    if (s_peer && s_enet) {
        enet_peer_reset(s_peer);
        s_peer = NULL;
    }
    if (s_enet) {
        enet_host_destroy(s_enet);
        s_enet = NULL;
    }
    s_connected = s_welcomed = s_disconnected = 0;
    s_got_room = 0;
    s_room_count = 0;
    s_room_phase = SMB_ROOM_LOBBY;
    memset(s_slot_owner, SMB_NET_PORT_NONE, sizeof s_slot_owner);
    s_room_code[0] = 0;
    s_want = WANT_NONE;
    s_got_error = 0;
    s_state = NP_LOBBY_OFFLINE;
}

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
    if (s_state != NP_LOBBY_STARTING) s_state = NP_LOBBY_OFFLINE;
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

/* ------------------------------------------------------------ SDL watch */

/* Keeps the window and network alive while the main thread sits inside the
 * OS's modal move/size loop (no frames run then). */
static bool SDLCALL net_event_watch(void *user, SDL_Event *event)
{
    uint32_t now;
    (void)user; (void)event;
    if (!s_enet || s_in_service || SDL_GetCurrentThreadID() != s_main_tid) return true;
    now = enet_time_get();
    if (now - s_watch_last_ms < 5) return true;
    s_watch_last_ms = now;
    net_service(0);
    BUMP_PULSE();
    return true;
}

/* ---------------------------------------------------------- lobby API */

static uint32_t disc_hash(void)
{
    return iso_fst_crc32(platform_get_iso_archive());
}

int netplay_lobby_connect(const char *server, const char *name)
{
    ENetAddress address;
    SessionConfig dc;
    if (!server || !server[0]) { netplay_push_notice(NP_EV_ERROR, "Enter a server address"); return 0; }
    if (s_active) return 0;
    if (s_enet) { netplay_lobby_disconnect(); }
    snprintf(s_server_str, sizeof s_server_str, "%s", server);
    parse_server(server);
    snprintf(s_name, sizeof s_name, "%s", name && name[0] ? name : "Player");

    /* what we announce: the same defaults the session uses, with the real disc identity */
    pc_session_default_config(&dc);
    memset(&s_hello_cfg, 0, sizeof s_hello_cfg);
    snprintf(s_hello_cfg.buildId, sizeof s_hello_cfg.buildId, "%s", dc.build_id);
    s_hello_cfg.discHash = disc_hash();
    s_hello_cfg.locale = (uint8_t)s_locale;

    if (!s_enet_ready) {
        if (enet_initialize() != 0) { netplay_push_notice(NP_EV_ERROR, "Network init failed"); return 0; }
        s_enet_ready = 1;
    }
    if (!s_atexit_set) { atexit(netplay_shutdown); atexit(write_exit_file); s_atexit_set = 1; }
    s_enet = enet_host_create(NULL, 1, SMB_NET_CHANNEL_COUNT, 0, 0);
    if (!s_enet || enet_address_set_host(&address, s_host_name) != 0) {
        char why[300];
        snprintf(why, sizeof why, "Can't find server '%s'", s_host_name);
        if (s_enet) { enet_host_destroy(s_enet); s_enet = NULL; }
        netplay_push_notice(NP_EV_ERROR, why);
        return 0;
    }
    address.port = s_server_port;
    s_peer = enet_host_connect(s_enet, &address, SMB_NET_CHANNEL_COUNT, 0);
    if (!s_peer) {
        enet_host_destroy(s_enet);
        s_enet = NULL;
        netplay_push_notice(NP_EV_ERROR, "Couldn't start connecting");
        return 0;
    }
    s_connected = s_welcomed = s_disconnected = 0;
    s_got_room = 0;
    s_got_error = 0;
    s_game_started = 0;
    s_hello_sent = 0;
    s_connect_ms = enet_time_get();
    s_state = NP_LOBBY_CONNECTING;
    s_main_tid = SDL_GetCurrentThreadID();
    if (!s_watch_added) { SDL_AddEventWatch(net_event_watch, NULL); s_watch_added = 1; }
    printf("[netplay] connecting to %s:%u as '%s'\n", s_host_name, s_server_port, s_name);
    fflush(stdout);
    return 1;
}

void netplay_lobby_disconnect(void)
{
    if (s_active) return;
    if (s_peer && s_enet) {
        ENetEvent ev;
        int i;
        enet_peer_disconnect(s_peer, 0);
        for (i = 0; i < 10 && !s_disconnected; i++) {
            if (enet_host_service(s_enet, &ev, 10) > 0) {
                if (ev.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(ev.packet);
                else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) break;
            }
        }
    }
    lobby_close();
}

static void send_room_request(void)
{
    uint8_t buf[16];
    SmbNetWriter w;
    smbw_init(&w, buf, sizeof buf);
    if (s_want == WANT_CREATE) {
        smbw_u8(&w, SMB_MSG_CREATE_ROOM);
    } else {
        smbw_u8(&w, SMB_MSG_JOIN_ROOM);
        smbw_bytes(&w, s_want_code, SMB_NET_ROOM_CODE_LEN);
    }
    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
    s_want = WANT_NONE;
    s_got_room = 0;
    s_room_count = 0;
    s_state = NP_LOBBY_JOINING;
}

int netplay_lobby_create(int delay)
{
    if (s_state == NP_LOBBY_OFFLINE || s_active) return 0;
    s_delay = delay > SMB_NET_MAX_INPUT_DELAY ? SMB_NET_MAX_INPUT_DELAY : delay;
    s_want = WANT_CREATE;
    if (s_state == NP_LOBBY_READY || s_state == NP_LOBBY_IN_ROOM) send_room_request();
    return 1;
}

int netplay_lobby_join(const char *code)
{
    int k;
    if (s_state == NP_LOBBY_OFFLINE || s_active) return 0;
    if (!code || strlen(code) != SMB_NET_ROOM_CODE_LEN) {
        netplay_push_notice(NP_EV_ERROR, "A room code has 4 letters");
        return 0;
    }
    for (k = 0; k < SMB_NET_ROOM_CODE_LEN; k++) {
        char ch = code[k];
        s_want_code[k] = (char)((ch >= 'a' && ch <= 'z') ? ch - 32 : ch);
    }
    s_want_code[SMB_NET_ROOM_CODE_LEN] = 0;
    s_want = WANT_JOIN;
    if (s_state == NP_LOBBY_READY || s_state == NP_LOBBY_IN_ROOM) send_room_request();
    return 1;
}

void netplay_lobby_leave(void)
{
    if (s_state == NP_LOBBY_IN_ROOM || s_state == NP_LOBBY_JOINING) {
        send_simple(SMB_MSG_LEAVE_ROOM);
        s_state = NP_LOBBY_READY;
    }
    s_want = WANT_NONE;
    s_got_room = 0;
    s_room_count = 0;
    s_room_code[0] = 0;
}

int netplay_lobby_set_port(int port)
{
    uint8_t buf[4];
    SmbNetWriter w;
    if (s_state != NP_LOBBY_IN_ROOM || port < 0 || port >= SMB_NET_MAX_PLAYERS) return 0;
    smbw_init(&w, buf, sizeof buf);
    smbw_u8(&w, SMB_MSG_CHOOSE);
    smbw_u8(&w, (uint8_t)port);
    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
    return 1;
}

int netplay_lobby_set_delay(int delay)
{
    s_delay = delay > SMB_NET_MAX_INPUT_DELAY ? SMB_NET_MAX_INPUT_DELAY : delay;
    return 1;
}

int netplay_lobby_server_can_start(void) { return (s_caps & SMB_CAP_RESUME) != 0; }

/* ceil(one-way / 16.67 ms) + 1, one-way = (the two largest pings to the server) / 2, never below 2 */
static int suggested_delay(void)
{
    int i, a = 0, b = 0, oneway, d;
    for (i = 0; i < s_room_count; i++) {
        int p = s_room_pings[i];
        if (p > a) { b = a; a = p; }
        else if (p > b) b = p;
    }
    oneway = (a + b) / 2;
    d = (oneway * 60 + 999) / 1000 + 1;
    if (d < 2) d = 2;
    if (d > 10) d = 10;
    return d;
}


int netplay_lobby_start(void)
{
    uint8_t buf[96];
    SmbNetWriter w;
    SmbNetSessionConfig cfg = s_hello_cfg;
    int delay = s_delay >= 0 ? s_delay : suggested_delay();
    if (s_state != NP_LOBBY_IN_ROOM || !s_got_room) return 0;
    if (s_room_host != s_client_id) { netplay_push_notice(NP_EV_WARN, "Only the host can start the game"); return 0; }
    if (s_room_phase != SMB_ROOM_LOBBY) return 0;
    if (!s_blocking && !netplay_lobby_server_can_start()) {
        netplay_push_notice(NP_EV_ERROR, "This server is too old to start a game from inside the game (update the server)");
        return 0;
    }
    cfg.epoch = 0;
    cfg.inputDelay = (uint8_t)delay;
    cfg.flags = 0;
    smbw_init(&w, buf, sizeof buf);
    smbw_u8(&w, SMB_MSG_START_GAME);
    smbw_config(&w, &cfg);
    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
    {
        int i;
        printf("[netplay] starting the game with %d players, input delay %d:", s_room_count, delay);
        for (i = 0; i < s_room_count; i++) printf(" %s", s_room_names[i]);
        printf("\n");
        fflush(stdout);
    }
    return 1;
}

NetplayLobbyState netplay_lobby_state(void) { return s_active ? NP_LOBBY_IN_GAME : s_state; }

const char *netplay_lobby_status(void)
{
    static char buf[160];
    switch (netplay_lobby_state()) {
    case NP_LOBBY_OFFLINE: return "Not connected";
    case NP_LOBBY_CONNECTING: snprintf(buf, sizeof buf, "Connecting to %s ...", s_server_str); return buf;
    case NP_LOBBY_READY: snprintf(buf, sizeof buf, "Connected to %s", s_server_str); return buf;
    case NP_LOBBY_JOINING: return "Entering the room ...";
    case NP_LOBBY_IN_ROOM:
        snprintf(buf, sizeof buf, "Room %s: %d player%s", s_room_code, s_room_count, s_room_count == 1 ? "" : "s");
        return buf;
    case NP_LOBBY_STARTING: return "Starting the online game ...";
    case NP_LOBBY_RESUMING: return "Rejoining the room ...";
    case NP_LOBBY_IN_GAME: return "Online game running";
    }
    return "";
}

int netplay_lobby_room(NetplayRoomInfo *out)
{
    int i;
    if (s_state != NP_LOBBY_IN_ROOM || !s_got_room) return 0;
    memset(out, 0, sizeof *out);
    memcpy(out->code, s_room_code, sizeof out->code);
    out->count = s_room_count;
    out->self_id = s_client_id;
    out->host_id = s_room_host;
    out->self_is_host = s_room_host == s_client_id;
    out->self_port = -1;
    out->delay = s_delay;
    out->suggested_delay = suggested_delay();
    for (i = 0; i < SMB_NET_MAX_PLAYERS; i++) out->owner_id[i] = s_slot_owner[i] == SMB_NET_PORT_NONE ? -1 : s_slot_owner[i];
    for (i = 0; i < s_room_count; i++) {
        NetplayPlayer *p = &out->players[i];
        int port;
        p->id = s_room_ids[i];
        snprintf(p->name, sizeof p->name, "%s", s_room_names[i]);
        p->ping_ms = s_room_pings[i];
        p->is_host = s_room_ids[i] == s_room_host;
        p->is_self = s_room_ids[i] == s_client_id;
        p->port = -1;
        for (port = 0; port < SMB_NET_MAX_PLAYERS; port++)
            if (s_slot_owner[port] == s_room_ids[i]) p->port = port;
        if (p->is_self) out->self_port = p->port;
    }
    return 1;
}

/* ------------------------------------------------------ restart (relaunch) */

#ifdef _WIN32
static const char *const k_strip_value[] = { "--server", "--join", "--players", "--delay", "--name", "--room-code-file",
    "--lobby-timeout", "--lobby-test-script", "--np-resume", "--np-notice", "--np-wait-pid" };
static const char *const k_strip_flag[] = { "--create", "--np-lobby" };
/* dropped when going back to offline play (a session's own test/recording flags) */
static const char *const k_strip_session_value[] = { "--np-exit-file", "--record", "--replay", "--checksum-log",
    "--max-frames", "--input-script", "--session-epoch", "--session-locale", "--np-return-script" };
static const char *const k_strip_session_flag[] = { "--det-session", "--checksum-ring", "--np-lobby-return" };

static int in_list(const char *s, const char *const *list, int n)
{
    int i;
    for (i = 0; i < n; i++) if (strcmp(s, list[i]) == 0) return 1;
    return 0;
}
#define IN_LIST(s, l) in_list((s), (l), (int)(sizeof(l) / sizeof((l)[0])))

/* Appends one argument to a Windows command line with the usual quoting rules. */
static void append_warg(wchar_t *cmd, size_t cap, const wchar_t *arg)
{
    size_t n = wcslen(cmd), i, bs;
    int quote = arg[0] == 0 || wcspbrk(arg, L" \t\"") != NULL;
    if (n + 4 >= cap) return;
    if (n) cmd[n++] = L' ';
    if (quote) cmd[n++] = L'"';
    for (i = 0; arg[i] && n + 4 < cap; i++) {
        for (bs = 0; arg[i] == L'\\'; i++, bs++) ;
        if (arg[i] == 0) {                 /* trailing backslashes: double them before the closing quote */
            size_t k;
            for (k = 0; k < bs * (quote ? 2 : 1) && n + 4 < cap; k++) cmd[n++] = L'\\';
            break;
        } else if (arg[i] == L'"') {
            size_t k;
            for (k = 0; k < bs * 2 + 1 && n + 4 < cap; k++) cmd[n++] = L'\\';
            cmd[n++] = L'"';
        } else {
            size_t k;
            for (k = 0; k < bs && n + 4 < cap; k++) cmd[n++] = L'\\';
            cmd[n++] = arg[i];
        }
    }
    if (quote) cmd[n++] = L'"';
    cmd[n] = 0;
}

/* Starts this program again: its original command line minus the lobby arguments (and, going
 * offline, the session test flags), plus `extra` (UTF-8 arguments).  The new process waits for
 * this one to exit (--np-wait-pid).  Returns 1 if it was started. */
static int spawn_self(int to_offline, const char *const *extra, int nextra)
{
    static wchar_t cmd[32768];
    wchar_t exe[MAX_PATH * 2];
    wchar_t tmp[1024];
    char utf8[1024];
    int wargc = 0, i;
    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &wargc);
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    if (!wargv || wargc < 1) return 0;
    cmd[0] = 0;
    append_warg(cmd, 32768, wargv[0]);
    for (i = 1; i < wargc; i++) {
        int skip = 0;
        if (WideCharToMultiByte(CP_UTF8, 0, wargv[i], -1, utf8, sizeof utf8, NULL, NULL) == 0) utf8[0] = 0;
        if (IN_LIST(utf8, k_strip_value)) skip = 2;
        else if (IN_LIST(utf8, k_strip_flag)) skip = 1;
        else if (to_offline && IN_LIST(utf8, k_strip_session_value)) skip = 2;
        else if (to_offline && IN_LIST(utf8, k_strip_session_flag)) skip = 1;
        if (skip) { i += skip - 1; continue; }
        append_warg(cmd, 32768, wargv[i]);
    }
    LocalFree(wargv);
    for (i = 0; i < nextra; i++) {
        if (MultiByteToWideChar(CP_UTF8, 0, extra[i], -1, tmp, 1024) == 0) continue;
        append_warg(cmd, 32768, tmp);
    }
    if (GetModuleFileNameW(NULL, exe, MAX_PATH * 2) == 0) return 0;
    memset(&si, 0, sizeof si);
    si.cb = sizeof si;
    /* handles are inherited so a redirected stdout/stderr (test logs) keeps flowing into the same file */
    if (!CreateProcessW(exe, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
        printf("[netplay] can't restart the program: error %lu\n", (unsigned long)GetLastError());
        fflush(stdout);
        return 0;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    return 1;
}

void netplay_wait_for_parent(unsigned long pid)
{
    HANDLE h = OpenProcess(SYNCHRONIZE, FALSE, (DWORD)pid);
    if (!h) return;   /* already gone */
    WaitForSingleObject(h, 20000);
    CloseHandle(h);
}
#else
static int spawn_self(int to_offline, const char *const *extra, int nextra)
{
    (void)to_offline; (void)extra; (void)nextra;
    return 0;   /* restarting the program is implemented for Windows only (D-9) */
}
void netplay_wait_for_parent(unsigned long pid) { (void)pid; }
#endif

static uint32_t make_token(void)
{
    uint64_t t = SDL_GetPerformanceCounter() ^ ((uint64_t)time(NULL) << 20) ^ (uint64_t)(uintptr_t)&t;
    uint32_t x = (uint32_t)(t ^ (t >> 32)) * 2654435761u;
    return x ? x : 1;
}

/* Restarts the program and hands our seat to the new process: tells the server (RELAUNCHING + token, so the
 * disconnect is not a drop and the seat is held for SMB_NET_RESUME_GRACE_MS), starts the new process with
 * --np-resume CODE,ID,TOKEN, and closes the connection politely once RELAUNCHING is acknowledged.
 *   to_session: new process = the online session (E8)        else: new process = offline game back in the lobby
 *   (E9, --np-lobby): reconnects, RESUMEs into the room's lobby phase.
 * `notice` (may be NULL) becomes a toast in the new process.  Returns 1 if the new process was started (the caller
 * then quits); 0 = nothing changed except that the server was told we are restarting (callers drop the connection). */
static int hand_over(int to_session, const char *notice)
{
    char pid[32], resume[64], client_name_arg[SMB_NET_MAX_NAME + 1];
    const char *args[16];
    int n = 0, ok, i;
    uint32_t token = make_token();
    uint8_t buf[8];
    SmbNetWriter w;
    ENetEvent ev;

    smbw_init(&w, buf, sizeof buf);
    smbw_u8(&w, SMB_MSG_RELAUNCHING);
    smbw_u32(&w, token);
    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);

#ifdef _WIN32
    snprintf(pid, sizeof pid, "%lu", (unsigned long)GetCurrentProcessId());
#else
    snprintf(pid, sizeof pid, "0");
#endif
    snprintf(resume, sizeof resume, "%s,%u,%x", s_room_code, (unsigned)s_client_id, (unsigned)token);
    snprintf(client_name_arg, sizeof client_name_arg, "%s", s_name);
    args[n++] = "--server";
    args[n++] = s_server_str;
    args[n++] = "--name";
    args[n++] = client_name_arg;
    args[n++] = "--np-resume";
    args[n++] = resume;
    args[n++] = "--np-wait-pid";
    args[n++] = pid;
    if (!to_session) {
        args[n++] = "--np-lobby";
        if (s_return_script[0]) {
            args[n++] = "--lobby-test-script";
            args[n++] = s_return_script;
        }
    }
    if (notice && notice[0]) {
        args[n++] = "--np-notice";
        args[n++] = notice;
    }
    ok = spawn_self(!to_session, args, n);
    if (!ok) return 0;
    s_state = NP_LOBBY_STARTING;
    /* delivered + acknowledged before the disconnect (a plain disconnect can drop the RELAUNCHING) */
    if (s_peer) enet_peer_disconnect_later(s_peer, 0);
    for (i = 0; i < 60 && !s_disconnected; i++) {
        if (enet_host_service(s_enet, &ev, 25) > 0) {
            if (ev.type == ENET_EVENT_TYPE_RECEIVE) enet_packet_destroy(ev.packet);
            else if (ev.type == ENET_EVENT_TYPE_DISCONNECT) break;
        }
    }
    {
        NetplayLobbyState keep = s_state;
        lobby_close();
        s_state = keep;
    }
    return 1;
}

/* GAME_START arrived in the running game: restart into the session. */
static void begin_relaunch(void)
{
    if (!netplay_lobby_server_can_start()) {
        netplay_push_notice(NP_EV_ERROR, "This server can't restart the game into a session (update the server)");
        netplay_lobby_leave();
        s_game_started = 0;
        return;
    }
    printf("[netplay] GAME_START received in the running game: restarting into the session\n");
    fflush(stdout);
    if (!hand_over(1, NULL)) {
        netplay_push_notice(NP_EV_ERROR, "Couldn't restart the game for the online session");
        netplay_lobby_disconnect();   /* the server counts us as dropped */
        s_game_started = 0;
        return;
    }
    pc_quit_request("online game starting");
}

/* ------------------------------------------------------- lobby test script */

enum { ST_CONNECT, ST_CREATE, ST_JOIN, ST_CODEFILE, ST_START, ST_PORT, ST_REPORT, ST_EXIT };
typedef struct ScriptStep { long frame; int kind; char arg[400]; } ScriptStep;
static ScriptStep s_script[24];
static int s_script_n, s_script_pos;
static long s_script_tick;

/* "F:connect=HOST:PORT/NAME,F:create=DELAY,F:join=CODE|@FILE,F:codefile=FILE,F:start=N,F:port=P"
 * Steps run in order; F = offline frame (tick) from which the step may run; a step that has to wait
 * (join=@FILE until the file holds a code, codefile/start/port until in a room, start until N members) blocks
 * the following ones. */
int netplay_set_lobby_script(const char *spec)
{
    static char copy[2048];
    char *p, *save;
    static const char *const names[] = { "connect", "create", "join", "codefile", "start", "port", "report", "exit" };
    snprintf(copy, sizeof copy, "%s", spec);
    s_script_n = s_script_pos = 0;
    for (p = strtok_r(copy, ",", &save); p && s_script_n < 24; p = strtok_r(NULL, ",", &save)) {
        ScriptStep *st = &s_script[s_script_n];
        char *colon = strchr(p, ':'), *eq;
        int k;
        if (!colon) return 0;
        *colon = 0;
        st->frame = atol(p);
        p = colon + 1;
        eq = strchr(p, '=');
        if (eq) *eq++ = 0;
        st->kind = -1;
        for (k = 0; k < 8; k++) if (strcmp(p, names[k]) == 0) st->kind = k;
        if (st->kind < 0) return 0;
        snprintf(st->arg, sizeof st->arg, "%s", eq ? eq : "");
        s_script_n++;
    }
    return s_script_n > 0;
}

static void run_script(void)
{
    s_script_tick++;
    while (s_script_pos < s_script_n) {
        ScriptStep *st = &s_script[s_script_pos];
        NetplayRoomInfo ri;
        int in_room;
        if (s_script_tick < st->frame) break;
        in_room = netplay_lobby_room(&ri);
        switch (st->kind) {
        case ST_CONNECT: {
            char *slash = strchr(st->arg, '/');
            if (slash) *slash++ = 0;
            netplay_lobby_connect(st->arg, slash ? slash : "Player");
            break;
        }
        case ST_CREATE:
            netplay_lobby_create(st->arg[0] ? atoi(st->arg) : -1);
            break;
        case ST_JOIN: {
            char code[16] = "";
            if (st->arg[0] == '@') {
                FILE *f = fopen(st->arg + 1, "rb");
                if (f) {
                    if (fgets(code, sizeof code, f)) code[strcspn(code, "\r\n")] = 0;
                    fclose(f);
                }
                if (strlen(code) != SMB_NET_ROOM_CODE_LEN) return;   /* not there yet */
            } else {
                snprintf(code, sizeof code, "%s", st->arg);
            }
            netplay_lobby_join(code);
            break;
        }
        case ST_CODEFILE: {
            FILE *f;
            if (!in_room) return;
            f = fopen(st->arg, "wb");
            if (f) { fprintf(f, "%s\n", ri.code); fclose(f); }
            else fprintf(stderr, "[netplay] cannot write %s\n", st->arg);
            break;
        }
        case ST_START:
            if (!in_room || !ri.self_is_host || ri.count < atoi(st->arg)) return;
            netplay_lobby_start();
            break;
        case ST_PORT:
            if (!in_room) return;
            netplay_lobby_set_port(atoi(st->arg));
            break;
        case ST_REPORT: {   /* E9: write the lobby state (room kept?) to a file; waits until in a room */
            FILE *f;
            int k;
            if (!in_room) return;
            f = fopen(st->arg, "wb");
            if (f) {
                fprintf(f, "room=%s\ncount=%d\nhost=%d\nself=%d\nself_is_host=%d\nphase=lobby\nnames=", ri.code, ri.count,
                        ri.host_id, ri.self_id, ri.self_is_host);
                for (k = 0; k < ri.count; k++) fprintf(f, "%s%s", k ? "," : "", ri.players[k].name);
                fprintf(f, "\n");
                fclose(f);
            } else {
                fprintf(stderr, "[netplay] cannot write %s\n", st->arg);
            }
            printf("[netplay] lobby report written: room %s, %d players\n", ri.code, ri.count);
            fflush(stdout);
            break;
        }
        case ST_EXIT:
            printf("[netplay] lobby test script: exit\n");
            fflush(NULL);
            exit(0);
        }
        s_script_pos++;
    }
}

/* ------------------------------------------------------------ lobby tick */

void netplay_lobby_tick(void)
{
    uint32_t now;
    if (s_active) return;   /* lockstep owns the connection */
    if (s_enet && s_state != NP_LOBBY_STARTING) {
        net_service(0);
        now = enet_time_get();
        if (s_disconnected && (s_state != NP_LOBBY_RESUMING || s_blocking == 3)) {
            netplay_push_notice(NP_EV_ERROR, "Lost the connection to the server");
            lobby_close();
        } else if (s_state == NP_LOBBY_CONNECTING) {
            if (s_connected && !s_hello_sent && s_peer) {
                uint8_t buf[256];
                SmbNetWriter w;
                {
                    enet_peer_timeout(s_peer, 0, 10000, 30000);
                    smbw_init(&w, buf, sizeof buf);
                    smbw_u8(&w, SMB_MSG_HELLO);
                    smbw_u16(&w, SMB_NET_PROTOCOL_VERSION);
                    smbw_str(&w, s_name, SMB_NET_MAX_NAME);
                    smbw_str(&w, s_hello_cfg.buildId, SMB_NET_MAX_BUILD_ID);
                    smbw_u32(&w, s_hello_cfg.discHash);
                    smbw_u8(&w, s_hello_cfg.locale);
                    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
                    s_hello_sent = 1;
                }
            }
            if (s_welcomed) {
                char t[128];
                if (s_resume_set && (s_blocking == 2 || s_blocking == 3)) {   /* restarted process: take the seat back */
                    uint8_t buf[32];
                    SmbNetWriter w;
                    smbw_init(&w, buf, sizeof buf);
                    smbw_u8(&w, SMB_MSG_RESUME);
                    smbw_bytes(&w, s_resume_code, SMB_NET_ROOM_CODE_LEN);
                    smbw_u8(&w, (uint8_t)s_resume_id);
                    smbw_u32(&w, s_resume_token);
                    send_buf(SMB_NET_CHANNEL_CONTROL, buf, w.pos);
                    s_state = NP_LOBBY_RESUMING;
                } else {
                    s_state = NP_LOBBY_READY;
                    snprintf(t, sizeof t, "Connected to %s", s_server_str);
                    netplay_push_notice(NP_EV_INFO, t);
                }
            } else if (now - s_connect_ms > CONNECT_TIMEOUT_MS) {
                char why[300];
                snprintf(why, sizeof why, "No answer from %s:%u (is UDP port %u open?)", s_host_name, s_server_port,
                         s_server_port);
                netplay_push_notice(NP_EV_ERROR, why);
                lobby_close();
            }
        }
        if (s_state == NP_LOBBY_READY && s_want != WANT_NONE) send_room_request();
        if (s_state == NP_LOBBY_JOINING && s_got_room) {
            char t[64];
            s_state = NP_LOBBY_IN_ROOM;
            snprintf(t, sizeof t, "In room %s", s_room_code);
            netplay_push_notice(NP_EV_INFO, t);
        }
        if (s_state == NP_LOBBY_RESUMING && s_blocking == 3 && s_got_room) {
            /* E9: back in the room's lobby after a session */
            char t[64];
            s_blocking = 0;
            s_state = NP_LOBBY_IN_ROOM;
            snprintf(t, sizeof t, "Back in room %s", s_room_code);
            netplay_push_notice(NP_EV_INFO, t);
            printf("[netplay] back in room %s (lobby): %d player(s), host client %u, we are client %u\n", s_room_code,
                   (int)s_room_count, (unsigned)s_room_host, (unsigned)s_client_id);
            fflush(stdout);
        }
        if (s_got_error && (!s_blocking || s_blocking == 3)) {
            s_got_error = 0;
            if (s_state == NP_LOBBY_JOINING) s_state = NP_LOBBY_READY;
            if (s_state == NP_LOBBY_RESUMING && s_blocking == 3) {
                s_blocking = 0;
                s_state = NP_LOBBY_READY;   /* the seat is gone (grace over, room closed): connected, no room */
                netplay_push_notice(NP_EV_WARN, "The room is gone; you can create or join another");
            }
        }
        /* the host started: restart into the session (the command-line wrappers consume it themselves) */
        if (s_game_started && !s_blocking && (s_state == NP_LOBBY_IN_ROOM || s_state == NP_LOBBY_JOINING))
            begin_relaunch();
    }
    run_script();
}

/* ------------------------------------------------- command-line wrappers */

static int lobby_fail(const char *why)
{
    fprintf(stderr, "[netplay] %s\n", why);
    fflush(stderr);
    lobby_close();
    s_blocking = 0;
    netplay_shutdown();
    return 0;
}

static void fill_session_config(SessionConfig *out)
{
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
}

/* Blocking pre-boot lobby from --server/--create/--join (fresh process: no restart needed). */
int netplay_lobby_run(SessionConfig *out)
{
    uint32_t t0;
    int sent_start = 0, wrote_code = 0;
    int delay = s_opt.delay >= 0 && s_opt.delay <= SMB_NET_MAX_INPUT_DELAY ? s_opt.delay : 2;
    int players = s_opt.players > 0 ? s_opt.players : 2;
    int timeout_s = s_opt.lobby_timeout_s > 0 ? s_opt.lobby_timeout_s : 120;

    if (!s_opt.server || !s_opt.server[0]) return lobby_fail("--server <host[:port]> is required");
    if (!s_opt.create && (!s_opt.join_code || strlen(s_opt.join_code) != SMB_NET_ROOM_CODE_LEN))
        return lobby_fail("--join needs a 4-letter room code (or use --create)");
    s_blocking = 1;
    if (!netplay_lobby_connect(s_opt.server, s_opt.name)) return lobby_fail("couldn't connect");
    if (s_opt.create) netplay_lobby_create(delay);
    else netplay_lobby_join(s_opt.join_code);
    t0 = enet_time_get();
    for (;;) {
        NetplayRoomInfo ri;
        net_service(10);
        netplay_lobby_tick();
        if (s_state == NP_LOBBY_OFFLINE) return lobby_fail(s_got_error ? s_error : "disconnected from the server");
        if (s_got_error) return lobby_fail(s_error);
        if (enet_time_get() - t0 > (uint32_t)timeout_s * 1000u)
            return lobby_fail(s_state == NP_LOBBY_CONNECTING ? "no answer from the server (is UDP open?)"
                                                              : "timed out in the lobby");
        if (netplay_lobby_room(&ri)) {
            if (!wrote_code) {
                printf("[netplay] in room %s as client %u (host: client %u)\n", s_room_code, (unsigned)s_client_id,
                       (unsigned)s_room_host);
                if (s_opt.create && s_opt.code_file && s_opt.code_file[0]) {
                    FILE *f = fopen(s_opt.code_file, "wb");
                    if (f) { fprintf(f, "%s\n", s_room_code); fclose(f); }
                    else fprintf(stderr, "[netplay] cannot write %s\n", s_opt.code_file);
                }
                fflush(stdout);
                wrote_code = 1;
            }
            if (s_opt.create && !sent_start && ri.self_is_host && ri.count >= players && s_room_phase == SMB_ROOM_LOBBY)
                sent_start = netplay_lobby_start();
        }
        if (s_game_started) break;
    }
    {   /* safety net: the server already checks this at JOIN/START */
        const char *bad = smb_config_mismatch(&s_game_cfg, &s_hello_cfg);
        if (bad) {
            char why[200];
            snprintf(why, sizeof why, "the session's %s doesn't match ours", bad);
            return lobby_fail(why);
        }
    }
    s_blocking = 0;
    fill_session_config(out);
    return 1;
}

/* The restarted process: reconnect, RESUME, get GAME_START again. */
int netplay_lobby_resume(SessionConfig *out)
{
    uint32_t t0;
    int timeout_s = s_opt.lobby_timeout_s > 0 ? s_opt.lobby_timeout_s : 120;
    if (!s_resume_set) return lobby_fail("--np-resume is missing");
    if (!s_opt.server || !s_opt.server[0]) return lobby_fail("--server <host[:port]> is required");
    s_blocking = 2;
    if (!netplay_lobby_connect(s_opt.server, s_opt.name)) return lobby_fail("couldn't connect");
    t0 = enet_time_get();
    printf("[netplay] restarted for room %s as client %d: taking the seat back\n", s_resume_code, s_resume_id);
    fflush(stdout);
    while (!s_game_started) {
        net_service(10);
        netplay_lobby_tick();
        if (s_state == NP_LOBBY_OFFLINE) return lobby_fail(s_got_error ? s_error : "disconnected from the server");
        if (s_got_error) return lobby_fail(s_error);
        if (s_disconnected) return lobby_fail("disconnected from the server");
        if (enet_time_get() - t0 > (uint32_t)timeout_s * 1000u) return lobby_fail("timed out rejoining the room");
    }
    {
        const char *bad = smb_config_mismatch(&s_game_cfg, &s_hello_cfg);
        if (bad) {
            char why[200];
            snprintf(why, sizeof why, "the session's %s doesn't match ours", bad);
            return lobby_fail(why);
        }
    }
    s_blocking = 0;
    s_resumed = 1;
    fill_session_config(out);
    return 1;
}
/* ------------------------------------------------------------ lockstep */

/* End of the session (E9).  `room_alive` = the server ended the game but the room still exists in its lobby phase
 * (desync, the host ended it, everyone else left): then a session that was started from the in-game lobby goes BACK
 * TO THE LOBBY: the program restarts (a session is a fresh process, E1 D20), tells the server RELAUNCHING (the seat is
 * held), and the new offline process RESUMEs into the room (--np-lobby).  Otherwise (lost the server, timeout) it
 * restarts into plain offline play.  Both show `why` as a toast.  Command-line and automated runs exit with `code`
 * instead (0 ended normally / window closed, 3 lost the server or timed out, 4 desync), unless --np-lobby-return
 * (test) asks to go back to the lobby anyway. */
static void session_end(int code, const char *why, int room_alive)
{
    printf("[netplay] %s\n", why);
    fflush(NULL);
#ifdef _WIN32
    if ((s_resumed && !s_automated) || s_test_return) {
        char notice[256];
        if (room_alive && !s_disconnected && s_peer && s_room_code[0] && (s_caps & SMB_CAP_LOBBY_RESUME)) {
            snprintf(notice, sizeof notice, "Back in the lobby - %s", why);
            s_active = 0;
            if (hand_over(0, notice)) {
                printf("[netplay] restarted back into the room lobby (room %s kept)\n", s_room_code);
                fflush(NULL);
                exit(0);
            }
            printf("[netplay] can't restart back into the lobby, going offline\n");
            fflush(NULL);
        }
    }
#endif
    netplay_shutdown();
#ifdef _WIN32
    if (s_resumed && !s_automated) {
        char notice[256], pid[32];
        const char *args[4];
        snprintf(notice, sizeof notice, "Online session ended: %s", why);
        snprintf(pid, sizeof pid, "%lu", (unsigned long)GetCurrentProcessId());
        args[0] = "--np-notice";
        args[1] = notice;
        args[2] = "--np-wait-pid";
        args[3] = pid;
        if (spawn_self(1, args, 4)) {
            printf("[netplay] restarted into offline mode\n");
            fflush(NULL);
            exit(0);
        }
    }
#endif
    exit(code);
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
        char why[160];
        if (s_end_reason == SMB_END_DESYNC)
            snprintf(why, sizeof why, "the game ended: DESYNC (the players' game states differ near poll %u)",
                     (unsigned)(s_desync ? s_desync_poll : s_next_poll));
        else if (s_end_reason == SMB_END_HOST)
            snprintf(why, sizeof why, "the game ended: the host ended it");
        else if (s_end_reason == SMB_END_PLAYER_LEFT)
            snprintf(why, sizeof why, "the game ended: everyone else left");
        else
            snprintf(why, sizeof why, "the game ended");
        netplay_push_notice(s_end_reason == SMB_END_DESYNC ? NP_EV_ERROR : NP_EV_INFO, why);
        session_end(s_end_reason == SMB_END_DESYNC ? 4 : 0, why, 1);
    }
    if (s_disconnected) session_end(3, "lost the connection to the server", 0);
}

/* E9 test hooks that act at a poll (SMB_NETPLAY_TEST_DROP / _STALL / _ENDGAME). */
static void run_test_hooks(uint32_t poll)
{
    if (s_hook_drop >= 0 && (long)poll >= s_hook_drop) {
        printf("[netplay] TEST HOOK: dropping out at poll %u (%s)\n", (unsigned)poll,
               s_hook_drop_mode == 0 ? "hard kill, no goodbye" : s_hook_drop_mode == 1 ? "Quit to Desktop" : "LEAVE_ROOM");
        fflush(NULL);
        s_hook_drop = -1;
        if (s_hook_drop_mode == 1) {
            pc_quit_request("Quit to Desktop (test hook)");   /* exits at the next frame via pc_quit_poll */
        } else if (s_hook_drop_mode == 2) {
            send_simple(SMB_MSG_LEAVE_ROOM);
            exit(0);
        } else {
#ifdef _WIN32
            TerminateProcess(GetCurrentProcess(), 99);
#else
            _Exit(99);
#endif
        }
    }
    if (s_hook_stall >= 0 && (long)poll >= s_hook_stall) {
        long left = s_hook_stall_ms;
        printf("[netplay] TEST HOOK: stalling the main loop for %ld ms at poll %u (%s)\n", left, (unsigned)poll,
               s_hook_stall_net ? "network still serviced" : "network frozen");
        fflush(NULL);
        s_hook_stall = -1;
        while (left > 0) {
            long step = left > 100 ? 100 : left;
#ifdef _WIN32
            Sleep((DWORD)step);
#endif
            left -= step;
            BUMP_PULSE();   /* a frozen process would trip the watchdog; this hook only freezes the sim/network */
            if (s_hook_stall_net) net_service(0);
        }
        printf("[netplay] TEST HOOK: stall over\n");
        fflush(NULL);
    }
    if (s_hook_endgame >= 0 && (long)poll >= s_hook_endgame) {
        s_hook_endgame = -1;
        printf("[netplay] TEST HOOK: host ends the game at poll %u\n", (unsigned)poll);
        fflush(NULL);
        netplay_end_game();
    }
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
    run_test_hooks(poll);

    {   /* checksum E3 computed for the poll just completed (every 60) */
        uint32_t h, cp = checksum_latest(&h);
        if (cp > s_last_cs_sent) {
            uint8_t b[16];
            SmbNetWriter w;
            smbw_init(&w, b, sizeof b);
            smbw_u8(&w, SMB_MSG_CHECKSUM);
            smbw_u32(&w, cp);
            if (s_hook_desync >= 0 && (long)cp >= s_hook_desync) {
                if (s_hook_desync != -2) {
                    printf("[netplay] TEST HOOK: sending a wrong checksum from poll %u on\n", (unsigned)cp);
                    fflush(NULL);
                }
                h ^= 0x5EED0001u;
            }
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
        if (waited > (poll < 4 ? FIRST_POLL_TIMEOUT_MS : WAIT_TIMEOUT_MS))
            session_end(3, "timed out waiting for the other players", 0);
    }
    if (last_msg) set_wait_title(NULL);

    if (slot->mask != s_human_mask) {
        /* E9: a player joined/dropped; the mask is per poll and identical everywhere */
        if (s_human_mask != 0xFF) {
            printf("[netplay] poll %u: human mask %02x -> %02x\n", (unsigned)poll, (unsigned)s_human_mask,
                   (unsigned)slot->mask);
            fflush(stdout);
            for (port = 0; port < INPUTSYNC_PORTS; port++)
                if ((s_human_mask & (1 << port)) && !(slot->mask & (1 << port))) {
                    printf("[netplay] poll %u: P%d (%s) is neutral from here on (no controller)\n", (unsigned)poll,
                           port + 1, s_port_names[port]);
                    fflush(stdout);
                }
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

static void read_test_hooks(void)
{
    const char *v;
    if ((v = getenv("SMB_NETPLAY_TEST_DROP")) && v[0]) {
        s_hook_drop = atol(v);
        s_hook_drop_mode = strstr(v, ":quit") ? 1 : strstr(v, ":leave") ? 2 : 0;
    }
    if ((v = getenv("SMB_NETPLAY_TEST_DESYNC")) && v[0]) s_hook_desync = atol(v);
    if ((v = getenv("SMB_NETPLAY_TEST_ENDGAME")) && v[0]) s_hook_endgame = atol(v);
    if ((v = getenv("SMB_NETPLAY_TEST_STALL")) && v[0]) {
        const char *c = strchr(v, ':');
        s_hook_stall = atol(v);
        s_hook_stall_ms = c ? atol(c + 1) : 10000;
        s_hook_stall_net = strstr(v, ":net") != NULL;
    }
    if (s_hook_drop >= 0 || s_hook_desync >= 0 || s_hook_stall >= 0 || s_hook_endgame >= 0)
        printf("[netplay] test hooks: drop %ld desync %ld stall %ld/%ldms endgame %ld\n", s_hook_drop, s_hook_desync,
               s_hook_stall, s_hook_stall_ms, s_hook_endgame);
}

int netplay_begin(void)
{
    if (!s_game_started || !s_peer) return 0;
    s_active = 1;
    s_state = NP_LOBBY_IN_GAME;
    s_main_tid = SDL_GetCurrentThreadID();
    inputsync_set_netplay(net_exchange, NULL);
    read_test_hooks();
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

/* ------------------------------------------------------------ E9 API */

void netplay_set_test_lobby_return(int on) { s_test_return = on; }
void netplay_set_return_script(const char *spec) { snprintf(s_return_script, sizeof s_return_script, "%s", spec ? spec : ""); }

/* Restarted back to the lobby (--np-lobby): reconnect, RESUME, land in NP_LOBBY_IN_ROOM (netplay_lobby_tick). */
int netplay_lobby_return(const char *server, const char *name)
{
    if (!s_resume_set) return 0;
    s_blocking = 3;
    if (!netplay_lobby_connect(server, name)) { s_blocking = 0; return 0; }
    printf("[netplay] restarted back to the lobby of room %s as client %d: taking the seat back\n", s_resume_code,
           s_resume_id);
    fflush(stdout);
    return 1;
}

int netplay_end_game(void)
{
    if (!s_active || !s_peer || s_room_host != s_client_id) return 0;
    send_simple(SMB_MSG_END_GAME);
    return 1;
}
