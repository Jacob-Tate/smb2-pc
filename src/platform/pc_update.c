/* In-game updater, network/model side (docs/updater.md, include/pc_update.h).
 *
 *  - Version: -DSMB2_VERSION (CMake option, installer/build.sh --version),
 *    compared as semver (1.2.3 > 1.2.3-rc1 > 1.2.2; a leading v is ignored).
 *  - Source: GET https://api.github.com/repos/<SMB2_UPDATE_REPO>/releases/latest
 *    (drafts and pre-releases are never "latest").  User-Agent
 *    "SMB2-PC/<version>", no other identifiers, no cookies.  Empty repo =
 *    compiled in but disabled (the default).
 *  - Background thread (WinHTTP), short timeouts; offline and every other
 *    failure is only logged.  The automatic check runs at most once a day
 *    (settings.dat update_last), the Updates screen has "Check now".
 *  - Update (installed mode only): SHA256SUMS.txt of the same release first
 *    (refused without it), then SMB2_PC_Setup_<ver>.exe into
 *    %TEMP%\SMB2_update\ with a running SHA-256; on a match Setup is started
 *    with ShellExecuteEx ("/UPDATE /WAITPID=<pid>": Windows shows the admin
 *    prompt, Setup waits for this process to exit) and the UI quits the game
 *    through pc_quit (which waits for memory card jobs).
 *
 * Test hook (no GitHub needed):
 *   SMB2_UPDATE_TEST=<url of a releases/latest JSON>  e.g. http://127.0.0.1:8765/latest
 *       enables the updater even without a repo, in automated runs too, and
 *       ignores the once-a-day limit.
 *   SMB2_UPDATE_TEMP=<windows dir>   download folder instead of %TEMP%\SMB2_update
 *   SMB2_UPDATE_DRYRUN=1             log instead of starting Setup / the browser
 */
#include "pc_update.h"
#include "pc_settings.h"
#include "install_mode.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <objbase.h>
#endif

#ifndef SMB2_VERSION_STR
#define SMB2_VERSION_STR "0.0.0-dev"
#endif
#ifndef SMB2_UPDATE_REPO_STR
#define SMB2_UPDATE_REPO_STR ""
#endif

#define API_MAX   (2 << 20)
#define SUMS_MAX  (64 << 10)
#define SETUP_MAX (512LL << 20)

const char* pc_update_current_version(void) { return SMB2_VERSION_STR; }
const char* pc_update_repo(void) { return SMB2_UPDATE_REPO_STR; }

static const char* test_url(void) {
    const char* t = getenv("SMB2_UPDATE_TEST");
    return (t && t[0]) ? t : NULL;
}
static int dry_run(void) {
    const char* d = getenv("SMB2_UPDATE_DRYRUN");
    return d && d[0] && d[0] != '0';
}

int pc_update_enabled(void) {
#ifdef _WIN32
    return test_url() != NULL || SMB2_UPDATE_REPO_STR[0] != 0;
#else
    return 0;
#endif
}
int pc_update_installed(void) { return pc_installed() ? 1 : 0; }

/* ------------------------------------------------------------------ semver */
static const char* skip_v(const char* s) {
    if (s && (s[0] == 'v' || s[0] == 'V') && isdigit((unsigned char)s[1])) s++;
    return s ? s : "";
}

int pc_update_vercmp(const char* a, const char* b) {
    const char *pa, *pb;
    int i;
    a = skip_v(a); b = skip_v(b);
    for (i = 0; i < 3; i++) {
        long x = strtol(a, (char**)&pa, 10), y = strtol(b, (char**)&pb, 10);
        if (x != y) return x < y ? -1 : 1;
        a = pa; b = pb;
        if (*a == '.') a++;
        if (*b == '.') b++;
    }
    while (isdigit((unsigned char)*a) || *a == '.') a++;   /* a 4th number is ignored */
    while (isdigit((unsigned char)*b) || *b == '.') b++;
    if (*a == '+') a += strlen(a);                      /* build metadata */
    if (*b == '+') b += strlen(b);
    /* a pre-release (1.2.0-rc1) is older than the release (1.2.0) */
    if (*a == '-' && *b != '-') return -1;
    if (*a != '-' && *b == '-') return 1;
    if (*a == '-' && *b == '-') {
        /* dot-separated identifiers: numeric ones compare as numbers */
        a++; b++;
        for (;;) {
            size_t la = strcspn(a, ".+"), lb = strcspn(b, ".+");
            int na = la > 0 && strspn(a, "0123456789") >= la, nb = lb > 0 && strspn(b, "0123456789") >= lb;
            int c;
            if (la == 0 && lb == 0) return 0;
            if (la == 0) return -1;
            if (lb == 0) return 1;
            if (na && nb) {
                long x = strtol(a, NULL, 10), y = strtol(b, NULL, 10);
                c = x < y ? -1 : x > y;
            } else if (na != nb) {
                c = na ? -1 : 1;
            } else {
                c = strncmp(a, b, la < lb ? la : lb);
                if (c == 0) c = la < lb ? -1 : la > lb;
            }
            if (c) return c < 0 ? -1 : 1;
            a += la; b += lb;
            if (*a == '.') a++;
            if (*b == '.') b++;
            if (*a == '+' || *a == 0) a += strlen(a);
            if (*b == '+' || *b == 0) b += strlen(b);
        }
    }
    return 0;
}

/* ---------------------------------------------------------- notes cleanup */
typedef struct { char* p; size_t n, cap; } Buf;
static void buf_putn(Buf* b, const char* s, size_t n) {
    if (b->n + n + 1 > b->cap) {
        size_t c = b->cap ? b->cap * 2 : 1024;
        char* q;
        while (c < b->n + n + 1) c *= 2;
        q = (char*)realloc(b->p, c);
        if (!q) return;
        b->p = q; b->cap = c;
    }
    memcpy(b->p + b->n, s, n);
    b->n += n;
    b->p[b->n] = 0;
}
static void buf_puts(Buf* b, const char* s) { buf_putn(b, s, strlen(s)); }
static void buf_putc(Buf* b, char c) { buf_putn(b, &c, 1); }

