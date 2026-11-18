/* State checksum bookkeeping (E3); see checksum.h. */
#include "netplay/checksum.h"
#include "netplay/session.h"
#include <stdio.h>
#include <stdlib.h>

extern unsigned long long platform_get_time_ns(void);

static FILE *s_log;
static int s_ring_on;
static uint32_t s_latest_poll, s_latest_hash;
static uint64_t s_latest_regions[CHECKSUM_REGIONS];
static uint32_t s_ring_poll[CHECKSUM_RING], s_ring_hash[CHECKSUM_RING];
static uint64_t s_calls, s_total_ns, s_max_ns;
static int s_atexit_set;

static void report_cost(void)
{
    uint64_t n;
    double avg, mx;
    checksum_get_cost(&n, &avg, &mx);
    if (n)
        printf("[checksum] %llu hash calls, avg %.2f us, max %.2f us\n", (unsigned long long)n, avg, mx);
    if (s_log) { fclose(s_log); s_log = NULL; }
}

void checksum_open_log(const char *path)
{
    if (!path || !*path)
        return;
    s_log = fopen(path, "w");
    if (!s_log)
        fprintf(stderr, "cannot open checksum log %s\n", path);
    if (!s_atexit_set) { atexit(report_cost); s_atexit_set = 1; }
}

void checksum_enable_ring(int on) { s_ring_on = on; }

static uint32_t timed_hash(uint64_t regions[CHECKSUM_REGIONS])
{
    unsigned long long t0 = platform_get_time_ns();
    uint64_t h = pc_state_hash(regions);
    unsigned long long dt = platform_get_time_ns() - t0;
    s_calls++;
    s_total_ns += dt;
    if (dt > s_max_ns) s_max_ns = dt;
    return pch_fold32(h);
}

void checksum_on_poll(uint64_t poll)
{
    int periodic = poll % CHECKSUM_PERIOD == 0;
    uint64_t regions[CHECKSUM_REGIONS];
    uint32_t h;

    if (!periodic && !s_ring_on)
        return;
    if (!s_atexit_set) { atexit(report_cost); s_atexit_set = 1; }
    h = timed_hash(regions);
    if (s_ring_on) {
        s_ring_poll[poll % CHECKSUM_RING] = (uint32_t)poll;
        s_ring_hash[poll % CHECKSUM_RING] = h;
    }
    if (periodic) {
        s_latest_poll = (uint32_t)poll;
        s_latest_hash = h;
        memcpy(s_latest_regions, regions, sizeof regions);
        if (s_log) {
            fprintf(s_log, "poll %llu hash %08X\n", (unsigned long long)poll, (unsigned)h);
            fflush(s_log);
        }
        printf("[checksum] poll %llu hash %08X\n", (unsigned long long)poll, (unsigned)h);
    }
}

uint32_t checksum_latest(uint32_t *hash_out)
{
    if (hash_out) *hash_out = s_latest_hash;
    return s_latest_poll;
}

int checksum_ring_get(uint32_t poll, uint32_t *hash_out)
{
    if (!s_ring_on || s_ring_poll[poll % CHECKSUM_RING] != poll)
        return 0;
    if (hash_out) *hash_out = s_ring_hash[poll % CHECKSUM_RING];
    return 1;
}

const uint64_t *checksum_latest_regions(void) { return s_latest_regions; }

void checksum_get_cost(uint64_t *calls, double *avg_us, double *max_us)
{
    if (calls) *calls = s_calls;
    if (avg_us) *avg_us = s_calls ? (double)s_total_ns / (double)s_calls / 1000.0 : 0.0;
    if (max_us) *max_us = (double)s_max_ns / 1000.0;
}
