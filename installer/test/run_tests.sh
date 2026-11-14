#!/bin/bash
# Automated installer tests (TESTMODE build; runs on Windows through WSL interop).
#
#   installer/test/run_tests.sh <TESTMODE setup.exe> <test root (Linux path)> [real]
#
# The TESTMODE installer only writes below the test root and to
# HKCU\Software\SMB2Test (and the old name's HKCU\Software\SMB2PCPortTest, which it
# upgrades); this script deletes them before and after.  SMB2_OLD_SETUP = a TESTMODE
# setup built from the pre-1.0.4 smb2.nsi (old name), for the upgrade tests.
# Without "real", the setup must be built with test/fake_port.c as the game exe
# (build.sh --exe32 fake32.exe --exe64 fake64.exe --test-root ...): it covers
# every exit-code path.  With "real" (setup built from the real port exes), it
# runs one extraction from $SMB2_TEST_ISO (the disc image as a Windows path,
# only read), checks the result and launches the installed game for a frame dump.
set -u
SETUP="$(realpath "$1")"
ROOT="$(realpath -m "$2")"
MODE="${3:-fake}"
# everything below the test root is deleted: only allow a dedicated folder
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
WROOT="$(wslpath -w "$ROOT")"
INST="$ROOT/PF/SMB2"; WINST="$WROOT\\PF\\SMB2"
GD="$INST/gamedata"; WGD="$WINST\\gamedata"
LEG="$ROOT/ProgramData/SMB2 PC Port/gamedata"   # 1.0.0 layout
UD="$ROOT/AppData/SMB2/installed"               # userdata (1.0.4+)
OLDUD="$ROOT/AppData/SMB2 PC Port"              # userdata before 1.0.4 (the game moves it)
OLDINST="$ROOT/PF/SMB2 PC Port"; WOLDINST="$WROOT\\PF\\SMB2 PC Port"   # install under the old name
OLD_SETUP="${SMB2_OLD_SETUP:-}"                 # TESTMODE setup built from the pre-1.0.4 smb2.nsi
MAGIC='SMB2PC-GAMEDATA 1'
# fake_port.c also logs here (incl. the --verify-gamedata copy Setup runs from $PLUGINSDIR)
FLOG="$ROOT/fake_calls_all.log"
KEY='HKCU\Software\SMB2Test'
OLDKEY='HKCU\Software\SMB2PCPortTest'          # TESTMODE keys of the old name
PASS=0 FAIL=0
ok()   { echo "  PASS: $*"; PASS=$((PASS+1)); }
bad()  { echo "  FAIL: $*"; FAIL=$((FAIL+1)); }
check() { local d="$1"; shift; if "$@"; then ok "$d"; else bad "$d"; fi; }
not() { ! "$@"; }
has_file() { compgen -G "$1" >/dev/null; }
no_key() { ! reg.exe query "$KEY" >/dev/null 2>&1; }
grepq() { grep -q -- "$1" "$2" 2>/dev/null; }
grepf() { grep -qF -- "$1" "$2" 2>/dev/null; }   # fixed string (Windows paths)