/* UTF-8 punctuation the game font has no glyph for -> ASCII */
static int typo_ascii(const unsigned char* s, Buf* o) {
    static const struct { const char* u; const char* a; } map[] = {
        { "\xE2\x80\x98", "'" }, { "\xE2\x80\x99", "'" }, { "\xE2\x80\x9C", "\"" }, { "\xE2\x80\x9D", "\"" },
        { "\xE2\x80\x93", "-" }, { "\xE2\x80\x94", "-" }, { "\xE2\x80\xA2", "-" }, { "\xE2\x80\xA6", "..." },
        { "\xE2\x86\x92", "->" }, { "\xC2\xA0", " " }, { "\xE2\x9C\x93", "*" }, { "\xE2\x9C\x94", "*" },
    };
    size_t i;
    for (i = 0; i < sizeof map / sizeof map[0]; i++) {
        size_t l = strlen(map[i].u);
        if (!strncmp((const char*)s, map[i].u, l)) { buf_puts(o, map[i].a); return (int)l; }
    }
    return 0;
}

/* inline Markdown of one line -> plain text */
static void md_inline(const char* s, size_t n, Buf* o) {
    size_t i = 0;
    while (i < n) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\\' && i + 1 < n && strchr("\\`*_{}[]()#+-.!<>|~", s[i + 1])) { buf_putc(o, s[i + 1]); i += 2; continue; }
        if (c == '`') { i++; continue; }
        if ((c == '*' || c == '_' || c == '~') && i + 1 < n && s[i + 1] == (char)c) { i += 2; continue; }
        if (c == '*') {
            /* emphasis marker: next to a word on one side only */
            int before = i > 0 && !isspace((unsigned char)s[i - 1]);
            int after = i + 1 < n && !isspace((unsigned char)s[i + 1]);
            if (before != after) { i++; continue; }
        }
        if (c == '!' && i + 1 < n && s[i + 1] == '[') { i++; continue; }   /* image: keep its alt text */
        if (c == '[') {
            /* [text](url) -> text */
            const char* close = memchr(s + i, ']', n - i);
            if (close && (size_t)(close - s) + 1 < n && close[1] == '(') {
                const char* end = memchr(close, ')', n - (close - s));
                if (end) {
                    md_inline(s + i + 1, (size_t)(close - (s + i + 1)), o);
                    i = (size_t)(end - s) + 1;
                    continue;
                }
            }
        }
        if (c == '<') {
            /* <https://...> autolinks keep the url; html tags / comments go */
            const char* end = memchr(s + i, '>', n - i);
            if (end) {
                size_t l = (size_t)(end - (s + i));
                if (l > 7 && (!strncmp(s + i + 1, "http://", 7) || !strncmp(s + i + 1, "https://", 8))) {
                    buf_putn(o, s + i + 1, l - 1);
                    i += l + 1;
                    continue;
                }
                if (l > 1 && (isalpha((unsigned char)s[i + 1]) || s[i + 1] == '/' || s[i + 1] == '!')) {
                    if (!strncmp(s + i + 1, "br", 2)) buf_putc(o, ' ');
                    i += l + 1;
                    continue;
                }
            }
        }
        if (c == '&') {
            static const struct { const char* e; char c; } ent[] = {
                { "&amp;", '&' }, { "&lt;", '<' }, { "&gt;", '>' }, { "&quot;", '"' }, { "&#39;", '\'' }, { "&nbsp;", ' ' } };
            size_t k;
            for (k = 0; k < sizeof ent / sizeof ent[0]; k++) {
                size_t l = strlen(ent[k].e);
                if (i + l <= n && !strncmp(s + i, ent[k].e, l)) { buf_putc(o, ent[k].c); i += l; break; }
            }
            if (k < sizeof ent / sizeof ent[0]) continue;
        }
        if (c == '\t') { buf_puts(o, "    "); i++; continue; }
        if (c >= 0x80) {
            int l = typo_ascii((const unsigned char*)s + i, o);
            if (l) { i += (size_t)l; continue; }
            if ((c & 0xF8) == 0xF0) {   /* 4-byte UTF-8 (emoji ...): no glyph */
                i += 4;
                continue;
            }
            if (c == 0xEF && i + 2 < n && (unsigned char)s[i + 1] == 0xB8 && (unsigned char)s[i + 2] == 0x8F) {
                i += 3;   /* variation selector */
                continue;
            }
        }
        if (c < 0x20) { i++; continue; }
        buf_putc(o, (char)c);
        i++;
    }
}

