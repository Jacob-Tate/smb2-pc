/* Test double for smb2_pc_port.exe's installer CLI (docs/installer.md).
 * Used only by installer/test/run_tests.sh to exercise every installer path
 * (progress, exit codes 2/3/4, verify/reuse) without a real disc image.
 *   --extract-iso <iso> --gamedata <dir> [--progress]
 *       iso name contains "wrong" -> exit 2, "corrupt" -> 3, "diskfull" -> 4,
 *       otherwise writes <dir>\manifest.txt + icon.ico (a copy of the installed smb2.ico) via <dir>.partial.
 *   --verify-gamedata <dir>   -> 0 if <dir>\manifest.txt starts with the port's magic line,
 *       else 6; also 6 if the manifest has a line "bad=1", or "badcopy=1" and <dir> ends
 *       in ".partial" (a copy that does not verify).
 * Every invocation is appended to fake_port_calls.log next to the exe, and to
 * %SMB2_FAKE_LOG% if set. */
#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAGIC "SMB2PC-GAMEDATA 1"   /* first manifest line, as in gamedata.c */

static void log_to(const char *p, int argc, char **argv) {
    FILE *f = fopen(p, "a"); if (!f) return;
    for (int i = 1; i < argc; i++) fprintf(f, "[%s] ", argv[i]);
    fprintf(f, "\n"); fclose(f);
}
static void log_call(int argc, char **argv) {
    char p[MAX_PATH]; GetModuleFileNameA(NULL, p, MAX_PATH);
    char *s = strrchr(p, '\\'); if (s) strcpy(s + 1, "fake_port_calls.log");
    log_to(p, argc, argv);
    /* also calls of the copy Setup unpacks to $PLUGINSDIR for --verify-gamedata */
    const char *all = getenv("SMB2_FAKE_LOG");
    if (all && *all) log_to(all, argc, argv);
}

static int fail(int code, const char *msg) {
    printf("RESULT ERROR %d %s\n", code, msg); fflush(stdout); return code;
}

int main(int argc, char **argv) {
    const char *iso = NULL, *gd = NULL, *verify = NULL; int progress = 0;
    log_call(argc, argv);
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--extract-iso") && i + 1 < argc) iso = argv[++i];
        else if (!strcmp(argv[i], "--gamedata") && i + 1 < argc) gd = argv[++i];
        else if (!strcmp(argv[i], "--verify-gamedata") && i + 1 < argc) verify = argv[++i];
        else if (!strcmp(argv[i], "--progress")) progress = 1;
    }
    if (verify) {
        char m[MAX_PATH], line[256]; int first = 1, ok = 0;
        size_t vl = strlen(verify);
        int is_partial = vl >= 8 && !_stricmp(verify + vl - 8, ".partial");
        snprintf(m, sizeof m, "%s\\manifest.txt", verify);
        FILE *mf = fopen(m, "r");
        if (!mf) return fail(6, "manifest missing");
        while (fgets(line, sizeof line, mf)) {
            line[strcspn(line, "\r\n")] = 0;
            if (first) { ok = !strcmp(line, MAGIC); first = 0; }
            if (!strcmp(line, "bad=1") || (is_partial && !strcmp(line, "badcopy=1"))) ok = 0;
        }
        fclose(mf);
        if (!ok) return fail(6, "game data does not verify");
        printf("RESULT OK\n"); return 0;
    }
    if (!iso) { printf("fake smb2_pc_port (installer test double)\n"); Sleep(1500); return 0; }
    if (!gd) return fail(5, "no --gamedata");
    if (strstr(iso, "wrong")) return fail(2, "This is not Super Monkey Ball 2 (USA): disc ID GM2P8P");
    if (strstr(iso, "corrupt")) return fail(3, "Cannot read the disc image (bad FST)");
    const long long total = 1459978240LL;
    for (int i = 0; i <= 40; i++) {
        if (strstr(iso, "diskfull") && i == 20) return fail(4, "Disk full while writing game data");
        if (progress) { printf("PROGRESS %lld %lld\n", total * i / 40, total); fflush(stdout); }
        Sleep(75);
    }
    char part[MAX_PATH], f[MAX_PATH];
    snprintf(part, sizeof part, "%s.partial", gd);
    CreateDirectoryA(part, NULL);
    snprintf(f, sizeof f, "%s\\manifest.txt", part);
    FILE *m = fopen(f, "w"); if (!m) return fail(4, "cannot write");
    fprintf(m, MAGIC "\ndisc_id=GM2E8P\nrev=0\nfake=1\n"); fclose(m);
    snprintf(f, sizeof f, "%s\\icon.ico", part);
    char src[MAX_PATH]; GetModuleFileNameA(NULL, src, MAX_PATH);
    char *s = strrchr(src, '\\'); if (s) strcpy(s + 1, "smb2.ico");
    CopyFileA(src, f, FALSE);
    /* replace old dir only after the new one is complete */
    char old[MAX_PATH]; snprintf(old, sizeof old, "%s.old", gd);
    if (GetFileAttributesA(gd) != INVALID_FILE_ATTRIBUTES) {
        MoveFileA(gd, old);
        snprintf(f, sizeof f, "%s\\manifest.txt", old); DeleteFileA(f);
        snprintf(f, sizeof f, "%s\\icon.ico", old); DeleteFileA(f);
        RemoveDirectoryA(old);
    }
    if (!MoveFileA(part, gd)) return fail(4, "rename failed");
    printf("RESULT OK\n"); fflush(stdout); return 0;
}