clean() {
    for d in PF "Program Files" ProgramData AppData LocalAppData StartMenu Desktop; do
        [ -e "$ROOT/$d" ] && rm -rf -- "${ROOT:?}/$d"
    done
    reg.exe delete "$KEY" /f >/dev/null 2>&1
    reg.exe delete "$OLDKEY" /f >/dev/null 2>&1
    return 0
}
setup() { # args...  -> sets RC
    "$SETUP" /S "/TESTROOT=$WROOT" "$@" "/D=$WINST"
    RC=$?
}
# exact Windows command line (WSL interop would quote "/ISO=D:\a b\c.iso" as a whole,
# which NSIS's GetOptions does not parse); CreateProcess, no ShellExecute zone prompt
setup_cmdline() { # "<args string>"  -> sets RC
    powershell.exe -NoProfile -Command "\$i=New-Object Diagnostics.ProcessStartInfo '$(wslpath -w "$SETUP")', '/S /TESTROOT=$WROOT $1 /D=$WINST'; \$i.UseShellExecute=\$false; \$p=[Diagnostics.Process]::Start(\$i); \$p.WaitForExit(); exit \$p.ExitCode"
    RC=$?
}
uninstall() { # args...
    cp "$INST/uninstall.exe" "$ROOT/un_copy.exe"; chmod +x "$ROOT/un_copy.exe"
    "$ROOT/un_copy.exe" /S "$@" "_?=$WINST"; RC=$?
    rm -f -- "$ROOT/un_copy.exe"
}
lnk_icon() { powershell.exe -NoProfile -Command "(New-Object -ComObject WScript.Shell).CreateShortcut('$(wslpath -w "$1")').IconLocation" | tr -d '\r'; }
lnk_target() { powershell.exe -NoProfile -Command "\$s=(New-Object -ComObject WScript.Shell).CreateShortcut('$(wslpath -w "$1")'); \$s.TargetPath + '|' + \$s.WorkingDirectory" | tr -d '\r'; }
regval() { reg.exe query "$KEY$1" /v "$2" 2>/dev/null | tr -d '\r' | awk -v n="$2" '$1==n {sub(/^ *[^ ]+ +[^ ]+ +/,""); print}'; }
calls() { cat "$INST/fake_port_calls.log" 2>/dev/null; }
# fake 1.0.0 layout: game data in %ProgramData% (+ install.ini pointing there)
make_legacy() { # extra manifest lines...
    mkdir -p "$LEG/files"
    { echo "$MAGIC"; echo "disc_id=GM2E8P"; for l in "$@"; do echo "$l"; done; } > "$LEG/manifest.txt"
    cp "$INST/smb2.ico" "$LEG/icon.ico" 2>/dev/null || echo ico > "$LEG/icon.ico"
    echo legacy-data > "$LEG/files/legacy_marker.bin"
    if [ -f "$INST/install.ini" ]; then
        printf 'mode=installed\r\ngamedata=%s\r\nuserdata=%s\r\nversion=1.0.0\r\n' \
            "$(wslpath -w "$LEG")" "$WROOT\\AppData\\SMB2 PC Port" > "$INST/install.ini"
    fi
}
ini_get() { tr -d '\r' < "$INST/install.ini" | sed -n "s/^$1=//p"; }
# install under the old name with the pre-1.0.4 TESTMODE setup (keys under $OLDKEY)
# run <exe> with this exact Windows command line (paths with spaces in /D=,
# which must be last and unquoted; WSL interop would quote the whole argument)
run_cmdline() { # exe "<args string>"  -> sets RC
    powershell.exe -NoProfile -Command "\$i=New-Object Diagnostics.ProcessStartInfo '$(wslpath -w "$1")', '$2'; \$i.UseShellExecute=\$false; \$p=[Diagnostics.Process]::Start(\$i); \$p.WaitForExit(); exit \$p.ExitCode"
    RC=$?
}
old_setup() { # dir(Windows) args...  -> sets RC
    local d="$1"; shift
    run_cmdline "$OLD_SETUP" "/S /TESTROOT=$WROOT $* /D=$d"
    rm -f -- "$(wslpath -u "$d")/fake_port_calls.log"   # (test double's own log)
}
old_key() { reg.exe query "$OLDKEY" >/dev/null 2>&1; }
new_setup_nod() { # args...  (no /D=: Setup picks the folder)
    "$SETUP" /S "/TESTROOT=$WROOT" "$@"
    RC=$?
}

mkdir -p "$ROOT/isos"
clean
: > "$FLOG"
export SMB2_FAKE_LOG="$(wslpath -w "$FLOG")"
export WSLENV="SMB2_FAKE_LOG${WSLENV:+:$WSLENV}"