char* pc_update_plain_notes(const char* md) {
    Buf o = { 0 };
    const char* s = md ? md : "";
    int blank = 1, fence = 0;
    buf_puts(&o, "");
    while (*s) {
        const char* e = s + strcspn(s, "\r\n");
        const char* p = s;
        size_t n;
        int heading = 0, indent = 0;
        while (p < e && *p == ' ') { p++; indent++; }
        n = (size_t)(e - p);
        if (n >= 3 && (!strncmp(p, "```", 3) || !strncmp(p, "~~~", 3))) { fence = !fence; goto next; }
        if (!fence) {
            /* horizontal rule */
            {
                size_t k, cnt = 0;
                char rc = n ? p[0] : 0;
                for (k = 0; k < n && (rc == '-' || rc == '*' || rc == '_'); k++) {
                    if (p[k] == rc) cnt++;
                    else if (p[k] != ' ') break;
                }
                if (n && k == n && cnt >= 3) { if (!blank) { buf_putc(&o, '\n'); blank = 1; } goto next; }
            }
            while (n && *p == '>') { p++; n--; while (n && *p == ' ') { p++; n--; } }
            if (n && *p == '#') {
                while (n && *p == '#') { p++; n--; }
                while (n && *p == ' ') { p++; n--; }
                while (n && (p[n - 1] == '#' || p[n - 1] == ' ')) n--;
                heading = 1;
            } else if (n >= 2 && (*p == '-' || *p == '*' || *p == '+') && p[1] == ' ') {
                p += 2; n -= 2;
                if (n >= 4 && p[0] == '[' && p[2] == ']' && p[3] == ' ') {   /* task list */
                    buf_puts(&o, "");
                    p += 4; n -= 4;
                }
                {
                    int k;
                    for (k = 0; k < indent / 2 && k < 4; k++) buf_puts(&o, "  ");
                }
                buf_puts(&o, "- ");
                md_inline(p, n, &o);
                buf_putc(&o, '\n');
                blank = 0;
                goto next;
            }
        } else {
            int k;
            for (k = 0; k < indent; k++) buf_putc(&o, ' ');
        }
        while (n && p[n - 1] == ' ') n--;
        if (n == 0) {
            if (!blank) { buf_putc(&o, '\n'); blank = 1; }
            goto next;
        }
        if (heading) {
            if (!blank && o.n) buf_putc(&o, '\n');
            buf_putc(&o, '\x01');
        }
        if (fence) buf_putn(&o, p, n);
        else md_inline(p, n, &o);
        buf_putc(&o, '\n');
        blank = 0;
    next:
        s = e;
        if (*s == '\r') s++;
        if (*s == '\n') s++;
    }
    while (o.n && (o.p[o.n - 1] == '\n')) o.p[--o.n] = 0;
    return o.p ? o.p : calloc(1, 1);
}

/* ------------------------------------------------------------------ SHA-256 */
typedef struct { unsigned int h[8]; unsigned char b[64]; unsigned long long len; unsigned int nb; } Sha256;
static const unsigned int K256[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2 };
#define ROR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))
static void sha_block(Sha256* c, const unsigned char* p) {
    unsigned int w[64], a, b, d, e, f, g, h, cc, t1, t2;
    int i;
    for (i = 0; i < 16; i++) w[i] = (unsigned)p[i * 4] << 24 | (unsigned)p[i * 4 + 1] << 16 | (unsigned)p[i * 4 + 2] << 8 | p[i * 4 + 3];
    for (i = 16; i < 64; i++) {
        unsigned s0 = ROR(w[i - 15], 7) ^ ROR(w[i - 15], 18) ^ (w[i - 15] >> 3);
        unsigned s1 = ROR(w[i - 2], 17) ^ ROR(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    a = c->h[0]; b = c->h[1]; cc = c->h[2]; d = c->h[3]; e = c->h[4]; f = c->h[5]; g = c->h[6]; h = c->h[7];
    for (i = 0; i < 64; i++) {
        t1 = h + (ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25)) + ((e & f) ^ (~e & g)) + K256[i] + w[i];
        t2 = (ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22)) + ((a & b) ^ (a & cc) ^ (b & cc));
        h = g; g = f; f = e; e = d + t1; d = cc; cc = b; b = a; a = t1 + t2;
    }
    c->h[0] += a; c->h[1] += b; c->h[2] += cc; c->h[3] += d; c->h[4] += e; c->h[5] += f; c->h[6] += g; c->h[7] += h;
}
static void sha_init(Sha256* c) {
    static const unsigned int iv[8] = { 0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19 };
    memcpy(c->h, iv, sizeof iv);
    c->len = 0; c->nb = 0;
}
static void sha_update(Sha256* c, const void* data, size_t n) {
    const unsigned char* p = (const unsigned char*)data;
    c->len += n;
    while (n) {
        size_t k = 64 - c->nb;
        if (k > n) k = n;
        memcpy(c->b + c->nb, p, k);
        c->nb += (unsigned)k; p += k; n -= k;
        if (c->nb == 64) { sha_block(c, c->b); c->nb = 0; }
    }
}
static void sha_hex(Sha256* c, char out[65]) {
    unsigned long long bits = c->len * 8;
    unsigned char pad = 0x80, z = 0, lb[8];
    int i;
    sha_update(c, &pad, 1);
    while (c->nb != 56) sha_update(c, &z, 1);
    for (i = 0; i < 8; i++) lb[i] = (unsigned char)(bits >> (56 - 8 * i));
    sha_update(c, lb, 8);
    for (i = 0; i < 8; i++) sprintf(out + i * 8, "%08x", c->h[i]);
    out[64] = 0;
}

/* --------------------------------------------------------------- JSON (tiny) */
enum { J_NULL, J_BOOL, J_NUM, J_STR, J_ARR, J_OBJ };
typedef struct JVal {
    int type;
    double num;
    char* str;              /* J_STR (UTF-8) */
    int n;                  /* J_ARR / J_OBJ */
    char** keys;            /* J_OBJ */
    struct JVal* items;
} JVal;

static void j_free(JVal* v) {
    int i;
    if (!v) return;
    free(v->str);
    for (i = 0; i < v->n; i++) {
        if (v->keys) free(v->keys[i]);
        j_free(&v->items[i]);
    }
    free(v->keys);
    free(v->items);
    memset(v, 0, sizeof *v);
}

static const char* j_ws(const char* s) { while (*s == ' ' || *s == '\t' || *s == '\r' || *s == '\n') s++; return s; }

static void utf8_put(Buf* b, unsigned cp) {
    char t[4];
    if (cp < 0x80) { t[0] = (char)cp; buf_putn(b, t, 1); }
    else if (cp < 0x800) { t[0] = (char)(0xC0 | cp >> 6); t[1] = (char)(0x80 | (cp & 0x3F)); buf_putn(b, t, 2); }
    else if (cp < 0x10000) { t[0] = (char)(0xE0 | cp >> 12); t[1] = (char)(0x80 | ((cp >> 6) & 0x3F)); t[2] = (char)(0x80 | (cp & 0x3F)); buf_putn(b, t, 3); }
    else { t[0] = (char)(0xF0 | cp >> 18); t[1] = (char)(0x80 | ((cp >> 12) & 0x3F)); t[2] = (char)(0x80 | ((cp >> 6) & 0x3F)); t[3] = (char)(0x80 | (cp & 0x3F)); buf_putn(b, t, 4); }
}

