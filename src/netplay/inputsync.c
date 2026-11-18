/* Input sync: live / record / replay / netplay pad source (E3); see inputsync.h. */
#include "netplay/inputsync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REC_MAGIC "SMB2REC1"
#define REC_VERSION 1
#define REC_HEADER_SIZE (8 + 2 + 2 + PC_SESSION_BUILD_ID_MAX + 4 + 1 + 8 + 4 + 1 + 1)
#define REC_POLL_SIZE (INPUTSYNC_PORTS * SMB_NET_PAD_SIZE)

static InputSyncSource s_src = INPUTSYNC_LIVE;
static uint64_t s_poll;
static uint64_t s_max_polls;
static FILE *s_rec;
static uint8_t *s_replay;       /* whole file */
static size_t s_replay_size;    /* bytes of pad data after the header */
static const uint8_t *s_replay_data;
static InputSyncNetFn s_net_fn;
static void *s_net_user;
static int s_atexit_set;

static void close_rec(void)
{
    if (s_rec) {
        fclose(s_rec);
        s_rec = NULL;
        printf("[record] closed after %llu polls\n", (unsigned long long)s_poll);
    }
}

int inputsync_open_record(const char *path)
{
    const SessionConfig *c = pc_session_config();
    uint8_t hdr[REC_HEADER_SIZE];
    SmbNetWriter w;

    if (!c || !path || s_src != INPUTSYNC_LIVE)
        return 0;
    s_rec = fopen(path, "wb");
    if (!s_rec) {
        fprintf(stderr, "cannot open record file %s\n", path);
        return 0;
    }
    smbw_init(&w, hdr, sizeof hdr);
    smbw_bytes(&w, REC_MAGIC, 8);
    smbw_u16(&w, REC_VERSION);
    smbw_u16(&w, 0);
    smbw_bytes(&w, c->build_id, PC_SESSION_BUILD_ID_MAX);
    smbw_u32(&w, c->disc_hash);
    smbw_u8(&w, c->locale);
    smbw_u64(&w, c->epoch);
    smbw_u32(&w, c->flags);
    smbw_u8(&w, (uint8_t)sizeof(void *));
    smbw_u8(&w, INPUTSYNC_PORTS);
    if (w.overflow || w.pos != sizeof hdr || fwrite(hdr, 1, sizeof hdr, s_rec) != sizeof hdr) {
        fprintf(stderr, "record header write failed\n");
        return 0;
    }
    fflush(s_rec);
    s_src = INPUTSYNC_RECORD;
    if (!s_atexit_set) { atexit(close_rec); s_atexit_set = 1; }
    printf("[record] recording pads to %s\n", path);
    return 1;
}

int inputsync_open_replay(const char *path, SessionConfig *cfg)
{
    FILE *f;
    long size;
    SmbNetReader r;
    uint8_t magic[8];
    unsigned ver, ports;

    if (!path || !cfg || s_src != INPUTSYNC_LIVE)
        return 0;
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "cannot open replay file %s\n", path);
        return 0;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (size < (long)REC_HEADER_SIZE) {
        fprintf(stderr, "replay file %s is too short\n", path);
        fclose(f);
        return 0;
    }
    s_replay = (uint8_t *)malloc((size_t)size);
    if (!s_replay || fread(s_replay, 1, (size_t)size, f) != (size_t)size) {
        fprintf(stderr, "cannot read replay file %s\n", path);
        fclose(f);
        free(s_replay);
        s_replay = NULL;
        return 0;
    }
    fclose(f);
    r.data = s_replay; r.size = (size_t)size; r.pos = 0; r.error = 0;
    smbr_bytes(&r, magic, 8);
    ver = smbr_u16(&r);
    (void)smbr_u16(&r);
    memset(cfg, 0, sizeof *cfg);
    smbr_bytes(&r, cfg->build_id, PC_SESSION_BUILD_ID_MAX);
    cfg->build_id[PC_SESSION_BUILD_ID_MAX - 1] = 0;
    cfg->disc_hash = smbr_u32(&r);
    cfg->locale = smbr_u8(&r);
    cfg->epoch = smbr_u64(&r);
    cfg->flags = smbr_u32(&r);
    (void)smbr_u8(&r);   /* recording arch pointer size */
    ports = smbr_u8(&r);
    if (r.error || memcmp(magic, REC_MAGIC, 8) != 0 || ver != REC_VERSION || ports != INPUTSYNC_PORTS) {
        fprintf(stderr, "%s is not a version-%d SMB2 input recording\n", path, REC_VERSION);
        free(s_replay);
        s_replay = NULL;
        return 0;
    }
    s_replay_data = s_replay + REC_HEADER_SIZE;
    s_replay_size = (size_t)size - REC_HEADER_SIZE;
    s_src = INPUTSYNC_REPLAY;
    printf("[replay] %s: %llu polls recorded (build '%s', epoch %016llx, locale %u)\n", path,
           (unsigned long long)(s_replay_size / REC_POLL_SIZE), cfg->build_id,
           (unsigned long long)cfg->epoch, (unsigned)cfg->locale);
    return 1;
}