if [ "$MODE" = real ]; then
    ISO="${SMB2_TEST_ISO:?set SMB2_TEST_ISO to the disc image (Windows path)}"
    echo "== real: silent install + extraction from $ISO"
    start=$(date +%s)
    setup_cmdline "/ISO=\"$ISO\""
    echo "  setup exit $RC after $(( $(date +%s) - start ))s"
    check "exit code 0" [ "$RC" = 0 ]
    check "manifest.txt written" [ -f "$GD/manifest.txt" ]
    check "icon.ico generated from disc" [ -f "$GD/icon.ico" ]
    check "no .partial left" [ ! -e "$GD.partial" ]
    check "shortcut uses disc icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$(wslpath -w "$GD")\\icon.ico,0" ]
    check "DisplayIcon uses disc icon" [ "$(regval '\Uninstall' DisplayIcon)" = "$(wslpath -w "$GD")\\icon.ico" ]
    chmod +x "$INST/smb2_pc_port.exe"
    "$INST/smb2_pc_port.exe" --verify-gamedata "$(wslpath -w "$GD")" >/dev/null; check "--verify-gamedata OK" [ $? = 0 ]
    check "install.ini gamedata=gamedata" [ "$(ini_get gamedata)" = gamedata ]
    check "nothing in the old %ProgramData% location" [ ! -e "$ROOT/ProgramData" ]
    echo "== real: upgrade from the 1.0.0 layout (game data in %ProgramData%) moves it"
    mkdir -p "$(dirname "$LEG")"; mv -- "$GD" "$LEG"
    printf 'mode=installed\r\ngamedata=%s\r\nuserdata=%s\r\nversion=1.0.0\r\n' \
        "$(wslpath -w "$LEG")" "$WROOT\\AppData\\SMB2 PC Port" > "$INST/install.ini"
    before=$(stat -c %Y "$LEG/manifest.txt")
    start=$(date +%s); setup_cmdline ""; echo "  setup exit $RC after $(( $(date +%s) - start ))s"
    check "exit 0" [ "$RC" = 0 ]
    check "moved into INSTDIR\\gamedata (not re-extracted)" [ -f "$GD/manifest.txt" -a "$(stat -c %Y "$GD/manifest.txt")" = "$before" ]
    check "old %ProgramData% folder removed" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
    check "install.ini gamedata=gamedata again" [ "$(ini_get gamedata)" = gamedata ]
    check "shortcut uses disc icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$(wslpath -w "$GD")\\icon.ico,0" ]
    "$INST/smb2_pc_port.exe" --verify-gamedata "$(wslpath -w "$GD")" >/dev/null; check "moved data: --verify-gamedata OK" [ $? = 0 ]
    echo "== real: launch the installed game (install.ini mode) and dump a frame"
    mkdir -p "$ROOT/run" "$ROOT/AppData"
    ( cd "$ROOT/run" && SMB2_PC_LAUNCHER=1 WSLENV=SMB2_PC_LAUNCHER timeout 180 "$INST/smb2_pc_port.exe" \
        --dump-interval 300 --exit-frame 301 --audio-null > "$ROOT/run/game.log" 2>&1 )
    echo "  game exit $?"
    ls "$ROOT/run" "$ROOT/AppData" 2>/dev/null | head
    check "frame dumped" has_file "$ROOT/run/frame_00300.ppm"
    check "ran from the installed game data" grep -q "Loaded game data folder: $(wslpath -w "$GD" | sed 's/\\/\\\\/g')" "$ROOT/run/game.log"
    echo "== real: reinstall reuses verified game data"
    before=$(stat -c %Y "$GD/manifest.txt")
    start=$(date +%s); setup_cmdline "/ISO=\"$ISO\""; echo "  setup exit $RC after $(( $(date +%s) - start ))s"
    check "exit 0" [ "$RC" = 0 ]
    check "game data not re-extracted" [ -n "$before" -a "$(stat -c %Y "$GD/manifest.txt")" = "$before" ]
    echo "== real: uninstall"
    uninstall
    check "game data removed" [ ! -e "$GD" ]
    check "program removed" [ ! -e "$INST/smb2_pc_port.exe" ]
    check "install folder removed" [ ! -e "$INST" ]
    check "registry removed" no_key
    clean
    if [ -n "$OLD_SETUP" ]; then
        OLD_SETUP="$(realpath "$OLD_SETUP")"
        echo "== real: upgrade from an \"SMB2 PC Port\" install (pre-1.0.4 setup, same exes)"
        start=$(date +%s); old_setup "$WOLDINST" "/ISO=\"$ISO\""; echo "  old setup exit $RC after $(( $(date +%s) - start ))s"
        check "old setup: exit 0" [ "$RC" = 0 ]
        check "old setup: game data extracted" [ -f "$OLDINST/gamedata/manifest.txt" -a -f "$ROOT/StartMenu/SMB2 PC Port.lnk" ]
        check "old setup: old key" old_key
        before=$(stat -c %Y "$OLDINST/gamedata/manifest.txt")
        start=$(date +%s); setup_cmdline ""; echo "  setup exit $RC after $(( $(date +%s) - start ))s"
        check "exit 0" [ "$RC" = 0 ]
        check "game data moved (not re-extracted)" [ -f "$GD/manifest.txt" -a "$(stat -c %Y "$GD/manifest.txt")" = "$before" ]
        chmod +x "$INST/smb2_pc_port.exe"
        "$INST/smb2_pc_port.exe" --verify-gamedata "$(wslpath -w "$GD")" >/dev/null; check "moved data: --verify-gamedata OK" [ $? = 0 ]
        check "old install removed completely" [ ! -e "$OLDINST" ]
        check "old shortcut removed, new one uses the disc icon" [ ! -e "$ROOT/StartMenu/SMB2 PC Port.lnk" -a "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WGD\\icon.ico,0" ]
        check "one ARP entry" not old_key
        check "ARP DisplayName SMB2" [ "$(regval '\Uninstall' DisplayName)" = SMB2 ]
        check "installed exe: VERSIONINFO FileDescription SMB2" [ "$(powershell.exe -NoProfile -Command "(Get-Item '$WINST\\smb2_pc_port.exe').VersionInfo.FileDescription" | tr -d '\r')" = SMB2 ]
        rm -rf -- "${ROOT:?}/run"; mkdir -p "$ROOT/run"
        ( cd "$ROOT/run" && SMB2_PC_LAUNCHER=1 WSLENV=SMB2_PC_LAUNCHER timeout 180 "$INST/smb2_pc_port.exe" \
            --dump-interval 300 --exit-frame 301 --audio-null > "$ROOT/run/game.log" 2>&1 )
        check "game runs from the moved game data" grep -q "Loaded game data folder: $(wslpath -w "$GD" | sed 's/\\/\\\\/g')" "$ROOT/run/game.log"
        check "frame dumped" has_file "$ROOT/run/frame_00300.ppm"
        uninstall
        check "uninstall: install folder removed" [ ! -e "$INST" ]
        check "uninstall: registry removed" no_key
        clean
    else
        bad "real old-name upgrade not run (SMB2_OLD_SETUP not set)"
    fi
    rm -rf -- "${ROOT:?}/run" "${ROOT:?}/isos"; rm -f -- "$FLOG"
    echo "RESULT: $PASS passed, $FAIL failed"; [ $FAIL = 0 ]; exit
fi