static const char* j_str(const char* s, char** out) {
    Buf b = { 0 };
    buf_puts(&b, "");
    s++;   /* opening quote */
    while (*s && *s != '"') {
        if (*s == '\\') {
            s++;
            switch (*s) {
            case 'n': buf_putc(&b, '\n'); break;
            case 'r': buf_putc(&b, '\r'); break;
            case 't': buf_putc(&b, '\t'); break;
            case 'b': case 'f': break;
            case 'u': {
                unsigned cp = 0;
                int k;
                for (k = 1; k <= 4; k++) {
                    char c = s[k];
                    if (!isxdigit((unsigned char)c)) { free(b.p); return NULL; }
                    cp = cp * 16 + (unsigned)(isdigit((unsigned char)c) ? c - '0' : (tolower((unsigned char)c) - 'a' + 10));
                }
                s += 4;
                if (cp >= 0xD800 && cp < 0xDC00 && s[1] == '\\' && s[2] == 'u') {
                    unsigned lo = (unsigned)strtoul((char[5]){ s[3], s[4], s[5], s[6], 0 }, NULL, 16);
                    if (lo >= 0xDC00 && lo < 0xE000) { cp = 0x10000 + ((cp - 0xD800) << 10) + (lo - 0xDC00); s += 6; }
                }
                utf8_put(&b, cp);
                break;
            }
            case 0: free(b.p); return NULL;
            default: buf_putc(&b, *s); break;
            }
            s++;
        } else {
            buf_putc(&b, *s++);
        }
    }
    if (*s != '"') { free(b.p); return NULL; }
    *out = b.p;
    return s + 1;
}

static const char* j_parse(const char* s, JVal* v, int depth) {
    memset(v, 0, sizeof *v);
    s = j_ws(s);
    if (depth > 32) return NULL;
    if (*s == '"') { v->type = J_STR; return j_str(s, &v->str); }
    if (*s == '{' || *s == '[') {
        int obj = *s == '{';
        int cap = 0;
        v->type = obj ? J_OBJ : J_ARR;
        s = j_ws(s + 1);
        if (*s == (obj ? '}' : ']')) return s + 1;
        for (;;) {
            char* key = NULL;
            if (v->n == cap) {
                JVal* ni;
                cap = cap ? cap * 2 : 8;
                ni = (JVal*)realloc(v->items, sizeof(JVal) * (size_t)cap);
                if (!ni) return NULL;
                v->items = ni;
                if (obj) {
                    char** nk = (char**)realloc(v->keys, sizeof(char*) * (size_t)cap);
                    if (!nk) return NULL;
                    v->keys = nk;
                }
            }
            if (obj) {
                s = j_ws(s);
                if (*s != '"' || !(s = j_str(s, &key))) return NULL;
                s = j_ws(s);
                if (*s != ':') { free(key); return NULL; }
                s++;
                v->keys[v->n] = key;
            }
            s = j_parse(s, &v->items[v->n], depth + 1);
            v->n++;
            if (!s) return NULL;
            s = j_ws(s);
            if (*s == ',') { s++; continue; }
            if (*s == (obj ? '}' : ']')) return s + 1;
            return NULL;
        }
    }
    if (!strncmp(s, "true", 4)) { v->type = J_BOOL; v->num = 1; return s + 4; }
    if (!strncmp(s, "false", 5)) { v->type = J_BOOL; return s + 5; }
    if (!strncmp(s, "null", 4)) { v->type = J_NULL; return s + 4; }
    if (*s == '-' || isdigit((unsigned char)*s)) {
        char* e;
        v->type = J_NUM;
        v->num = strtod(s, &e);
        return e;
    }
    return NULL;
}

static JVal* j_get(JVal* o, const char* key) {
    int i;
    if (!o || o->type != J_OBJ) return NULL;
    for (i = 0; i < o->n; i++)
        if (!strcmp(o->keys[i], key)) return &o->items[i];
    return NULL;
}
static const char* j_gets(JVal* o, const char* key) {
    JVal* v = j_get(o, key);
    return (v && v->type == J_STR) ? v->str : NULL;
}

/* SHA256SUMS.txt ("<hex>  <name>" / "<hex> *<name>") -> hex of name */
static int find_sum(const char* sums, const char* name, char out[65]) {
    const char* s = sums;
    while (*s) {
        const char* e = s + strcspn(s, "\r\n");
        const char* p = s;
        int k;
        for (k = 0; k < 64 && isxdigit((unsigned char)p[k]); k++) {}
        if (k == 64 && (p[64] == ' ' || p[64] == '\t')) {
            const char* nm = p + 64;
            size_t l;
            while (nm < e && (*nm == ' ' || *nm == '\t' || *nm == '*')) nm++;
            l = (size_t)(e - nm);
            while (l && (nm[l - 1] == ' ' || nm[l - 1] == '\t')) l--;
            if (l == strlen(name) && !strncmp(nm, name, l)) {
                for (k = 0; k < 64; k++) out[k] = (char)tolower((unsigned char)p[k]);
                out[64] = 0;
                return 1;
            }
        }
        s = e;
        while (*s == '\r' || *s == '\n') s++;
    }
    return 0;
}

/* ------------------------------------------------------------------ state */
static PcuRelease s_rel;
static int s_have_rel;
static volatile int s_state = PCU_IDLE;
static volatile int s_err;
static volatile int s_cancel;
static volatile long long s_done, s_total;
static int s_auto;           /* the result belongs to the automatic check */
static int s_prompted;
static volatile long long s_mark_checked;   /* worker -> main thread: settings.dat update_last */
static char s_setup_path[1024];

int pc_update_state(void) { return s_state; }
int pc_update_error(void) { return s_err; }
const PcuRelease* pc_update_release(void) { return s_have_rel ? &s_rel : NULL; }
void pc_update_progress(long long* done, long long* total) {
    if (done) *done = s_done;
    if (total) *total = s_total;
}
void pc_update_cancel(void) { s_cancel = 1; }

