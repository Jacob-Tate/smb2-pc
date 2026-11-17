/* Disc image reader: backend picked by file magic. */
#include "disc_reader.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

struct DiscReader {
    const char* format;
    int64_t size;
    bool (*read)(DiscReader*, uint64_t, uint32_t, void*);
    void (*close)(DiscReader*);
    FILE* file;                 /* plain backend */
};

static void set_err(char* err, size_t errsz, const char* fmt, const char* a, const char* b)
{
    if (err && errsz) snprintf(err, errsz, fmt, a, b);
}

/* ---- plain image backend ------------------------------------------------ */
static int seek64(FILE* f, int64_t off, int whence)
{
#ifdef _WIN32
    return _fseeki64(f, off, whence);
#else
    return fseeko(f, (off_t)off, whence);
#endif
}

static int64_t tell64(FILE* f)
{
#ifdef _WIN32
    return _ftelli64(f);
#else
    return (int64_t)ftello(f);
#endif
}

static bool plain_read(DiscReader* r, uint64_t off, uint32_t len, void* dst)
{
    if (off > (uint64_t)r->size || len > (uint64_t)r->size - off) return false;
    if (len == 0) return true;
    if (seek64(r->file, (int64_t)off, SEEK_SET) != 0) return false;
    return fread(dst, 1, len, r->file) == len;
}

static void plain_close(DiscReader* r)
{
    if (r->file) fclose(r->file);
}

DiscReader* disc_reader_open(const char* path, char* err, size_t errsz)
{
    uint8_t magic[4] = {0, 0, 0, 0};
    DiscReader* r;
    FILE* f;
    if (err && errsz) err[0] = 0;
    if (!path || !*path) { set_err(err, errsz, "%s%s", "no disc image path", ""); return NULL; }
    f = fopen(path, "rb");
    if (!f) { set_err(err, errsz, "cannot open %s: %s", path, strerror(errno)); return NULL; }
    if (fread(magic, 1, 4, f) != 4) memset(magic, 0, 4);

    if (!memcmp(magic, "RVZ\x01", 4)) {
        fclose(f);
        set_err(err, errsz, "%s is an RVZ image; RVZ is not supported yet (convert it to ISO)%s", path, "");
        return NULL;
    }

    r = calloc(1, sizeof *r);
    if (!r) { fclose(f); set_err(err, errsz, "%s%s", "out of memory", ""); return NULL; }
    if (seek64(f, 0, SEEK_END) != 0 || (r->size = tell64(f)) < 0) {
        free(r);
        fclose(f);
        set_err(err, errsz, "cannot determine the size of %s%s", path, "");
        return NULL;
    }
    r->format = "iso";
    r->file = f;
    r->read = plain_read;
    r->close = plain_close;
    return r;
}

int64_t disc_reader_size(DiscReader* r) { return r ? r->size : -1; }

bool disc_reader_read(DiscReader* r, uint64_t offset, uint32_t len, void* dst)
{
    if (!r || (!dst && len)) return false;
    return r->read(r, offset, len, dst);
}

void disc_reader_close(DiscReader* r)
{
    if (!r) return;
    if (r->close) r->close(r);
    free(r);
}

const char* disc_reader_format(DiscReader* r) { return r ? r->format : ""; }