for n in good wrong_game corrupt diskfull; do printf 'GM2E8P01' > "$ROOT/isos/$n.iso"; head -c 2000 /dev/zero >> "$ROOT/isos/$n.iso"; done
printf 'GM2P8P01' > "$ROOT/isos/pal.iso"; head -c 100 /dev/zero >> "$ROOT/isos/pal.iso"
WI="$WROOT\\isos"

echo "== 1. fresh install, good image"
setup "/ISO=$WI\\good.iso"
check "exit 0" [ "$RC" = 0 ]
for f in smb2_pc_port.exe install.ini README.txt LICENSE.txt smb2.ico uninstall.exe assets/prompts/xbox; do
    check "installed $f" [ -e "$INST/$f" ]
done
check "no stray files from the source tree" [ "$(ls "$INST" | tr '\n' ' ')" = "LICENSE.txt README.txt assets fake_port_calls.log gamedata install.ini smb2.ico smb2_pc_port.exe uninstall.exe " ]
check "install.ini mode" [ "$(ini_get mode)" = installed ]
check "install.ini gamedata=gamedata (relative to the exe folder)" [ "$(ini_get gamedata)" = gamedata ]
check "install.ini userdata" [ "$(ini_get userdata)" = "$WROOT\\AppData\\SMB2\\installed" ]
check "install.ini version" [ -n "$(ini_get version)" ]
check "install.ini has no BOM" [ "$(head -c 3 "$INST/install.ini" | od -An -tx1 | tr -d ' ')" != efbbbf ]
check "extract called with --progress" grep -q -- '\[--extract-iso\] .*good.iso\] \[--gamedata\] .*gamedata\] \[--progress\]' "$INST/fake_port_calls.log"
check "game data present in the install folder" [ -f "$GD/manifest.txt" ]
check "nothing written to the old %ProgramData% location" [ ! -e "$ROOT/ProgramData" ]
check "Start Menu shortcut" [ -f "$ROOT/StartMenu/SMB2.lnk" ]
check "desktop shortcut" [ -f "$ROOT/Desktop/SMB2.lnk" ]
check "shortcut target + working dir" [ "$(lnk_target "$ROOT/StartMenu/SMB2.lnk")" = "$WINST\\smb2_pc_port.exe|$WINST" ]
check "shortcut icon = gamedata\\icon.ico" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$(wslpath -w "$GD")\\icon.ico,0" ]
check "desktop icon = gamedata\\icon.ico" [ "$(lnk_icon "$ROOT/Desktop/SMB2.lnk")" = "$(wslpath -w "$GD")\\icon.ico,0" ]
check "ARP DisplayName" [ "$(regval '\Uninstall' DisplayName)" = "SMB2" ]
check "ARP Comments mention the unofficial port" grep -q "Unofficial PC port" <<<"$(regval '\Uninstall' Comments)"
check "ARP DisplayVersion" [ -n "$(regval '\Uninstall' DisplayVersion)" ]
check "ARP Publisher" [ -n "$(regval '\Uninstall' Publisher)" ]
check "ARP DisplayIcon = gamedata icon" [ "$(regval '\Uninstall' DisplayIcon)" = "$(wslpath -w "$GD")\\icon.ico" ]
check "ARP EstimatedSize includes the game data" [ $(( $(regval '\Uninstall' EstimatedSize) )) -gt 1013300 ]
check "ARP UninstallString" [ "$(regval '\Uninstall' UninstallString)" = "\"$WINST\\uninstall.exe\"" ]
check "arch x86_64 on 64-bit Windows" [ "$(regval '' Arch)" = x86_64 ]

echo "== 2. reinstall over it: verified data is reused (no re-extraction)"
: > "$INST/fake_port_calls.log"
before=$(stat -c %Y "$GD/manifest.txt"); sleep 1
mkdir -p "$INST/assets/stale_dir"; touch "$INST/assets/stale_dir/old.png"
setup "/ISO=$WI\\good.iso"
check "exit 0" [ "$RC" = 0 ]
check "game data untouched (verified by the new exe from \$PLUGINSDIR)" [ "$(stat -c %Y "$GD/manifest.txt")" = "$before" ]
check "no --extract-iso" not grep -q -- '--extract-iso' "$INST/fake_port_calls.log"
check "stale asset removed on upgrade" [ ! -e "$INST/assets/stale_dir" ]
check "shortcut still uses disc icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$(wslpath -w "$GD")\\icon.ico,0" ]

echo "== 3. /REEXTRACT forces extraction"
: > "$INST/fake_port_calls.log"
setup "/ISO=$WI\\good.iso" /REEXTRACT
check "exit 0" [ "$RC" = 0 ]
check "--extract-iso called" grep -q -- '--extract-iso' "$INST/fake_port_calls.log"
check "game data present" [ -f "$GD/manifest.txt" ]

