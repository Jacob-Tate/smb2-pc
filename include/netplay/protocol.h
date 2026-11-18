#ifndef SMB_NETPLAY_PROTOCOL_H
#define SMB_NETPLAY_PROTOCOL_H

/*
 * SMB2 PC netplay wire protocol, shared by the game client (C) and the relay server (C++).
 * Adapted from partyboard's protocol.h. Full message table: docs/handoff/E5.md.
 *
 * Transport: ENet (UDP). Channel 0 carries control messages, channel 1 carries inputs; both are
 * reliable and ordered (ordering is NOT guaranteed between the two channels). Every packet starts
 * with a one-byte message type, then the fields in order, little-endian, no padding. Strings are
 * a u8 length followed by that many bytes (no terminator).
 *
 * The server never runs game code: it relays pad input and compares checksums. A pad is an opaque
 * 9-byte blob (SmbNetPad) to the server.
 *
 * Time base: everything is indexed by POLL number, not by game tick. A poll is one call of
 * input_poll_gc_pads from video_finish_frame (docs/handoff/E1.md: Baseball calls it 2 extra times
 * without ticking the sim). Poll numbers start at 0 for each GAME_START.
 *
 * Lockstep with input delay D: when the client is about to complete poll P it sends INPUT for poll
 * P+D, then blocks until FRAME for poll P arrives. Right after GAME_START it sends neutral INPUT
 * for polls 0..D-1 (the server expects every human port to supply every poll starting from 0).
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SMB_NET_PROTOCOL_VERSION 1
#define SMB_NET_DEFAULT_PORT 7855
#define SMB_NET_CHANNEL_CONTROL 0
#define SMB_NET_CHANNEL_INPUT 1
#define SMB_NET_CHANNEL_COUNT 2
#define SMB_NET_MAX_PLAYERS 4
#define SMB_NET_MAX_NAME 24
#define SMB_NET_MAX_BUILD_ID 40
#define SMB_NET_ROOM_CODE_LEN 4
#define SMB_NET_PAD_SIZE 9            /* per-port pad on the wire */
#define SMB_NET_PORT_NONE 0xFF        /* slot owner value: nobody plays this port */
#define SMB_NET_KEEP 0xFF             /* CHOOSE: leave this field unchanged */
#define SMB_NET_CHECKSUM_INTERVAL 60  /* clients report a checksum every this many polls */
#define SMB_NET_MAX_INPUT_DELAY 30
#define SMB_NET_LATE_MS 20            /* server: lateness beyond a client's usual that counts as "late" */
#define SMB_NET_CLIENT_ID_NONE 0xFF
#define SMB_CAP_RESUME 0x01           /* server understands RELAUNCHING / RESUME */
#define SMB_CAP_LOBBY_RESUME 0x02     /* E9: RELAUNCHING / RESUME also work in the lobby phase ("back to the lobby" after a game) */
#define SMB_NET_RESUME_GRACE_MS 90000 /* a relaunching member has this long to come back */

/* Compile-time size check usable from C99 and C++. */
#define SMB_NET_STATIC_ASSERT(name, cond) typedef char smb_net_assert_##name[(cond) ? 1 : -1]

/* FRAME flags */
#define SMB_FRAME_PORTS_CHANGED 0x01 /* humanMask differs from the previous FRAME (a player dropped) */

/* SessionConfig.flags (unknown bits are rejected by the server) */
#define SMB_CFG_FLAG_ALLOW_CROSS_ARCH 0x00000001u /* i686 and x64 may mix (only after E1 test T6 passes) */
#define SMB_CFG_FLAGS_KNOWN (SMB_CFG_FLAG_ALLOW_CROSS_ARCH)

/* ROOM_STATE phase */
#define SMB_ROOM_LOBBY 0
#define SMB_ROOM_IN_GAME 1