int pc_update_should_prompt(void) {
    return s_auto && !s_prompted && s_state == PCU_AVAILABLE && s_have_rel &&
           strcmp(s_rel.version, g_pcs.update_skip) != 0;
}
void pc_update_prompt_shown(void) { s_prompted = 1; }

void pc_update_skip_release(void) {
    if (!s_have_rel) return;
    printf("[update] skipping version %s (remembered in settings.dat)\n", s_rel.version);
    fflush(stdout);
    pcs_set_update_skip(s_rel.version);
}

void pc_update_poll(void) {
    if (s_mark_checked) {
        long long t = s_mark_checked;
        s_mark_checked = 0;
        pcs_set_update_last(t);
    }
}

#ifdef _WIN32
/* ------------------------------------------------------------------ WinHTTP */
static CRITICAL_SECTION s_lock;
static int s_lock_init;
static HANDLE s_thread;

typedef int (*Sink)(void* ctx, const void* data, DWORD n);

static int winhttp_err(DWORD e) {
    switch (e) {
    case ERROR_WINHTTP_NAME_NOT_RESOLVED: case ERROR_WINHTTP_CANNOT_CONNECT:
    case ERROR_WINHTTP_TIMEOUT: case ERROR_WINHTTP_CONNECTION_ERROR:
    case ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR: case ERROR_WINHTTP_SECURE_FAILURE:
        return PCU_ERR_OFFLINE;
    }
    return PCU_ERR_DOWNLOAD;
}

static WCHAR* widen(const char* s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    WCHAR* w = n > 0 ? (WCHAR*)malloc(sizeof(WCHAR) * (size_t)n) : NULL;
    if (w) MultiByteToWideChar(CP_UTF8, 0, s, -1, w, n);
    return w;
}

/* GET url; body bytes go to sink.  Returns PCU_ERR_*; *status = HTTP status.
 * Redirects (GitHub asset downloads go to a CDN) are followed by WinHTTP. */
static int http_get(const char* url, int api, int recv_timeout_ms, Sink sink, void* ctx,
                    int* status, long long* clen) {
    WCHAR *wurl = widen(url), host[256], path[2048], ua[64];
    URL_COMPONENTS uc;
    HINTERNET ses = NULL, con = NULL, req = NULL;
    int ret = PCU_ERR_DOWNLOAD;
    DWORD st = 0, sz = sizeof st;
    long long got_total = 0;
    *status = 0;
    if (clen) *clen = -1;
    if (!wurl) return PCU_ERR_DOWNLOAD;
    memset(&uc, 0, sizeof uc);
    uc.dwStructSize = sizeof uc;
    uc.lpszHostName = host; uc.dwHostNameLength = 256;
    uc.lpszUrlPath = path; uc.dwUrlPathLength = 2048;
    {
        WCHAR extra[1024];
        uc.lpszExtraInfo = extra; uc.dwExtraInfoLength = 1024;
        if (!WinHttpCrackUrl(wurl, 0, 0, &uc)) { free(wurl); return PCU_ERR_BAD_REPLY; }
        if (uc.dwExtraInfoLength && wcslen(path) + wcslen(extra) < 2048) wcscat(path, extra);
    }
    free(wurl);
    wcscpy(ua, L"SMB2-PC/");
    MultiByteToWideChar(CP_UTF8, 0, SMB2_VERSION_STR, -1, ua + 8, 64 - 8);
    /* automatic proxy (Windows 8.1+), else the WinHTTP default proxy */
    ses = WinHttpOpen(ua, 4 /* WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY */, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) ses = WinHttpOpen(ua, WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!ses) return PCU_ERR_OFFLINE;
    {
        DWORD proto = 0x00000800 /* TLS 1.2 */ | 0x00002000 /* TLS 1.3 */;
        if (!WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &proto, sizeof proto)) {
            proto = 0x00000800;
            WinHttpSetOption(ses, WINHTTP_OPTION_SECURE_PROTOCOLS, &proto, sizeof proto);
        }
        /* never send or keep cookies */
        proto = WINHTTP_DISABLE_COOKIES;
        WinHttpSetOption(ses, WINHTTP_OPTION_DISABLE_FEATURE, &proto, sizeof proto);
    }
    WinHttpSetTimeouts(ses, 5000, 5000, 8000, recv_timeout_ms);
    con = WinHttpConnect(ses, host, uc.nPort, 0);
    if (!con) { ret = PCU_ERR_OFFLINE; goto done; }
    req = WinHttpOpenRequest(con, L"GET", path, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                             uc.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);
    if (!req) goto done;
    {
        const WCHAR* hdr = api ? L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2022-11-28\r\n"
                               : L"Accept: application/octet-stream\r\n";
        if (!WinHttpSendRequest(req, hdr, (DWORD)-1L, WINHTTP_NO_REQUEST_DATA, 0, 0, 0) ||
            !WinHttpReceiveResponse(req, NULL)) {
            ret = winhttp_err(GetLastError());
            printf("[update] %s: WinHTTP error %lu\n", url, GetLastError());
            goto done;
        }
    }
    WinHttpQueryHeaders(req, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX,
                        &st, &sz, WINHTTP_NO_HEADER_INDEX);
    *status = (int)st;
    if (clen) {
        WCHAR cl[32];
        DWORD cls = sizeof cl;
        if (WinHttpQueryHeaders(req, WINHTTP_QUERY_CONTENT_LENGTH, WINHTTP_HEADER_NAME_BY_INDEX, cl, &cls,
                                WINHTTP_NO_HEADER_INDEX))
            *clen = _wcstoi64(cl, NULL, 10);
    }
    if (st != 200) { ret = PCU_ERR_NONE; goto done; }   /* caller looks at the status */
    got_total = 0;
    for (;;) {
        char data[16384];
        DWORD got = 0;
        if (s_cancel) { ret = PCU_ERR_CANCELLED; goto done; }
        if (!WinHttpReadData(req, data, sizeof data, &got)) {
            ret = winhttp_err(GetLastError());
            if (ret == PCU_ERR_OFFLINE) ret = PCU_ERR_DOWNLOAD;   /* connection lost mid-way */
            printf("[update] %s: read error %lu\n", url, GetLastError());
            goto done;
        }
        if (got == 0) break;
        got_total += got;
        if (sink(ctx, data, got) != 0) { ret = s_cancel ? PCU_ERR_CANCELLED : PCU_ERR_WRITE; goto done; }
    }
    if (clen && *clen >= 0 && got_total != *clen) {
        /* the server closed the connection early */
        printf("[update] %s: connection closed after %lld of %lld bytes\n", url, got_total, *clen);
        ret = PCU_ERR_DOWNLOAD;
        goto done;
    }
    ret = PCU_ERR_NONE;