echo "== 4. uninstall keeps saves by default"
mkdir -p "$UD/memcard/slotA"; echo save > "$UD/memcard/slotA/x.bin"
mkdir -p "$ROOT/LocalAppData/SMB2/gamedata"; echo "$MAGIC" > "$ROOT/LocalAppData/SMB2/gamedata/manifest.txt"
mkdir -p "$ROOT/LocalAppData/SMB2 PC Port/gamedata"; echo "$MAGIC" > "$ROOT/LocalAppData/SMB2 PC Port/gamedata/manifest.txt"
touch "$INST/user_added_file.txt"
mkdir -p "$INST/my_mods"; echo m > "$INST/my_mods/mod.txt"
mkdir -p "$GD.partial"; touch "$GD.partial/.smb2-extract-partial" "$GD.partial/half.bin"
uninstall
check "exit 0" [ "$RC" = 0 ]
check "exe removed" [ ! -e "$INST/smb2_pc_port.exe" ]
check "assets removed" [ ! -e "$INST/assets" ]
check "install.ini removed" [ ! -e "$INST/install.ini" ]
check "unknown user file kept (no blind rm -r of INSTDIR)" [ -e "$INST/user_added_file.txt" ]
check "user folder in INSTDIR kept" [ -f "$INST/my_mods/mod.txt" ]
check "INSTDIR\\gamedata + icon removed" [ ! -e "$GD" ]
check "interrupted extraction (.partial with marker) removed" [ ! -e "$GD.partial" ]
check "LocalAppData recovery copy removed" [ ! -e "$ROOT/LocalAppData/SMB2" ]
check "old-name LocalAppData recovery copy removed" [ ! -e "$ROOT/LocalAppData/SMB2 PC Port" ]
check "saves kept" [ -f "$UD/memcard/slotA/x.bin" ]
check "shortcuts removed" [ ! -e "$ROOT/StartMenu/SMB2.lnk" -a ! -e "$ROOT/Desktop/SMB2.lnk" ]
check "registry removed" no_key
clean

echo "== 5. /SKIPISO /DESKTOP=0 /ARCH=x86"
mkdir -p "$UD"; echo s > "$UD/settings.dat"
mkdir -p "$OLDUD/memcard"; echo s > "$OLDUD/settings.dat"                       # not moved by the game yet
mkdir -p "$ROOT/AppData/SMB2/smb2_pc_port"; echo p > "$ROOT/AppData/SMB2/smb2_pc_port/settings.dat"   # portable build's SDL pref path
setup /SKIPISO /DESKTOP=0 /ARCH=x86
check "exit 0" [ "$RC" = 0 ]
check "no extraction" not grepq '--extract-iso' "$INST/fake_port_calls.log"
check "no game data" [ ! -e "$GD" ]
check "no desktop shortcut" [ ! -e "$ROOT/Desktop/SMB2.lnk" ]
check "Start Menu shortcut uses generic icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WINST\\smb2.ico,0" ]
check "DisplayIcon generic" [ "$(regval '\Uninstall' DisplayIcon)" = "$WINST\\smb2.ico" ]
check "arch i686 forced" [ "$(regval '' Arch)" = i686 ]
check "i686 exe installed" file "$INST/smb2_pc_port.exe" | grep -q 'Intel 80386'
echo "   uninstall /REMOVEUSERDATA"
uninstall /REMOVEUSERDATA
check "saves/settings removed" [ ! -e "$UD" ]
check "old-name saves/settings removed" [ ! -e "$OLDUD" ]
check "portable build's settings (AppData\\SMB2\\smb2_pc_port) kept" [ -f "$ROOT/AppData/SMB2/smb2_pc_port/settings.dat" ]
clean

for t in "wrong_game:12" "corrupt:13" "diskfull:14" "pal:12"; do
    n=${t%%:*}; want=${t##*:}
    echo "== 6. extraction error: $n.iso -> setup exit $want"
    setup "/ISO=$WI\\$n.iso"
    check "exit $want" [ "$RC" = "$want" ]
    check "program still installed" [ -f "$INST/smb2_pc_port.exe" ]
    check "no game data" [ ! -e "$GD/manifest.txt" ]
    check "shortcut uses generic icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WINST\\smb2.ico,0" ]
    uninstall
    clean
done

echo "== 7. missing image file"
setup "/ISO=$WI\\does_not_exist.iso"
check "exit 12 (rejected)" [ "$RC" = 12 ]
uninstall; clean

echo "== 8. upgrade from the 1.0.0 layout: verified %ProgramData% game data is moved (rename)"
setup /SKIPISO
make_legacy
: > "$INST/fake_port_calls.log"; : > "$FLOG"
setup
check "exit 0" [ "$RC" = 0 ]
check "old data verified in %ProgramData%" grepf "[--verify-gamedata] [$(wslpath -w "$LEG")]" "$FLOG"
check "no --extract-iso" not grepq '--extract-iso' "$INST/fake_port_calls.log"
check "moved, not re-extracted" [ -f "$GD/files/legacy_marker.bin" -a -f "$GD/manifest.txt" -a -f "$GD/icon.ico" ]
check "old game data folder gone" [ ! -e "$LEG" ]
check "old %ProgramData%\\SMB2 PC Port folder removed" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "install.ini rewritten: gamedata=gamedata" [ "$(ini_get gamedata)" = gamedata ]
check "shortcut icon = INSTDIR\\gamedata\\icon.ico" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WGD\\icon.ico,0" ]
check "DisplayIcon = INSTDIR\\gamedata\\icon.ico" [ "$(regval '\Uninstall' DisplayIcon)" = "$WGD\\icon.ico" ]
check "EstimatedSize includes the moved data" [ $(( $(regval '\Uninstall' EstimatedSize) )) -gt 1013300 ]
rm -f -- "$INST/fake_port_calls.log"   # (test double's own log)
uninstall
check "uninstall: game data removed" [ ! -e "$GD" ]
check "uninstall: empty install folder removed" [ ! -e "$INST" ]
check "uninstall: registry removed" no_key
clean