typedef enum SmbNetMsg {
    /* client -> server */
    SMB_MSG_HELLO = 1,       /* u16 protocol, str name, str buildId, u32 discHash, u8 locale */
    SMB_MSG_CREATE_ROOM = 2, /* (empty) */
    SMB_MSG_JOIN_ROOM = 3,   /* char code[4] (case-insensitive) */
    SMB_MSG_LEAVE_ROOM = 4,  /* (empty). In a game this is a drop: the port goes neutral. */
    SMB_MSG_START_GAME = 5,  /* host only: SmbNetSessionConfig (58 bytes). The server overwrites the epoch. */
    SMB_MSG_INPUT = 6,       /* u32 poll, u8 gameSeq, then u8 pad[9] for each port this client owns, in port order. Wrong gameSeq: ignored. */
    SMB_MSG_CHECKSUM = 7,    /* u32 poll, u32 hash. Sent when poll % SMB_NET_CHECKSUM_INTERVAL == 0 (poll > 0). */
    SMB_MSG_CHOOSE = 8,      /* lobby: u8 port (0-3 or SMB_NET_KEEP). Move to a free port. */
    SMB_MSG_PERF = 9,        /* u32 poll, u16 polls, u16 slowPolls, u16 stalls, u16 worstMs (since the last PERF) */
    SMB_MSG_END_GAME = 10,   /* host only: end the running game for everyone (room returns to the lobby) */
    /* E8 (additive, protocol version unchanged; needs SMB_CAP_RESUME in the trailing byte of WELCOME): "I got
     * GAME_START and am restarting the program for the session; I will be back". Sent in a running game right
     * before disconnecting: u32 token (random, chosen by the client). The server then keeps the member's seat
     * (no drop) for SMB_NET_RESUME_GRACE_MS instead of treating the disconnect as a leave. */
    SMB_MSG_RELAUNCHING = 11,
    /* Sent by the restarted process right after HELLO instead of CREATE/JOIN: char code[4], u8 clientId (the id
     * WELCOME gave the old process), u32 token. The server rebinds the seat to the new connection and answers with
     * WELCOME (old id), ROOM_STATE, GAME_START (same config/epoch/ports/seq as the original). */
    SMB_MSG_RESUME = 12,

    /* server -> client */
    SMB_MSG_WELCOME = 64,    /* u8 clientId, [u8 caps (SMB_CAP_*); absent from servers older than E8] */
    SMB_MSG_ERROR = 65,      /* u8 SmbNetError, str message */
    /* char code[4], u8 hostId, u8 count, count x (u8 id, str name, u16 pingMs: round trip to the server),
     * 4 x u8 owner (client id or SMB_NET_PORT_NONE), u8 phase (SMB_ROOM_*). */
    SMB_MSG_ROOM_STATE = 66,
    SMB_MSG_GAME_START = 67, /* SmbNetSessionConfig (58 bytes, epoch filled in), 4 x u8 portOwner, u8 gameSeq (1, 2, ... per game in a room) */
    SMB_MSG_FRAME = 68,      /* u32 poll, u8 gameSeq (drop FRAMEs from an older game: channel 1 is unordered against channel 0), u8 humanMask, u8 flags (SMB_FRAME_*), u8 pad[9] for each humanMask bit (port 0..3) */
    SMB_MSG_DESYNC = 69,     /* u32 poll. GAME_ENDED (reason DESYNC) and ROOM_STATE (lobby) follow. */
    SMB_MSG_GAME_ENDED = 70, /* u8 reason (SmbNetEndReason), u8 clientId (who left, or SMB_NET_CLIENT_ID_NONE) */
    /* A player left mid-game. FRAME humanMask is authoritative (and per poll); this only says why.
     * From poll `poll` on, the ports in `ports` are neutral. */
    SMB_MSG_PLAYER_DROPPED = 71, /* u8 clientId, u8 ports (bit per port), u32 poll */
    /* The game is held up: inputs of these ports (bit per port) haven't arrived for u16 ms. Sent every
     * half second while a poll waits; the next FRAME means it is moving again. */
    SMB_MSG_WAITING = 72,    /* u8 missingPorts, u16 waitedMs */
    /* Once a second during a game: u8 count, count x (u8 id, u16 pingMs, u16 jitterMs, u8 lossPct,
     * u16 lateCount, u16 slowPolls, u16 stalls, u16 worstMs). late = server-side: this client's input was
     * the last to arrive, beyond its usual lateness by >= SMB_NET_LATE_MS; slow/stalls/worst come from
     * its PERF reports. Late + slow = slow PC, late alone = connection. */
    SMB_MSG_NET_STATS = 73
} SmbNetMsg;

