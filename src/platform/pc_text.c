/* PC-only text in the game's encoding: see include/pc_text.h. */
#include "pc_text.h"
#include "pc_settings.h"
#include <string.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#endif

/* cp1252 0x80..0x9F -> Unicode */
static const unsigned short s_cp1252_hi[32] = {
    0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
    0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178,
};

static int latin_byte(unsigned cp) {
    int i;
    if (cp >= 0xA0 && cp <= 0xFF) return (int)cp;
    for (i = 0; i < 32; i++)
        if (s_cp1252_hi[i] && s_cp1252_hi[i] == cp) return 0x80 + i;
    return -1;
}

void pc_text_game_buf(const char* in, char* out, int outsize) {
    const unsigned char* s = (const unsigned char*)(in ? in : "");
    int n = 0;
    while (*s && n < outsize - 3) {
        unsigned cp;
        int lb;
        if (*s < 0x80) { out[n++] = (char)*s++; continue; }
        if ((*s & 0xE0) == 0xC0 && s[1]) { cp = ((s[0] & 0x1F) << 6) | (s[1] & 0x3F); s += 2; }
        else if ((*s & 0xF0) == 0xE0 && s[1] && s[2]) {
            cp = ((s[0] & 0x0F) << 12) | ((s[1] & 0x3F) << 6) | (s[2] & 0x3F); s += 3;
        } else { s++; out[n++] = '?'; continue; }
        lb = latin_byte(cp);
        if (lb >= 0) {
            out[n++] = (char)0x86;
            out[n++] = (char)lb;
            continue;
        }
#ifdef _WIN32
        {
            WCHAR w = (WCHAR)cp;
            char mb[4];
            BOOL bad = FALSE;
            int k = WideCharToMultiByte(932, 0, &w, 1, mb, sizeof mb, NULL, &bad);
            if (k == 2 && !bad) { out[n++] = mb[0]; out[n++] = mb[1]; continue; }
        }
#endif
        out[n++] = '?';
    }
    out[n] = 0;
}

const char* pc_text_game(const char* utf8) {
    static struct { const char* in; char* out; } cache[512];
    static int count;
    int i;
    char buf[1024];
    if (!utf8) return "";
    for (i = 0; i < count; i++)
        if (cache[i].in == utf8) return cache[i].out;
    pc_text_game_buf(utf8, buf, sizeof buf);
    if (count < (int)(sizeof cache / sizeof cache[0])) {
        char* o = (char*)malloc(strlen(buf) + 1);
        if (!o) return "";
        strcpy(o, buf);
        cache[count].in = utf8;
        cache[count].out = o;
        count++;
        return o;
    }
    {   /* cache full (not expected): rotating buffers */
        static char ring[4][1024];
        static int r;
        r = (r + 1) & 3;
        memcpy(ring[r], buf, sizeof buf);
        return ring[r];
    }
}

const char* pc_text_pick(const char* const tbl[6]) {
    int l = pcs_active_locale();
    const char* s;
    if (l < 0 || l > 5) l = 0;
    s = tbl[l];
    if (!s || !s[0]) s = tbl[0];
    return pc_text_game(s);
}