echo "== 9. upgrade from the 1.0.0 layout on another drive: copy + verify (/TESTNORENAME)"
setup /SKIPISO
make_legacy
: > "$INST/fake_port_calls.log"; : > "$FLOG"
setup "/ISO=$WI\\good.iso" /TESTNORENAME
check "exit 0" [ "$RC" = 0 ]
check "copy verified as .partial" grepf "[--verify-gamedata] [$WGD.partial]" "$FLOG"
check "no --extract-iso" not grepq '--extract-iso' "$INST/fake_port_calls.log"
check "copied" [ -f "$GD/files/legacy_marker.bin" -a -f "$GD/manifest.txt" ]
check "copy marker removed" [ ! -e "$GD/.smb2-extract-partial" ]
check "no .partial left" [ ! -e "$GD.partial" ]
check "old game data deleted after the copy" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "shortcut icon = INSTDIR\\gamedata\\icon.ico" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WGD\\icon.ico,0" ]
uninstall; clean

echo "== 10. copy does not verify: fall back to extracting /ISO, old data kept until the next run"
setup /SKIPISO
make_legacy badcopy=1
: > "$INST/fake_port_calls.log"
setup "/ISO=$WI\\good.iso" /TESTNORENAME
check "exit 0" [ "$RC" = 0 ]
check "--extract-iso called" grepq '--extract-iso' "$INST/fake_port_calls.log"
check "fresh game data" [ -f "$GD/manifest.txt" -a ! -e "$GD/files/legacy_marker.bin" ]
check "no .partial left" [ ! -e "$GD.partial" ]
check "old game data kept (move failed)" [ -f "$LEG/files/legacy_marker.bin" ]
setup
check "next run: exit 0" [ "$RC" = 0 ]
check "next run: old game data removed" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "next run: new game data kept" [ -f "$GD/manifest.txt" ]
uninstall; clean

echo "== 11. old game data fails verification: deleted, fresh extraction"
setup /SKIPISO
make_legacy bad=1
: > "$INST/fake_port_calls.log"
setup "/ISO=$WI\\good.iso"
check "exit 0" [ "$RC" = 0 ]
check "--extract-iso called" grepf "good.iso] [--gamedata] [$WGD] [--progress]" "$INST/fake_port_calls.log"
check "fresh game data" [ -f "$GD/manifest.txt" -a ! -e "$GD/files/legacy_marker.bin" ]
check "old game data deleted" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
uninstall; clean

echo "== 12. old game data fails verification, no disc image: deleted, no game data"
setup /SKIPISO
make_legacy bad=1
setup
check "exit 0" [ "$RC" = 0 ]
check "old game data deleted" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "no game data" [ ! -e "$GD" ]
check "shortcut uses generic icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WINST\\smb2.ico,0" ]
uninstall; clean

echo "== 13. unexpected folders are never deleted recursively"
mkdir -p "$LEG"; echo mine > "$LEG/my_notes.txt"
setup /SKIPISO
check "exit 0" [ "$RC" = 0 ]
check "setup: %ProgramData% folder without our manifest kept" [ -f "$LEG/my_notes.txt" ]
mkdir -p "$GD"; echo "not ours" > "$GD/manifest.txt"; echo mine > "$GD/notes.txt"
uninstall
check "uninstall: INSTDIR\\gamedata without our manifest kept" [ -f "$GD/notes.txt" ]
check "uninstall: %ProgramData% folder without our manifest kept" [ -f "$LEG/my_notes.txt" ]
check "uninstall: program removed" [ ! -e "$INST/smb2_pc_port.exe" -a ! -e "$INST/assets" ]
clean

echo "== 14. uninstall also removes leftover 1.0.0 game data"
setup /SKIPISO
make_legacy
uninstall
check "exit 0" [ "$RC" = 0 ]
check "old game data removed" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "install folder removed" [ ! -e "$INST" ]
clean


if [ -z "$OLD_SETUP" ]; then
    echo "== 15-21. upgrade from the old name: SKIPPED (set SMB2_OLD_SETUP to a TESTMODE setup built from the pre-1.0.4 smb2.nsi)"
    bad "upgrade tests not run (SMB2_OLD_SETUP not set)"
else
OLD_SETUP="$(realpath "$OLD_SETUP")"

