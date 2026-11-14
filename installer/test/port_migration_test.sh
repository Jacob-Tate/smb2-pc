#!/bin/bash
# Port-side test of the 1.0.4 name migration (install_mode.c, migrate_old_names):
# in installed mode (install.ini) the pre-1.0.4 folders "%APPDATA%\SMB2 PC Port"
# and "%LOCALAPPDATA%\SMB2 PC Port\gamedata" move to the new userdata and
# recovery-copy folders.  Runs the real exe on Windows through WSL interop.
#
#   installer/test/port_migration_test.sh <smb2_pc_port.exe> <test root (Linux path)>
#
# Everything lives below the test root: install.ini points userdata= there and
# the "old" folders are given with SMB2_PC_TEST_OLD_USERDATA /
# SMB2_PC_TEST_OLD_LOCAL_GAMEDATA (then the exe never looks at the real
# %APPDATA%/%LOCALAPPDATA%).  The disc image ($SMB2_TEST_ISO, a Windows
# path) is only read; $SMB2_TEST_MEMCARD is a memcard/slotA folder with saves.
set -u
EXE="$(realpath "$1")"
ROOT="$(realpath -m "$2")"
case "$ROOT" in
    /|"$HOME"|"$HOME"/|/mnt/?|/mnt/?/) echo "refusing test root $ROOT" >&2; exit 2;;
esac
# a dedicated folder: empty on first use, then marked with .smb2_test_root
mkdir -p "$ROOT"
if [ ! -e "$ROOT/.smb2_test_root" ]; then
    [ -z "$(ls -A "$ROOT")" ] || { echo "refusing non-empty test root without .smb2_test_root: $ROOT" >&2; exit 2; }
    : > "$ROOT/.smb2_test_root"