done:
    if (req) WinHttpCloseHandle(req);
    if (con) WinHttpCloseHandle(con);
    if (ses) WinHttpCloseHandle(ses);
    return ret;
}

typedef struct { Buf b; size_t max; } MemSink;
static int mem_sink(void* ctx, const void* data, DWORD n) {
    MemSink* m = (MemSink*)ctx;
    if (m->b.n + n > m->max) return 1;
    buf_putn(&m->b, (const char*)data, n);
    return 0;
}

static void set_state(int st, int err) {
    EnterCriticalSection(&s_lock);
    s_err = err;
    s_state = st;
    LeaveCriticalSection(&s_lock);
}

/* ---------------------------------------------------------------- check job */
static int parse_release(const char* json, PcuRelease* r) {
    JVal root;
    const char *tag, *s;
    JVal* assets;
    int i;
    char want[96];
    if (!j_parse(json, &root, 0) || root.type != J_OBJ) { j_free(&root); return PCU_ERR_BAD_REPLY; }
    tag = j_gets(&root, "tag_name");
    if (!tag || !isdigit((unsigned char)*skip_v(tag)) || strlen(tag) >= sizeof r->tag) { j_free(&root); return PCU_ERR_BAD_REPLY; }
    memset(r, 0, sizeof *r);
    snprintf(r->tag, sizeof r->tag, "%s", tag);
    snprintf(r->version, sizeof r->version, "%s", skip_v(tag));
    for (i = 0; r->version[i]; i++)   /* used in a file name below */
        if (!isalnum((unsigned char)r->version[i]) && !strchr(".-+", r->version[i])) { j_free(&root); return PCU_ERR_BAD_REPLY; }
    snprintf(r->name, sizeof r->name, "%s", (s = j_gets(&root, "name")) ? s : "");
    snprintf(r->html_url, sizeof r->html_url, "%s", (s = j_gets(&root, "html_url")) ? s : "");
    s = j_gets(&root, "body");
    r->body = strdup(s ? s : "");
    snprintf(want, sizeof want, "SMB2_PC_Setup_%s.exe", r->version);
    assets = j_get(&root, "assets");
    for (i = 0; assets && assets->type == J_ARR && i < assets->n; i++) {
        JVal* a = &assets->items[i];
        const char* nm = j_gets(a, "name");
        const char* u = j_gets(a, "browser_download_url");
        JVal* sz = j_get(a, "size");
        if (!nm || !u) continue;
        if (!strcmp(nm, want)) {
            snprintf(r->setup_name, sizeof r->setup_name, "%s", nm);
            snprintf(r->setup_url, sizeof r->setup_url, "%s", u);
            r->setup_size = (sz && sz->type == J_NUM) ? (long long)sz->num : -1;
        } else if (!strcmp(nm, "SHA256SUMS.txt")) {
            snprintf(r->sums_url, sizeof r->sums_url, "%s", u);
        }
    }
    j_free(&root);
    return PCU_ERR_NONE;
}

static char s_check_url[512];

static DWORD WINAPI check_thread(LPVOID arg) {
    MemSink m = { { 0 }, API_MAX };
    int status = 0, err;
    PcuRelease r;
    (void)arg;
    printf("[update] checking %s\n", s_check_url);
    fflush(stdout);
    err = http_get(s_check_url, 1, 10000, mem_sink, &m, &status, NULL);
    if (err == PCU_ERR_NONE && status == 404) err = PCU_ERR_NO_RELEASE;
    else if (err == PCU_ERR_NONE && status != 200) err = PCU_ERR_HTTP;
    if (err == PCU_ERR_NONE || err == PCU_ERR_NO_RELEASE || err == PCU_ERR_HTTP)
        s_mark_checked = (long long)time(NULL);   /* the server answered: counts as today's check */
    if (err == PCU_ERR_NONE) {
        buf_puts(&m.b, "");
        err = parse_release(m.b.p, &r);
    }
    free(m.b.p);
    if (err != PCU_ERR_NONE) {
        printf("[update] check failed: error %d (HTTP %d)\n", err, status);
        fflush(stdout);
        set_state(err == PCU_ERR_NO_RELEASE ? PCU_UP_TO_DATE : PCU_CHECK_FAILED, err);
        return 0;
    }
    EnterCriticalSection(&s_lock);
    free(s_rel.body);
    s_rel = r;
    s_have_rel = 1;
    LeaveCriticalSection(&s_lock);
    if (pc_update_vercmp(r.version, SMB2_VERSION_STR) > 0) {
        printf("[update] version %s available (this is %s): %s; setup %s (%lld bytes), checksums %s\n",
               r.version, SMB2_VERSION_STR, r.html_url, r.setup_name[0] ? r.setup_name : "-",
               r.setup_size, r.sums_url[0] ? "yes" : "no");
        set_state(PCU_AVAILABLE, 0);
    } else {
        printf("[update] up to date (latest release %s, this is %s)\n", r.version, SMB2_VERSION_STR);
        set_state(PCU_UP_TO_DATE, 0);
    }
    fflush(stdout);
    return 0;
}

static int busy(void) {
    int st = s_state;
    return st == PCU_CHECKING || st == PCU_DOWNLOADING || st == PCU_VERIFYING || st == PCU_LAUNCHING ||
           st == PCU_LAUNCHED;
}