echo "== 15. upgrade from an \"SMB2 PC Port\" install: game data moved (rename), old install removed, one entry"
old_setup "$WOLDINST" "/ISO=$WI\\good.iso"
check "old setup: exit 0" [ "$RC" = 0 ]
check "old setup: old install present" [ -f "$OLDINST/smb2_pc_port.exe" -a -f "$OLDINST/gamedata/manifest.txt" -a -f "$ROOT/StartMenu/SMB2 PC Port.lnk" ]
check "old setup: old uninstall key" old_key
echo old-marker > "$OLDINST/gamedata/files_marker.bin"
mkdir -p "$OLDUD/memcard/slotA"; echo save > "$OLDUD/memcard/slotA/x.bin"
: > "$FLOG"
setup
check "exit 0" [ "$RC" = 0 ]
check "old game data verified" grepf "[--verify-gamedata] [$WOLDINST\\gamedata]" "$FLOG"
check "no --extract-iso" not grepq '--extract-iso' "$FLOG"
check "moved into the new folder (not re-extracted)" [ -f "$GD/files_marker.bin" -a -f "$GD/manifest.txt" -a -f "$GD/icon.ico" ]
check "old program files removed" [ ! -e "$OLDINST/smb2_pc_port.exe" -a ! -e "$OLDINST/assets" -a ! -e "$OLDINST/install.ini" -a ! -e "$OLDINST/uninstall.exe" -a ! -e "$OLDINST/README.txt" ]
check "old game data gone" [ ! -e "$OLDINST/gamedata" ]
check "old (now empty) folder removed" [ ! -e "$OLDINST" ]
check "old shortcuts removed" [ ! -e "$ROOT/StartMenu/SMB2 PC Port.lnk" -a ! -e "$ROOT/Desktop/SMB2 PC Port.lnk" ]
check "new shortcuts" [ -f "$ROOT/StartMenu/SMB2.lnk" -a -f "$ROOT/Desktop/SMB2.lnk" ]
check "new shortcut uses the moved disc icon" [ "$(lnk_icon "$ROOT/StartMenu/SMB2.lnk")" = "$WGD\\icon.ico,0" ]
check "old registry keys removed (one entry left)" not old_key
check "new ARP entry" [ "$(regval '\Uninstall' DisplayName)" = SMB2 -a "$(regval '\Uninstall' InstallLocation)" = "$WINST" ]
check "no OldInstallDir left" [ -z "$(regval '' OldInstallDir)" ]
check "install.ini: gamedata=gamedata, new userdata" [ "$(ini_get gamedata)" = gamedata -a "$(ini_get userdata)" = "$WROOT\\AppData\\SMB2\\installed" ]
check "saves untouched by Setup (the game moves them)" [ -f "$OLDUD/memcard/slotA/x.bin" ]
uninstall
check "uninstall: exit 0" [ "$RC" = 0 ]
check "uninstall: game data + folder removed" [ ! -e "$INST" ]
check "uninstall: saves kept (default)" [ -f "$OLDUD/memcard/slotA/x.bin" ]
check "uninstall: registry removed" no_key
clean

echo "== 16. old install in the old default folder, no registry key, no /D=: found, new folder next to it"
old_setup "$WROOT\\Program Files\\SMB2 PC Port" "/ISO=$WI\\good.iso" /DESKTOP=0
check "old setup: exit 0" [ "$RC" = 0 ]
reg.exe delete "$OLDKEY" /f >/dev/null 2>&1
echo mine > "$ROOT/Program Files/SMB2 PC Port/user_notes.txt"
new_setup_nod
check "exit 0" [ "$RC" = 0 ]
check "installed to <parent>\\SMB2" [ -f "$ROOT/Program Files/SMB2/smb2_pc_port.exe" ]
check "game data moved" [ -f "$ROOT/Program Files/SMB2/gamedata/manifest.txt" -a ! -e "$ROOT/Program Files/SMB2 PC Port/gamedata" ]
check "old program removed" [ ! -e "$ROOT/Program Files/SMB2 PC Port/smb2_pc_port.exe" -a ! -e "$ROOT/Program Files/SMB2 PC Port/assets" ]
check "user file in the old folder kept (no recursive delete)" [ -f "$ROOT/Program Files/SMB2 PC Port/user_notes.txt" ]
check "registry: InstallLocation = new folder" [ "$(regval '\Uninstall' InstallLocation)" = "$WROOT\\Program Files\\SMB2" ]
clean

echo "== 17. old install, other drive (/TESTNORENAME): copy + verify, old removed"
old_setup "$WOLDINST" "/ISO=$WI\\good.iso"
echo old-marker > "$OLDINST/gamedata/files_marker.bin"
: > "$FLOG"
setup /TESTNORENAME
check "exit 0" [ "$RC" = 0 ]
check "copy verified as .partial" grepf "[--verify-gamedata] [$WGD.partial]" "$FLOG"
check "no --extract-iso" not grepq '--extract-iso' "$FLOG"
check "copied" [ -f "$GD/files_marker.bin" -a ! -e "$GD/.smb2-extract-partial" -a ! -e "$GD.partial" ]
check "old install + game data removed" [ ! -e "$OLDINST" ]
check "old keys removed" not old_key
uninstall; clean