typedef enum SmbNetError {
    SMB_ERR_BAD_MESSAGE = 1,
    SMB_ERR_VERSION = 2,         /* protocol version mismatch */
    SMB_ERR_NO_SUCH_ROOM = 3,
    SMB_ERR_ROOM_FULL = 4,
    SMB_ERR_IN_GAME = 5,         /* room is already playing */
    SMB_ERR_NOT_HOST = 6,
    SMB_ERR_BAD_SETTINGS = 7,
    SMB_ERR_TAKEN = 8,           /* CHOOSE asked for a port someone else holds */
    SMB_ERR_CONFIG_MISMATCH = 9  /* build id / disc hash / locale differs from the room's; message names it */
} SmbNetError;

typedef enum SmbNetEndReason {
    SMB_END_PLAYER_LEFT = 1, /* the last human left */
    SMB_END_DESYNC = 2,
    SMB_END_HOST = 3         /* host sent END_GAME */
} SmbNetEndReason;

/* One port's pad, from GC PADStatus (post stick gate, see E1). */
#pragma pack(push, 1)
typedef struct SmbNetPad {
    uint16_t buttons;
    int8_t stickX, stickY;
    int8_t substickX, substickY;
    uint8_t triggerL, triggerR;
    int8_t err;
} SmbNetPad;

/* Settings every PC must agree on. The first three are "must match" between members (checked at
 * JOIN and START_GAME); epoch is chosen by the server; delay and flags by the host. */
typedef struct SmbNetSessionConfig {
    char buildId[SMB_NET_MAX_BUILD_ID]; /* NUL-padded; same build on every PC */
    uint32_t discHash;                  /* disc manifest hash (fst_crc32) */
    uint8_t locale;
    uint64_t epoch;                     /* session epoch (seeds the deterministic clock) */
    uint8_t inputDelay;                 /* polls between sampling a pad and that input taking effect */
    uint32_t flags;                     /* SMB_CFG_FLAG_* */
} SmbNetSessionConfig;
#pragma pack(pop)

#define SMB_NET_SESSION_CONFIG_WIRE_SIZE 58
SMB_NET_STATIC_ASSERT(pad_size, sizeof(SmbNetPad) == SMB_NET_PAD_SIZE);
SMB_NET_STATIC_ASSERT(config_size, sizeof(SmbNetSessionConfig) == SMB_NET_SESSION_CONFIG_WIRE_SIZE);
SMB_NET_STATIC_ASSERT(config_size_sum,
                      SMB_NET_MAX_BUILD_ID + 4 + 1 + 8 + 1 + 4 == SMB_NET_SESSION_CONFIG_WIRE_SIZE);

/* ---- Byte buffer helpers (bounds-checked, little-endian) ---- */

typedef struct SmbNetWriter {
    uint8_t *data;
    size_t size;
    size_t pos;
    int overflow;
} SmbNetWriter;

typedef struct SmbNetReader {
    const uint8_t *data;
    size_t size;
    size_t pos;
    int error;
} SmbNetReader;

static inline void smbw_init(SmbNetWriter *w, uint8_t *data, size_t size) {
    w->data = data; w->size = size; w->pos = 0; w->overflow = 0;
}
static inline void smbw_bytes(SmbNetWriter *w, const void *src, size_t n) {
    if (w->overflow || w->pos + n > w->size) { w->overflow = 1; return; }
    memcpy(w->data + w->pos, src, n);
    w->pos += n;
}
static inline void smbw_u8(SmbNetWriter *w, uint8_t v) { smbw_bytes(w, &v, 1); }
static inline void smbw_u16(SmbNetWriter *w, uint16_t v) {
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v >> 8) };
    smbw_bytes(w, b, 2);
}
static inline void smbw_u32(SmbNetWriter *w, uint32_t v) {
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
    smbw_bytes(w, b, 4);
}
static inline void smbw_u64(SmbNetWriter *w, uint64_t v) {
    smbw_u32(w, (uint32_t)v);
    smbw_u32(w, (uint32_t)(v >> 32));
}
static inline void smbw_str(SmbNetWriter *w, const char *s, size_t maxLen) {
    size_t n = s ? strlen(s) : 0;
    if (n > maxLen) n = maxLen;
    smbw_u8(w, (uint8_t)n);
    smbw_bytes(w, s, n);
}
static inline void smbw_pad(SmbNetWriter *w, const SmbNetPad *p) {
    smbw_u16(w, p->buttons);
    smbw_u8(w, (uint8_t)p->stickX);
    smbw_u8(w, (uint8_t)p->stickY);
    smbw_u8(w, (uint8_t)p->substickX);
    smbw_u8(w, (uint8_t)p->substickY);
    smbw_u8(w, p->triggerL);
    smbw_u8(w, p->triggerR);
    smbw_u8(w, (uint8_t)p->err);
}
static inline void smbw_config(SmbNetWriter *w, const SmbNetSessionConfig *c) {
    char id[SMB_NET_MAX_BUILD_ID];
    memset(id, 0, sizeof(id));
    { size_t n = 0; while (n < SMB_NET_MAX_BUILD_ID && c->buildId[n]) n++; memcpy(id, c->buildId, n); }
    smbw_bytes(w, id, SMB_NET_MAX_BUILD_ID);
    smbw_u32(w, c->discHash);
    smbw_u8(w, c->locale);
    smbw_u64(w, c->epoch);
    smbw_u8(w, c->inputDelay);
    smbw_u32(w, c->flags);
}

