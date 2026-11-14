#!/bin/bash
# Collect the assets of a GitHub release and write SHA256SUMS.txt
# (docs/updater.md "Release procedure").  Creates nothing on GitHub.
#
#   tools/make_release.sh --version 1.2.3 --setup <SMB2_PC_Setup_1.2.3.exe> \
#       [--zip32 <SMB2_PC.zip>] [--zip64 <SMB2_PC_x64.zip>] [--out <dir>]
#
# Output: <dir> (default <pc_port>/release/v<version>) with the setup, the
# zips and SHA256SUMS.txt ("<sha256>  <file name>", sha256sum format), which
# the in-game updater requires before it installs a downloaded setup.
# Checks: the setup is named SMB2_PC_Setup_<version>.exe, and the
# smb2_pc_port.exe in each zip carries the same version (VERSIONINFO, from
# -DSMB2_VERSION) - a zip built without the right version would offer the
# update again after installing it.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PORT="$(cd "$HERE/.." && pwd)"
VERSION="" SETUP="" ZIP32="" ZIP64="" OUT=""
while [ $# -gt 0 ]; do
    case "$1" in
        --version) VERSION="$2"; shift 2;;
        --setup) SETUP="$2"; shift 2;;
        --zip32) ZIP32="$2"; shift 2;;
        --zip64) ZIP64="$2"; shift 2;;
        --out) OUT="$2"; shift 2;;
        *) echo "unknown option $1" >&2; exit 2;;
    esac
done
[ -n "$VERSION" ] && [ -n "$SETUP" ] || { echo "usage: $0 --version V --setup FILE [--zip32 FILE] [--zip64 FILE] [--out DIR]" >&2; exit 2; }
echo "$VERSION" | grep -Eq '^[0-9]+\.[0-9]+\.[0-9]+([-+][0-9A-Za-z.+-]*)?$' || { echo "version must look like 1.2.3 (got $VERSION)" >&2; exit 2; }
[ "$(basename "$SETUP")" = "SMB2_PC_Setup_${VERSION}.exe" ] || { echo "setup must be named SMB2_PC_Setup_${VERSION}.exe (the updater looks for that asset name)" >&2; exit 1; }
OUT="${OUT:-$PORT/release/v$VERSION}"

exe_version() { # zip -> FileVersion of smb2_pc_port.exe inside (UTF-16 resource string)
    unzip -p "$1" '*smb2_pc_port.exe' 2>/dev/null | strings -el | grep -A1 -x 'FileVersion' | tail -1
}
for z in "$ZIP32" "$ZIP64"; do
    [ -n "$z" ] || continue
    [ -f "$z" ] || { echo "missing $z" >&2; exit 1; }
    v="$(exe_version "$z")"
    if [ "$v" != "$VERSION" ]; then
        echo "$z: smb2_pc_port.exe has version '$v', not $VERSION (build with -DSMB2_VERSION=$VERSION)" >&2
        exit 1
    fi
done

mkdir -p "$OUT"
cp -f "$SETUP" "$OUT/"
[ -n "$ZIP32" ] && cp -f "$ZIP32" "$OUT/SMB2_PC.zip"
[ -n "$ZIP64" ] && cp -f "$ZIP64" "$OUT/SMB2_PC_x64.zip"
(
    cd "$OUT"
    files=("SMB2_PC_Setup_${VERSION}.exe")
    [ -f SMB2_PC.zip ] && files+=(SMB2_PC.zip)
    [ -f SMB2_PC_x64.zip ] && files+=(SMB2_PC_x64.zip)
    sha256sum "${files[@]}" > SHA256SUMS.txt
    sha256sum -c SHA256SUMS.txt
)
echo
echo "Release assets in $OUT:"
ls -l "$OUT"
cat <<EOF

Next (docs/updater.md): tag v$VERSION, create the GitHub release "v$VERSION"
with the release notes as its body and upload every file above, e.g.
  gh release create v$VERSION $OUT/* --title "SMB2 $VERSION" --notes-file NOTES.md
Do not mark it as a pre-release (the updater only sees releases/latest).
EOF