echo "== 18. old install, copy does not verify: /ISO extracted, old game data kept for the next run"
old_setup "$WOLDINST" "/ISO=$WI\\good.iso"
echo badcopy=1 >> "$OLDINST/gamedata/manifest.txt"
: > "$FLOG"
setup "/ISO=$WI\\good.iso" /TESTNORENAME
check "exit 0" [ "$RC" = 0 ]
check "--extract-iso into the new folder" grepf "good.iso] [--gamedata] [$WGD] [--progress]" "$FLOG"
check "fresh game data" [ -f "$GD/manifest.txt" ] && check "not the old one" not grepq badcopy "$GD/manifest.txt"
check "old program files removed" [ ! -e "$OLDINST/smb2_pc_port.exe" -a ! -e "$OLDINST/assets" ]
check "old game data kept (move failed)" [ -f "$OLDINST/gamedata/manifest.txt" ]
check "old keys removed (one entry)" not old_key
check "OldInstallDir recorded" [ "$(regval '' OldInstallDir)" = "$WOLDINST" ]
: > "$FLOG"
setup
check "next run: exit 0" [ "$RC" = 0 ]
check "next run: old game data + folder removed" [ ! -e "$OLDINST" ]
check "next run: OldInstallDir cleared" [ -z "$(regval '' OldInstallDir)" ]
check "next run: new game data kept" [ -f "$GD/manifest.txt" ]
uninstall; clean

echo "== 19. old install, game data fails verification: deleted, fresh extraction"
old_setup "$WOLDINST" "/ISO=$WI\\good.iso"
echo bad=1 >> "$OLDINST/gamedata/manifest.txt"
: > "$FLOG"
setup "/ISO=$WI\\good.iso"
check "exit 0" [ "$RC" = 0 ]
check "--extract-iso called" grepf "good.iso] [--gamedata] [$WGD] [--progress]" "$FLOG"
check "old install + game data removed" [ ! -e "$OLDINST" ]
check "OldInstallDir not recorded" [ -z "$(regval '' OldInstallDir)" ]
uninstall; clean

echo "== 20. old install with 1.0.0 game data in %ProgramData%: that is moved, old install removed"
old_setup "$WOLDINST" /SKIPISO
mkdir -p "$LEG/files"; { echo "$MAGIC"; echo "disc_id=GM2E8P"; } > "$LEG/manifest.txt"; echo legacy-data > "$LEG/files/legacy_marker.bin"
printf 'mode=installed\r\ngamedata=%s\r\nuserdata=%s\r\nversion=1.0.0\r\n' "$(wslpath -w "$LEG")" "$WROOT\\AppData\\SMB2 PC Port" > "$OLDINST/install.ini"
: > "$FLOG"
setup
check "exit 0" [ "$RC" = 0 ]
check "legacy data verified" grepf "[--verify-gamedata] [$(wslpath -w "$LEG")]" "$FLOG"
check "moved into the new folder" [ -f "$GD/files/legacy_marker.bin" ]
check "%ProgramData% folder removed" [ ! -e "$ROOT/ProgramData/SMB2 PC Port" ]
check "old install removed" [ ! -e "$OLDINST" ]
check "old keys removed" not old_key
uninstall; clean

echo "== 21. new install chosen in the old folder (/D=<old dir>): in place, one entry"
old_setup "$WOLDINST" "/ISO=$WI\\good.iso"
before=$(stat -c %Y "$OLDINST/gamedata/manifest.txt"); sleep 1
run_cmdline "$SETUP" "/S /TESTROOT=$WROOT /D=$WOLDINST"
check "exit 0" [ "$RC" = 0 ]
check "game data kept in place" [ "$(stat -c %Y "$OLDINST/gamedata/manifest.txt")" = "$before" ]
check "program still there (new version)" [ -f "$OLDINST/smb2_pc_port.exe" -a -f "$OLDINST/uninstall.exe" ]
check "install.ini rewritten (new userdata)" [ "$(tr -d '\r' < "$OLDINST/install.ini" | sed -n 's/^userdata=//p')" = "$WROOT\\AppData\\SMB2\\installed" ]
check "shortcut renamed" [ -f "$ROOT/StartMenu/SMB2.lnk" -a ! -e "$ROOT/StartMenu/SMB2 PC Port.lnk" ]
check "old keys removed, new one points at the folder" [ "$(regval '\Uninstall' InstallLocation)" = "$WOLDINST" ] && check "one entry" not old_key
cp "$OLDINST/uninstall.exe" "$ROOT/un_copy.exe"; chmod +x "$ROOT/un_copy.exe"; run_cmdline "$ROOT/un_copy.exe" "/S _?=$WOLDINST"; rm -f -- "$ROOT/un_copy.exe"
rm -f -- "$OLDINST/fake_port_calls.log"; rmdir "$OLDINST" 2>/dev/null
check "uninstall: removed" [ ! -e "$OLDINST" ]
clean
fi
rm -rf -- "${ROOT:?}/isos"
rm -f -- "$FLOG"
echo "RESULT: $PASS passed, $FAIL failed"
[ $FAIL = 0 ]