static inline void smbr_init(SmbNetReader *r, const uint8_t *data, size_t size) {
    r->data = data; r->size = size; r->pos = 0; r->error = 0;
}
static inline void smbr_bytes(SmbNetReader *r, void *dst, size_t n) {
    if (r->error || r->pos + n > r->size) { r->error = 1; memset(dst, 0, n); return; }
    memcpy(dst, r->data + r->pos, n);
    r->pos += n;
}
static inline uint8_t smbr_u8(SmbNetReader *r) { uint8_t v; smbr_bytes(r, &v, 1); return v; }
static inline uint16_t smbr_u16(SmbNetReader *r) {
    uint8_t b[2]; smbr_bytes(r, b, 2);
    return (uint16_t)(b[0] | (b[1] << 8));
}
static inline uint32_t smbr_u32(SmbNetReader *r) {
    uint8_t b[4]; smbr_bytes(r, b, 4);
    return (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
}
static inline uint64_t smbr_u64(SmbNetReader *r) {
    uint64_t lo = smbr_u32(r);
    uint64_t hi = smbr_u32(r);
    return lo | (hi << 32);
}
/* Reads a string into dst (capacity cap, always NUL-terminated); longer strings are an error. */
static inline void smbr_str(SmbNetReader *r, char *dst, size_t cap) {
    uint8_t n = smbr_u8(r);
    if (r->error || n >= cap) { r->error = 1; dst[0] = '\0'; return; }
    smbr_bytes(r, dst, n);
    dst[r->error ? 0 : n] = '\0';
}
static inline void smbr_pad(SmbNetReader *r, SmbNetPad *p) {
    p->buttons = smbr_u16(r);
    p->stickX = (int8_t)smbr_u8(r);
    p->stickY = (int8_t)smbr_u8(r);
    p->substickX = (int8_t)smbr_u8(r);
    p->substickY = (int8_t)smbr_u8(r);
    p->triggerL = smbr_u8(r);
    p->triggerR = smbr_u8(r);
    p->err = (int8_t)smbr_u8(r);
}
static inline void smbr_config(SmbNetReader *r, SmbNetSessionConfig *c) {
    memset(c, 0, sizeof(*c));
    smbr_bytes(r, c->buildId, SMB_NET_MAX_BUILD_ID);
    c->discHash = smbr_u32(r);
    c->locale = smbr_u8(r);
    c->epoch = smbr_u64(r);
    c->inputDelay = smbr_u8(r);
    c->flags = smbr_u32(r);
}

/* A pad with nothing pressed, sticks centred, err = 0 (what a dropped port reads). */
static inline void smb_pad_neutral(SmbNetPad *p) { memset(p, 0, sizeof(*p)); }

/* Validates the host-chosen parts of a config the server will hand to every client. */
static inline int smb_config_valid(const SmbNetSessionConfig *c) {
    return c->inputDelay <= SMB_NET_MAX_INPUT_DELAY && (c->flags & ~SMB_CFG_FLAGS_KNOWN) == 0;
}

/* The "must match" fields: build id, disc manifest hash, locale. Returns NULL if equal, else the
 * name of the first differing field (for error messages). */
static inline const char *smb_config_mismatch(const SmbNetSessionConfig *a, const SmbNetSessionConfig *b) {
    if (memcmp(a->buildId, b->buildId, SMB_NET_MAX_BUILD_ID) != 0) return "build id";
    if (a->discHash != b->discHash) return "disc image";
    if (a->locale != b->locale) return "language";
    return NULL;
}

#ifdef __cplusplus
}
#endif

#endif