fi
case "$ROOT" in */pc_port|*/pc_port/*|*/decomp|*/decomp/*) echo "refusing test root inside a source checkout: $ROOT" >&2; exit 2;; esac
ISO="${SMB2_TEST_ISO:?set SMB2_TEST_ISO to the disc image (Windows path)}"
MEMCARD_SRC="${SMB2_TEST_MEMCARD:?set SMB2_TEST_MEMCARD to a memcard/slotA folder with saves}"
WROOT="$(wslpath -w "$ROOT")"
BIN="$ROOT/bin"
OLD_UD="$ROOT/AppData/SMB2 PC Port"
NEW_UD="$ROOT/AppData/SMB2/installed"
OLD_LGD="$ROOT/LocalAppData/SMB2 PC Port/gamedata"
NEW_LGD="$ROOT/LocalAppData/SMB2/gamedata"
PASS=0 FAIL=0
ok()   { echo "  PASS: $*"; PASS=$((PASS+1)); }
bad()  { echo "  FAIL: $*"; FAIL=$((FAIL+1)); }
check() { local d="$1"; shift; if "$@"; then ok "$d"; else bad "$d"; fi; }
not() { ! "$@"; }

clean() {
    for d in bin AppData LocalAppData run; do
        [ -e "$ROOT/$d" ] && rm -rf -- "${ROOT:?}/$d"
    done
    return 0
}
prepare() { # [portable]
    clean
    mkdir -p "$BIN" "$ROOT/run"
    cp "$EXE" "$BIN/smb2_pc_port.exe"; chmod +x "$BIN/smb2_pc_port.exe"
    cp -r "$(dirname "$EXE")/assets" "$BIN/assets"
    if [ "${1:-}" != portable ]; then
        printf 'mode=installed\r\ngamedata=gamedata\r\nuserdata=%s\r\nversion=test\r\n' \
            "$WROOT\\AppData\\SMB2\\installed" > "$BIN/install.ini"
    fi
    # old (pre-1.0.4) userdata: settings with recognisable values + the memory card
    mkdir -p "$OLD_UD/memcard/slotA" "$OLD_UD/logs"
    printf 'SMB2PC 1\nwidth=800\nheight=600\ndisplay=windowed\nvsync=0\nmsaa=0\naspect=auto\nvolume=37\nrenderer=gl\n' > "$OLD_UD/settings.dat"
    cp "$MEMCARD_SRC"/* "$OLD_UD/memcard/slotA/"
    head -c 8192 /dev/zero > "$OLD_UD/memcard/slotA/migration_probe.bin"   # 1 sector, no .meta: card_shim logs it when it lists the card
    echo old-log > "$OLD_UD/logs/smb2_pc_port.log"
    # old recovery copy of the game data (only the manifest matters here)
    mkdir -p "$OLD_LGD/files"
    printf 'SMB2PC-GAMEDATA 1\ndisc_id=GM2E8P\n' > "$OLD_LGD/manifest.txt"
    echo data > "$OLD_LGD/files/marker.bin"
}
run() { # env-mode -> $ROOT/run/game.log
    local envs=SMB2_PC_LAUNCHER
    export SMB2_PC_LAUNCHER="${LAUNCHER:-1}" SMB2_PC_LOCAL_GAMEDATA="$WROOT\\LocalAppData\\SMB2\\gamedata"
    envs="$envs:SMB2_PC_LOCAL_GAMEDATA"
    if [ "$1" = test-old ]; then
        export SMB2_PC_TEST_OLD_USERDATA="$WROOT\\AppData\\SMB2 PC Port"
        export SMB2_PC_TEST_OLD_LOCAL_GAMEDATA="$WROOT\\LocalAppData\\SMB2 PC Port\\gamedata"
        envs="$envs:SMB2_PC_TEST_OLD_USERDATA:SMB2_PC_TEST_OLD_LOCAL_GAMEDATA"
    else
        unset SMB2_PC_TEST_OLD_USERDATA SMB2_PC_TEST_OLD_LOCAL_GAMEDATA
    fi
    shift
    ( cd "$ROOT/run" && WSLENV="$envs" timeout 180 "$BIN/smb2_pc_port.exe" --iso "$ISO" \
        --exit-frame "${EXITF:-400}" --audio-null "$@" > "$ROOT/run/game.log" 2>&1 )
    RC=$?
}
grepf() { grep -qF -- "$1" "$ROOT/run/game.log"; }

echo "== 1. installed mode, new userdata missing: old folders are moved (rename) and used"
prepare
# real menus up to the memory-card check (tools/x64test/modes.sh "memcard")
LAUNCHER=0 EXITF=2801 run test-old --input-script "1350:START,1450:START,1550:START,1700:A,1900:A,2100:A"
check "game exit 0" [ "$RC" = 0 ]
check "logged: userdata migrated" grepf "[install] migrated saves and settings: moved $WROOT\\AppData\\SMB2 PC Port -> $WROOT\\AppData\\SMB2\\installed"
check "old userdata folder gone" [ ! -e "$OLD_UD" ]
check "settings.dat moved" [ -f "$NEW_UD/settings.dat" ]
check "memory card moved" [ -f "$NEW_UD/memcard/slotA/super_monkey_ball_2.dat.bin" -a -f "$NEW_UD/memcard/slotA/super_monkey_ball_2.dat.meta" ]
check "settings loaded from the new folder (800x600, volume 37)" grep -qE "\[settings\] loaded .*AppData.SMB2.installed.settings\.dat: 800x600 .*vol=37" "$ROOT/run/game.log"
check "memory card listed from the new folder" grepf "[card] migration_probe.bin has no usable .meta"
check "game opened its save on the migrated card" grepf "[card] CARDOpen(0, super_monkey_ball_2.dat) -> 0"
check "logged: recovery copy migrated" grepf "[install] migrated the game data recovery copy: moved $WROOT\\LocalAppData\\SMB2 PC Port\\gamedata -> $WROOT\\LocalAppData\\SMB2\\gamedata"
check "recovery copy moved" [ -f "$NEW_LGD/manifest.txt" -a -f "$NEW_LGD/files/marker.bin" -a ! -e "$OLD_LGD" ]
check "old LocalAppData parent left alone in test mode (only real paths are pruned)" [ -d "$ROOT/LocalAppData/SMB2 PC Port" ]
echo "   second start: nothing left to migrate"
run test-old
check "game exit 0" [ "$RC" = 0 ]
check "no migration logged" not grepf "[install] migrated"
check "settings still loaded from the new folder" grep -qE "\[settings\] loaded .*AppData.SMB2.installed.settings\.dat: 800x600" "$ROOT/run/game.log"

echo "== 2. new userdata exists without saves/settings (only logs\\): merged by copy"
prepare
mkdir -p "$NEW_UD/logs"; echo new-log > "$NEW_UD/logs/smb2_pc_port.log"
run test-old
check "game exit 0" [ "$RC" = 0 ]
check "logged: copied, one file kept in the old folder" grepf "[install] migrated saves and settings by copying $WROOT\\AppData\\SMB2 PC Port -> $WROOT\\AppData\\SMB2\\installed; 1 file(s) could not be moved"
check "settings + memory card copied" [ -f "$NEW_UD/settings.dat" -a -f "$NEW_UD/memcard/slotA/super_monkey_ball_2.dat.bin" ]
check "copied files deleted from the old folder" [ ! -e "$OLD_UD/settings.dat" -a ! -e "$OLD_UD/memcard" ]
check "existing new file not overwritten; old copy kept" [ "$(cat "$NEW_UD/logs/smb2_pc_port.log")" = new-log -a "$(cat "$OLD_UD/logs/smb2_pc_port.log")" = old-log ]
check "settings loaded from the new folder" grep -qE "\[settings\] loaded .*AppData.SMB2.installed.settings\.dat: 800x600" "$ROOT/run/game.log"

echo "== 3. new userdata already has settings: old folder left alone"
prepare
mkdir -p "$NEW_UD"; printf 'SMB2PC 1\nwidth=1024\nheight=768\nvolume=55\n' > "$NEW_UD/settings.dat"
run test-old
check "game exit 0" [ "$RC" = 0 ]
check "logged: not migrating" grepf "[install] not migrating $WROOT\\AppData\\SMB2 PC Port"
check "old folder kept" [ -f "$OLD_UD/settings.dat" -a -f "$OLD_UD/memcard/slotA/super_monkey_ball_2.dat.bin" ]
check "new settings used" grep -qE "\[settings\] loaded .*AppData.SMB2.installed.settings\.dat: 1024x768" "$ROOT/run/game.log"

echo "== 4. custom userdata= without the test variables: real %APPDATA% folders never looked at"
prepare
run real
check "game exit 0" [ "$RC" = 0 ]
check "no migration" not grepf "[install] migrated"
check "no 'not migrating' either" not grepf "[install] not migrating"
check "test-root old folder untouched" [ -f "$OLD_UD/settings.dat" ]

echo "== 5. portable mode (no install.ini): unchanged, no migration"
prepare portable
mkdir -p "$ROOT/run/save"
run test-old --save-dir "$WROOT\\run\\save"
check "game exit 0" [ "$RC" = 0 ]
check "not installed mode" not grepf "[install] installed mode"
check "no migration" not grepf "[install] migrated"
check "old folders untouched" [ -f "$OLD_UD/settings.dat" -a -f "$OLD_LGD/manifest.txt" ]
check "new folders not created" [ ! -e "$ROOT/AppData/SMB2" -a ! -e "$ROOT/LocalAppData/SMB2" ]
check "settings from --save-dir" grep -qE "\[settings\] (no|loaded) .*run.save.settings\.dat" "$ROOT/run/game.log"

echo "== 6. installed mode + --save-dir: no migration"
prepare
mkdir -p "$ROOT/run/save"
run test-old --save-dir "$WROOT\\run\\save"
check "game exit 0" [ "$RC" = 0 ]
check "no migration" not grepf "[install] migrated"
check "old folder untouched" [ -f "$OLD_UD/settings.dat" ]

clean
echo "RESULT: $PASS passed, $FAIL failed"
[ $FAIL = 0 ]