static void start_check(int automatic) {
    const char* t = test_url();
    if (!s_lock_init) { InitializeCriticalSection(&s_lock); s_lock_init = 1; }
    if (busy()) return;
    if (!pc_update_enabled()) { set_state(PCU_CHECK_FAILED, PCU_ERR_DISABLED); return; }
    if (t) snprintf(s_check_url, sizeof s_check_url, "%s", t);
    else snprintf(s_check_url, sizeof s_check_url, "https://api.github.com/repos/%s/releases/latest", SMB2_UPDATE_REPO_STR);
    s_auto = automatic;
    s_cancel = 0;
    set_state(PCU_CHECKING, 0);
    if (s_thread) { CloseHandle(s_thread); s_thread = NULL; }
    s_thread = CreateThread(NULL, 0, check_thread, NULL, 0, NULL);
    if (!s_thread) set_state(PCU_CHECK_FAILED, PCU_ERR_OFFLINE);
}

void pc_update_check_now(void) { start_check(0); }

void pc_update_startup(int automated) {
    long long now = (long long)time(NULL), last = g_pcs.update_last;
    const char* t = test_url();
    if (!s_lock_init) { InitializeCriticalSection(&s_lock); s_lock_init = 1; }
    printf("[update] SMB2 %s (%s); update source: %s\n", SMB2_VERSION_STR,
           pc_installed() ? "installed" : "portable",
           t ? t : SMB2_UPDATE_REPO_STR[0] ? SMB2_UPDATE_REPO_STR : "none (updater disabled in this build)");
    if (g_pcs.update_skip[0]) printf("[update] skipped version: %s\n", g_pcs.update_skip);
    fflush(stdout);
    if (!pc_update_enabled() || !g_pcs.update_check) return;
    if (automated && !t) return;   /* scripted test runs: never (unless the test hook asks) */
    if (!t && last > 0 && last <= now && now - last < 24 * 3600) {
        printf("[update] last check %lld s ago - next automatic check after 24 h\n", now - last);
        fflush(stdout);
        return;
    }
    start_check(1);
}

/* ------------------------------------------------------------- download job */
typedef struct { HANDLE f; Sha256 sha; long long n, max; } FileSink;
static int file_sink(void* ctx, const void* data, DWORD n) {
    FileSink* fs = (FileSink*)ctx;
    DWORD w = 0;
    if (s_cancel) return 1;
    if (fs->n + n > fs->max) return 1;
    if (!WriteFile(fs->f, data, n, &w, NULL) || w != n) return 1;
    sha_update(&fs->sha, data, n);
    fs->n += n;
    s_done = fs->n;
    return 0;
}

static int launch_setup(void) {
    SHELLEXECUTEINFOW sei;
    WCHAR params[64], *wpath;
    if (dry_run()) {
        printf("[update] DRY RUN: would start \"%s\" /UPDATE /WAITPID=%lu\n", s_setup_path, (unsigned long)GetCurrentProcessId());
        fflush(stdout);
        return PCU_ERR_NONE;
    }
    wpath = widen(s_setup_path);
    if (!wpath) return PCU_ERR_LAUNCH;
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
    {
        char p8[64];
        snprintf(p8, sizeof p8, "/UPDATE /WAITPID=%lu", (unsigned long)GetCurrentProcessId());
        MultiByteToWideChar(CP_UTF8, 0, p8, -1, params, 64);
    }
    memset(&sei, 0, sizeof sei);
    sei.cbSize = sizeof sei;
    sei.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_NOASYNC;
    /* test hook only: a test download folder on a network path (WSL) would
     * stop at the "Open File - Security Warning" box.  Real downloads go to
     * the local %TEMP% (WinHTTP sets no Mark of the Web), so they keep the
     * normal zone checks. */
    {
        const char* t = getenv("SMB2_UPDATE_TEMP");
        if (t && t[0]) sei.fMask |= 0x00800000 /* SEE_MASK_NOZONECHECKS */;
    }
    sei.lpVerb = L"open";   /* the setup's manifest asks for admin: Windows shows the prompt */
    sei.lpFile = wpath;
    sei.lpParameters = params;
    sei.nShow = SW_SHOWNORMAL;
    printf("[update] starting %s /UPDATE /WAITPID=%lu\n", s_setup_path, (unsigned long)GetCurrentProcessId());
    fflush(stdout);
    if (!ShellExecuteExW(&sei)) {
        DWORD e = GetLastError();
        free(wpath);
        printf("[update] Setup did not start (error %lu)\n", e);
        fflush(stdout);
        return e == ERROR_CANCELLED ? PCU_ERR_LAUNCH_DENIED : PCU_ERR_LAUNCH;
    }
    free(wpath);
    if (sei.hProcess) CloseHandle(sei.hProcess);
    return PCU_ERR_NONE;
}