void inputsync_set_netplay(InputSyncNetFn fn, void *user)
{
    s_net_fn = fn;
    s_net_user = user;
    /* --record keeps recording the exchanged bundle (all 4 ports) while netplay runs */
    if (fn && (s_src == INPUTSYNC_LIVE || s_src == INPUTSYNC_RECORD))
        s_src = INPUTSYNC_NETPLAY;
    else if (!fn && s_src == INPUTSYNC_NETPLAY)
        s_src = INPUTSYNC_LIVE;
}

void inputsync_set_max_polls(uint64_t n) { s_max_polls = n; }
InputSyncSource inputsync_source(void) { return s_src; }
int inputsync_overrides(void) { return s_src != INPUTSYNC_LIVE; }
uint64_t inputsync_poll_count(void) { return s_poll; }

static void finish(const char *why)
{
    printf("[inputsync] %s at poll %llu - exiting.\n", why, (unsigned long long)s_poll);
    fflush(NULL);
    exit(0);
}

void inputsync_exchange(const SmbNetPad local[INPUTSYNC_PORTS], SmbNetPad out[INPUTSYNC_PORTS])
{
    uint64_t p = s_poll;
    int i;

    if (s_max_polls && p >= s_max_polls)
        finish("--max-frames reached");

    switch (s_src) {
    case INPUTSYNC_RECORD: {
        uint8_t buf[REC_POLL_SIZE];
        SmbNetWriter w;
        smbw_init(&w, buf, sizeof buf);
        for (i = 0; i < INPUTSYNC_PORTS; i++)
            smbw_pad(&w, &local[i]);
        fwrite(buf, 1, sizeof buf, s_rec);
        if (p % 60 == 0)
            fflush(s_rec);
        if (out != local)
            memmove(out, local, INPUTSYNC_PORTS * sizeof(SmbNetPad));
        break;
    }
    case INPUTSYNC_REPLAY: {
        SmbNetReader r;
        if ((p + 1) * REC_POLL_SIZE > s_replay_size)
            finish("end of recording");
        r.data = s_replay_data + p * REC_POLL_SIZE;
        r.size = REC_POLL_SIZE; r.pos = 0; r.error = 0;
        for (i = 0; i < INPUTSYNC_PORTS; i++)
            smbr_pad(&r, &out[i]);
        break;
    }
    case INPUTSYNC_NETPLAY:
        if (!s_net_fn || s_net_fn((uint32_t)p, local, out, s_net_user) != 0)
            finish("netplay aborted");
        if (s_rec) {   /* --record with netplay: the bundle everyone played */
            uint8_t buf[REC_POLL_SIZE];
            SmbNetWriter w;
            smbw_init(&w, buf, sizeof buf);
            for (i = 0; i < INPUTSYNC_PORTS; i++)
                smbw_pad(&w, &out[i]);
            fwrite(buf, 1, sizeof buf, s_rec);
            if (p % 60 == 0)
                fflush(s_rec);
        }
        break;
    default:
        if (out != local)
            memmove(out, local, INPUTSYNC_PORTS * sizeof(SmbNetPad));
        break;
    }
    s_poll++;
}