static DWORD WINAPI download_thread(LPVOID arg) {
    PcuRelease r;
    MemSink m = { { 0 }, SUMS_MAX };
    char want[65], got[65];
    WCHAR dir[MAX_PATH + 64], wpart[MAX_PATH + 160], wfinal[MAX_PATH + 160];
    const char* envdir = getenv("SMB2_UPDATE_TEMP");
    FileSink fs;
    int status = 0, err;
    long long clen = -1;
    (void)arg;
    EnterCriticalSection(&s_lock);
    r = s_rel;   /* body pointer shared, read-only here */
    LeaveCriticalSection(&s_lock);

    /* 1. checksums */
    if (!r.sums_url[0]) { printf("[update] release %s has no SHA256SUMS.txt - refusing to install\n", r.version); set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_NO_SUMS); return 0; }
    s_done = 0; s_total = r.setup_size > 0 ? r.setup_size : 0;
    err = http_get(r.sums_url, 0, 15000, mem_sink, &m, &status, NULL);
    if (err == PCU_ERR_NONE && status != 200) err = PCU_ERR_NO_SUMS;
    if (err == PCU_ERR_NONE) {
        buf_puts(&m.b, "");
        if (!find_sum(m.b.p, r.setup_name, want)) err = PCU_ERR_NO_SUMS;
    }
    free(m.b.p);
    if (err != PCU_ERR_NONE) {
        printf("[update] checksums: error %d (HTTP %d)%s\n", err, status,
               err == PCU_ERR_NO_SUMS ? " - SHA256SUMS.txt missing or does not list the setup; refusing" : "");
        set_state(PCU_DOWNLOAD_FAILED, err);
        return 0;
    }
    printf("[update] expected SHA-256 of %s: %s\n", r.setup_name, want);

    /* 2. the setup, into %TEMP%\SMB2_update\ */
    if (envdir && envdir[0]) {
        MultiByteToWideChar(CP_UTF8, 0, envdir, -1, dir, MAX_PATH);
        if (dir[wcslen(dir) - 1] != L'\\') wcscat(dir, L"\\");
    } else {
        DWORD n = GetTempPathW(MAX_PATH, dir);
        if (n == 0 || n > MAX_PATH) { set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_WRITE); return 0; }
        wcscat(dir, L"SMB2_update\\");
    }
    CreateDirectoryW(dir, NULL);
    wcscpy(wfinal, dir);
    MultiByteToWideChar(CP_UTF8, 0, r.setup_name, -1, wfinal + wcslen(wfinal), 150);
    wcscpy(wpart, wfinal);
    wcscat(wpart, L".part");
    WideCharToMultiByte(CP_UTF8, 0, wfinal, -1, s_setup_path, sizeof s_setup_path, NULL, NULL);
    memset(&fs, 0, sizeof fs);
    fs.max = SETUP_MAX;
    sha_init(&fs.sha);
    fs.f = CreateFileW(wpart, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (fs.f == INVALID_HANDLE_VALUE) {
        printf("[update] cannot write %s.part\n", s_setup_path);
        set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_WRITE);
        return 0;
    }
    printf("[update] downloading %s -> %s\n", r.setup_url, s_setup_path);
    fflush(stdout);
    err = http_get(r.setup_url, 0, 30000, file_sink, &fs, &status, &clen);
    CloseHandle(fs.f);
    if (err == PCU_ERR_NONE && status != 200) err = PCU_ERR_DOWNLOAD;
    if (err != PCU_ERR_NONE) {
        printf("[update] download failed: error %d (HTTP %d) after %lld bytes\n", err, status, fs.n);
        DeleteFileW(wpart);
        set_state(PCU_DOWNLOAD_FAILED, err);
        return 0;
    }
    /* 3. verify */
    set_state(PCU_VERIFYING, 0);
    sha_hex(&fs.sha, got);
    printf("[update] downloaded %lld bytes, SHA-256 %s\n", fs.n, got);
    if (strcmp(got, want) != 0 || (r.setup_size > 0 && fs.n != r.setup_size)) {
        printf("[update] SHA-256 / size MISMATCH (expected %s, %lld bytes) - deleting the download, not installing\n",
               want, r.setup_size);
        fflush(stdout);
        DeleteFileW(wpart);
        set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_MISMATCH);
        return 0;
    }
    if (!MoveFileExW(wpart, wfinal, MOVEFILE_REPLACE_EXISTING)) {
        DeleteFileW(wpart);
        set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_WRITE);
        return 0;
    }
    printf("[update] checksum OK\n");
    /* 4. start Setup (admin prompt); the UI quits the game when it is up */
    set_state(PCU_LAUNCHING, 0);
    err = launch_setup();
    set_state(err == PCU_ERR_NONE ? PCU_LAUNCHED : PCU_LAUNCH_FAILED, err);
    fflush(stdout);
    return 0;
}

void pc_update_download_start(void) {
    if (!s_lock_init) { InitializeCriticalSection(&s_lock); s_lock_init = 1; }
    if (busy() || !s_have_rel) return;
    if (!pc_installed()) { set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_LAUNCH); return; }
    if (!s_rel.setup_name[0]) {
        printf("[update] release %s has no SMB2_PC_Setup_%s.exe asset\n", s_rel.version, s_rel.version);
        set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_NO_ASSET);
        return;
    }
    s_cancel = 0;
    s_done = 0;
    s_total = s_rel.setup_size > 0 ? s_rel.setup_size : 0;
    set_state(PCU_DOWNLOADING, 0);
    if (s_thread) { CloseHandle(s_thread); s_thread = NULL; }
    s_thread = CreateThread(NULL, 0, download_thread, NULL, 0, NULL);
    if (!s_thread) set_state(PCU_DOWNLOAD_FAILED, PCU_ERR_DOWNLOAD);
}

int pc_update_open_page(void) {
    const char* u = s_have_rel ? s_rel.html_url : "";
    WCHAR* w;
    int ok;
    if (strncmp(u, "https://", 8) != 0 && !(test_url() && strncmp(u, "http://", 7) == 0)) {
        printf("[update] no usable release page URL (%s)\n", u);
        return 0;
    }
    if (dry_run()) {
        printf("[update] DRY RUN: would open %s\n", u);
        fflush(stdout);
        return 1;
    }
    w = widen(u);
    if (!w) return 0;
    ok = (INT_PTR)ShellExecuteW(NULL, L"open", w, NULL, NULL, SW_SHOWNORMAL) > 32;
    free(w);
    printf("[update] opened %s in the browser: %s\n", u, ok ? "ok" : "failed");
    fflush(stdout);
    return ok;
}

#else /* !_WIN32: the updater needs WinHTTP; compiled in but never active */
void pc_update_startup(int automated) {
    (void)automated;
    printf("[update] SMB2 %s (updater: Windows only)\n", SMB2_VERSION_STR);
}
void pc_update_check_now(void) { s_err = PCU_ERR_DISABLED; s_state = PCU_CHECK_FAILED; }
void pc_update_download_start(void) {}
int pc_update_open_page(void) { return 0; }
#endif
