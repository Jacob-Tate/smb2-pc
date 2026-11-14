; SMB2 - Windows installer (NSIS 3, Modern UI 2) for the unofficial PC port.
; The app was called "SMB2 PC Port" before 1.0.4; Setup upgrades such an
; install (MigrateOld*, see pc_port/docs/installer.md "Upgrade from the old name").
;
; Interface with the port (CLI, install.ini, folders): pc_port/docs/installer.md.
; Build with installer/build.sh (it passes the defines below).  Defines:
;   VERSION      version string (install.ini, Add/Remove Programs)
;   EXE32, EXE64 the i686 and x86_64 smb2_pc_port.exe to package
;   ASSETS_DIR   the assets\ directory to install next to the exe
;   OUTFILE      output setup exe
;   TESTMODE     (optional) per-user test build: installs nothing outside
;                TESTROOT (or /TESTROOT=<dir>), registry only under
;                HKCU\Software\SMB2Test (the old name's test key
;                HKCU\Software\SMB2PCPortTest is upgraded).  For automated tests only.
;   TESTROOT     default test root (Windows path) for TESTMODE
;
; Command line (setup):
;   /S                      silent
;   /ISO=<path>             disc image to extract (silent mode); default: skip
;   /SKIPISO                do not extract (the game asks for the image later)
;   /REEXTRACT              extract even if the existing game data verifies OK
;   /DESKTOP=0|1            desktop shortcut (default 1)
;   /ARCH=x86|x64           force the exe flavour (x64 only on 64-bit Windows)
;   /D=<dir>                install directory (must be last, no quotes)
;   /TESTNORENAME           (TESTMODE only) migrate old game data by copy+verify, as across drives
;
; Layout: program + extracted game data in $INSTDIR (gamedata\ = $INSTDIR\gamedata,
; install.ini gamedata=gamedata, relative to the exe folder); saves etc. in %APPDATA%.
; Game data from 1.0.0 (%ProgramData%\SMB2 PC Port\gamedata) or from an install
; under the old name (<old dir>\gamedata) is moved into $INSTDIR\gamedata if it
; verifies, else deleted (MoveGameData / RemoveLegacy / RemoveOldInstall).
; Exit code in silent mode: 0 ok, 10+N if extraction failed with port exit code N
; (program files stay installed), 1 cancelled/failed.
; Uninstaller: /S silent; /REMOVEUSERDATA also deletes saves/settings.

Unicode true
ManifestDPIAware true
ManifestSupportedOS all
SetCompressor /SOLID lzma
!if ${NSIS_PTR_SIZE} > 4
  !error "STARTUPINFO layout below assumes the x86 installer stub"
!endif

!ifndef VERSION
  !define VERSION "0.0.0-dev"
!endif
!ifndef EXE32
  !error "define EXE32"
!endif
!ifndef EXE64
  !error "define EXE64"
!endif
!ifndef ASSETS_DIR
  !error "define ASSETS_DIR"
!endif
!ifndef PROG_KB_64
  !define PROG_KB_64 20000
!endif
!ifndef PROG_KB_32
  !define PROG_KB_32 20000
!endif
!ifndef OUTFILE
  !define OUTFILE "SMB2_PC_Setup_${VERSION}.exe"
!endif

!define APPNAME   "SMB2"
!define PUBLISHER "Unofficial fan project (not affiliated with SEGA or Nintendo)"
!define COMMENTS  "Unofficial PC port. Requires your own GameCube disc image; no game content included."
!define EXENAME   "smb2_pc_port.exe"
!define LNKNAME   "SMB2.lnk"
!define APPDIR    "SMB2"            ; install folder, %APPDATA% / %LOCALAPPDATA% folder
!define USERSUB   "installed"       ; %APPDATA%\SMB2\installed (the portable build uses %APPDATA%\smb2\smb2_pc_port)
; the name before 1.0.4 (upgraded by MigrateOld*)
!define OLD_APPNAME "SMB2 PC Port"
!define OLD_LNKNAME "SMB2 PC Port.lnk"
!define GAMEDATA_KB 1013300  ; extracted game data (GM2E8P: 1,037,446,520 bytes)
!define FREE_NEEDED_MB 1200
!define MANIFEST_MAGIC "SMB2PC-GAMEDATA 1"       ; first line of gamedata\manifest.txt (gamedata.c)
!define PARTIAL_MARKER ".smb2-extract-partial"   ; marker in gamedata.partial (gamedata.c)

!ifdef TESTMODE
  !define REGROOT   HKCU
  !define REGKEY    "Software\SMB2Test"
  !define UNINSTKEY "Software\SMB2Test\Uninstall"
  !define OLD_REGKEY    "Software\SMB2PCPortTest"            ; TESTMODE keys of the old name
  !define OLD_UNINSTKEY "Software\SMB2PCPortTest\Uninstall"
  !ifndef TESTROOT
    !define TESTROOT "$TEMP\SMB2Test"
  !endif
  RequestExecutionLevel user
  Name "${APPNAME} (TEST MODE)"
!else
  !define REGROOT   HKLM
  ; stable IDs (not shown anywhere); the visible name is DisplayName
  !define REGKEY    "Software\SMB2PCPort"
  !define UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\SMB2PCPort"
  !define OLD_REGKEY    "Software\SMB2 PC Port"
  !define OLD_UNINSTKEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\SMB2 PC Port"
  RequestExecutionLevel admin
  Name "${APPNAME}"
!endif

OutFile "${OUTFILE}"
BrandingText "${APPNAME} ${VERSION}"
ShowInstDetails show
ShowUninstDetails show

; version resource (VIProductVersion needs a.b.c.d)
!ifndef VIVERSION
  !define VIVERSION "0.0.0.0"
!endif
VIProductVersion "${VIVERSION}"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "ProductVersion" "${VERSION}"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "FileDescription" "${APPNAME} Setup"
VIAddVersionKey "CompanyName" "unofficial PC port"
VIAddVersionKey "Comments" "${COMMENTS}"
VIAddVersionKey "LegalCopyright" "CC0 1.0 (see LICENSE.txt); no game content included"

!include MUI2.nsh
!include LogicLib.nsh
!include nsDialogs.nsh
!include x64.nsh
!include WinVer.nsh
!include FileFunc.nsh
!include WordFunc.nsh
!include TextFunc.nsh
!include WinMessages.nsh

; ---------------------------------------------------------------- variables
Var Use64            ; 1 = install the x86_64 exe
Var IsUpgrade
!ifdef TESTMODE
Var TestRoot
!endif
Var GameDataDir      ; $INSTDIR\gamedata (SetGameDataDir)
Var LegacyParent     ; 1.0.0 layout: %ProgramData%\SMB2 PC Port
Var LegacyDir        ; 1.0.0 layout: %ProgramData%\SMB2 PC Port\gamedata
Var UserDataIni      ; userdata= value for install.ini
Var UserDataDir      ; real per-user dir (uninstaller): %APPDATA%\SMB2\installed
Var UserDataParent   ; %APPDATA%\SMB2 (removed only if empty)
Var OldUserDataDir   ; %APPDATA%\SMB2 PC Port (before 1.0.4; the game moves it)
Var LocalDataDir     ; per-user recovery-flow dir (uninstaller)
Var OldLocalDataDir  ; %LOCALAPPDATA%\SMB2 PC Port (before 1.0.4)
Var OldInstDir       ; install under the old name to upgrade ("" = none)
Var OldKeep          ; 1 = keep $OldInstDir\gamedata (moving it failed)
Var LnkDir           ; Start Menu programs folder
Var DeskDir
Var WantDesktop
Var IsoPath
Var SkipIso
Var Reuse            ; 1 = keep existing, verified game data
Var CanReuse         ; existing game data verified OK
Var ReuseSrc         ; where it is: "inst" ($GameDataDir), "old" ($OldInstDir\gamedata)
                     ; or "legacy" ($LegacyDir)
Var MoveSrc          ; MoveGameData: the folder to move into $GameDataDir
Var VerifyExe        ; new exe unpacked to $PLUGINSDIR for --verify-gamedata
!ifdef TESTMODE
Var NoRename
!endif
Var ForceReextract
Var ExtractState     ; "", running, ok, failed, skipped
Var ExtractCode
Var ExtractMsg
Var MigrateMsg       ; why the old game data could not be moved (extraction page)
Var DirText          ; directory page text (mentions an old install)
Var hProc
Var hLog
Var LastDone
Var LastTotal
; ISO page controls
Var IsoDlg
Var IsoEdit
Var IsoBrowse
Var IsoInfo
Var IsoSkip
Var IsoKeep
; extract page controls
Var ExDlg
Var ExTitle
Var ExBar
Var ExStatus
Var ExError
Var ExPick
Var ExSkip

; ---------------------------------------------------------------- MUI setup
!define MUI_ICON   "smb2.ico"
!define MUI_UNICON "smb2.ico"
!define MUI_CUSTOMFUNCTION_ABORT OnUserAbortHook

!define MUI_WELCOMEPAGE_TITLE "Welcome to ${APPNAME} ${VERSION} Setup"
!define MUI_WELCOMEPAGE_TEXT "This installs ${APPNAME}, an unofficial, non-commercial PC port of Super Monkey Ball 2 for the GameCube.$\r$\n$\r$\nNo game content is included. You need your own Super Monkey Ball 2 GameCube disc image (USA version, disc ID GM2E8P, .iso or .gcm) made from a disc you own. Setup extracts the game data from it (about 1 GB). You can also skip that step and choose the disc image later in the game.$\r$\n$\r$\nClick Next to continue."
!insertmacro MUI_PAGE_WELCOME

!define MUI_LICENSEPAGE_TEXT_TOP "License, credits and third-party notices:"
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"

!define MUI_PAGE_CUSTOMFUNCTION_PRE DirPagePre
!define MUI_PAGE_CUSTOMFUNCTION_SHOW DirPageShow
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirPageLeave
!define MUI_DIRECTORYPAGE_VERIFYONLEAVE
!define MUI_DIRECTORYPAGE_TEXT_TOP "$DirText"
!insertmacro MUI_PAGE_DIRECTORY

!define MUI_COMPONENTSPAGE_NODESC
!insertmacro MUI_PAGE_COMPONENTS

Page custom IsoPageCreate IsoPageLeave

!insertmacro MUI_PAGE_INSTFILES

Page custom ExtractPageCreate ExtractPageLeave

!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME} now"
!define MUI_FINISHPAGE_RUN_FUNCTION LaunchGame
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Show README"
!define MUI_FINISHPAGE_SHOWREADME_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION ShowReadme
!define MUI_PAGE_CUSTOMFUNCTION_SHOW FinishPageShow
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---------------------------------------------------------------- helpers

; Write $0 as UTF-8 (no BOM, no terminator) to the file handle in $1.
Function WriteUTF8
  Push $2
  Push $3
  Push $4
  System::Call 'kernel32::WideCharToMultiByte(i 65001, i 0, w r0, i -1, p 0, i 0, p 0, p 0) i .r2'
  ${If} $2 > 1
    System::Alloc $2
    Pop $3
    System::Call 'kernel32::WideCharToMultiByte(i 65001, i 0, w r0, i -1, p r3, i r2, p 0, p 0) i .r2'
    IntOp $2 $2 - 1
    System::Call 'kernel32::WriteFile(p r1, p r3, i r2, *i .r4, p 0) i'
    System::Free $3
  ${EndIf}
  Pop $4
  Pop $3
  Pop $2
FunctionEnd
!macro WriteIniLine H TEXT
  Push $0
  Push $1
  StrCpy $0 "${TEXT}$\r$\n"
  StrCpy $1 ${H}
  Call WriteUTF8
  Pop $1
  Pop $0
!macroend

; Paths that do not depend on the install dir.  Shared by both executables.
!macro InitPaths UN
Function ${UN}InitPaths
!ifdef TESTMODE
  ${If} $TestRoot == ""
    StrCpy $TestRoot "${TESTROOT}"
  ${EndIf}
  StrCpy $LegacyParent    "$TestRoot\ProgramData\${OLD_APPNAME}"
  StrCpy $UserDataParent  "$TestRoot\AppData\${APPDIR}"
  StrCpy $UserDataDir     "$UserDataParent\${USERSUB}"
  StrCpy $UserDataIni     "$UserDataDir"
  StrCpy $OldUserDataDir  "$TestRoot\AppData\${OLD_APPNAME}"
  StrCpy $LocalDataDir    "$TestRoot\LocalAppData\${APPDIR}"
  StrCpy $OldLocalDataDir "$TestRoot\LocalAppData\${OLD_APPNAME}"
  StrCpy $LnkDir          "$TestRoot\StartMenu"
  StrCpy $DeskDir         "$TestRoot\Desktop"
!else
  SetShellVarContext current
  StrCpy $UserDataParent  "$APPDATA\${APPDIR}"
  StrCpy $UserDataDir     "$UserDataParent\${USERSUB}"
  StrCpy $OldUserDataDir  "$APPDATA\${OLD_APPNAME}"
  StrCpy $LocalDataDir    "$LOCALAPPDATA\${APPDIR}"
  StrCpy $OldLocalDataDir "$LOCALAPPDATA\${OLD_APPNAME}"
  SetShellVarContext all
  StrCpy $LegacyParent   "$APPDATA\${OLD_APPNAME}"     ; = %ProgramData% (1.0.0 game data)
  ; expanded by the port; must equal PC_DEFAULT_USERDATA in install_mode.c
  ; (only that value lets the game move the old %APPDATA%\SMB2 PC Port)
  StrCpy $UserDataIni    "%APPDATA%\${APPDIR}\${USERSUB}"
  StrCpy $LnkDir  "$SMPROGRAMS"
  StrCpy $DeskDir "$DESKTOP"
!endif
  StrCpy $LegacyDir "$LegacyParent\gamedata"
FunctionEnd

; The game data lives in the install folder (read-only at runtime).
Function ${UN}SetGameDataDir
  StrCpy $GameDataDir "$INSTDIR\gamedata"
FunctionEnd

; $0 = folder.  $1 = 1 if it holds OUR game data (manifest.txt whose first
; line is the port's magic), else 0.  Only such folders are deleted recursively.
Function ${UN}IsOurGameData
  Push $2
  StrCpy $1 0
  ClearErrors
  FileOpen $2 "$0\manifest.txt" r
  ${IfNot} ${Errors}
    FileRead $2 $1
    FileClose $2
    ${TrimNewLines} $1 $1
    ${If} $1 == "${MANIFEST_MAGIC}"
      StrCpy $1 1
    ${Else}
      StrCpy $1 0
    ${EndIf}
  ${EndIf}
  Pop $2
FunctionEnd

; Remove the game data folder $0 and its extraction leftovers ($0.partial with
; the port's marker, $0.old with our manifest).  Anything else is only removed
; if it is empty: never delete a folder we did not create.
Function ${UN}RemoveGameData
  Push $0
  Push $1
  Call ${UN}IsOurGameData
  ${If} $1 == 1
    DetailPrint "Removing game data: $0"
    RMDir /r "$0"
  ${Else}
    RMDir "$0"
    ${If} ${FileExists} "$0\*.*"
      DetailPrint "Not removing $0: it is not a game data folder (no ${APPNAME} game data manifest.txt)"
    ${EndIf}
  ${EndIf}
  ${If} ${FileExists} "$0.partial\${PARTIAL_MARKER}"
    RMDir /r "$0.partial"
  ${Else}
    RMDir "$0.partial"
  ${EndIf}
  StrCpy $0 "$0.old"
  Call ${UN}IsOurGameData
  ${If} $1 == 1
    RMDir /r "$0"
  ${Else}
    RMDir "$0"
  ${EndIf}
  Pop $1
  Pop $0
FunctionEnd
!macroend
!insertmacro InitPaths ""
!insertmacro InitPaths "un."

!macro CheckAdmin
!ifndef TESTMODE
  UserInfo::GetAccountType
  Pop $0
  ${If} $0 != "admin"
    MessageBox MB_ICONSTOP "${APPNAME} is installed for all users and needs administrator rights.$\r$\nPlease run Setup again and allow it to make changes (or right-click > Run as administrator)." /SD IDOK
    SetErrorLevel 740
    Quit
  ${EndIf}
!endif
!macroend

!macro SetRegViewForOS
!ifndef TESTMODE
  ${If} ${RunningX64}
    SetRegView 64
  ${ElseIf} ${IsNativeARM64}
    SetRegView 64
  ${EndIf}
!endif
!macroend

; Wait until the installed exe is not running (it is locked while in use).
!macro WaitGameClosedIn DIR
  ${Do}
    ${IfNot} ${FileExists} "${DIR}\${EXENAME}"
      ${Break}
    ${EndIf}
    ClearErrors
    FileOpen $0 "${DIR}\${EXENAME}" a
    ${IfNot} ${Errors}
      FileClose $0
      ${Break}
    ${EndIf}
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "${APPNAME} is running. Please close the game, then click Retry." /SD IDCANCEL IDRETRY +2
    Abort "${APPNAME} is running."
  ${Loop}
!macroend
!define WaitGameClosed `!insertmacro WaitGameClosedIn "$INSTDIR"`

; ---------------------------------------------------------------- init
Function .onInit
  System::Call 'kernel32::CreateMutexW(p 0, i 0, w "SMB2PCPortSetupMutex") p .r1 ?e'
  Pop $0
  ${If} $0 == 183
    MessageBox MB_ICONEXCLAMATION "${APPNAME} Setup is already running." /SD IDOK
    Abort
  ${EndIf}
  ${IfNot} ${AtLeastWin7}
    MessageBox MB_ICONSTOP "${APPNAME} needs Windows 7 or later." /SD IDOK
    Abort
  ${EndIf}
  !insertmacro CheckAdmin

  ${GetParameters} $R0
!ifdef TESTMODE
  ClearErrors
  ${GetOptions} $R0 "/TESTROOT=" $TestRoot
  StrCpy $NoRename 0
  ClearErrors
  ${GetOptions} $R0 "/TESTNORENAME" $R1
  ${IfNot} ${Errors}
    StrCpy $NoRename 1
  ${EndIf}
!endif
  Call InitPaths

  ; exe flavour: x86_64 on x64 Windows (and Windows 11 on ARM, which runs x64), else i686
  StrCpy $Use64 0
  ${If} ${IsNativeAMD64}
    StrCpy $Use64 1
  ${ElseIf} ${IsNativeARM64}
  ${AndIf} ${AtLeastBuild} 22000
    StrCpy $Use64 1
  ${EndIf}
  ClearErrors
  ${GetOptions} $R0 "/ARCH=" $R1
  ${IfNot} ${Errors}
    ${If} $R1 == "x86"
      StrCpy $Use64 0
    ${ElseIf} $R1 == "x64"
    ${AndIf} ${RunningX64}
      StrCpy $Use64 1
    ${EndIf}
  ${EndIf}
  !insertmacro SetRegViewForOS

  ; upgrade: reuse the existing location unless /D= was given
  StrCpy $IsUpgrade 0
  ReadRegStr $R2 ${REGROOT} "${UNINSTKEY}" "InstallLocation"
  ${If} $R2 != ""
  ${AndIf} ${FileExists} "$R2\${EXENAME}"
    StrCpy $IsUpgrade 1
    ${If} $INSTDIR == ""
      StrCpy $INSTDIR $R2
    ${EndIf}
  ${EndIf}
  ; an install under the old name "SMB2 PC Port": upgraded into the new
  ; folder (default: next to it, <parent>\SMB2; /D= or the directory page may
  ; choose another one)
  Call FindOldInstall
  ${If} $INSTDIR == ""
  ${AndIf} $OldInstDir != ""
    ${GetParent} $OldInstDir $R2
    ${If} $R2 != ""
      StrCpy $INSTDIR "$R2\${APPDIR}"
    ${EndIf}
  ${EndIf}
  ${If} $INSTDIR == ""
!ifdef TESTMODE
    StrCpy $INSTDIR "$TestRoot\Program Files\${APPDIR}"
!else
    ${If} $Use64 == 1
      StrCpy $INSTDIR "$PROGRAMFILES64\${APPDIR}"
    ${Else}
      StrCpy $INSTDIR "$PROGRAMFILES\${APPDIR}"
    ${EndIf}
!endif
  ${EndIf}

  ; silent-mode options
  StrCpy $SkipIso 1
  StrCpy $IsoPath ""
  ClearErrors
  ${GetOptions} $R0 "/ISO=" $IsoPath
  ${If} $IsoPath != ""
    StrCpy $SkipIso 0
  ${EndIf}
  ClearErrors
  ${GetOptions} $R0 "/SKIPISO" $R1
  ${IfNot} ${Errors}
    StrCpy $SkipIso 1
  ${EndIf}
  StrCpy $ForceReextract 0
  ClearErrors
  ${GetOptions} $R0 "/REEXTRACT" $R1
  ${IfNot} ${Errors}
    StrCpy $ForceReextract 1
  ${EndIf}
  Call ApplyDesktopOption
  ${IfNot} ${Silent}
    StrCpy $SkipIso 0   ; interactive: decided on the disc image page
  ${EndIf}
  StrCpy $Reuse 0
  StrCpy $CanReuse ""
  StrCpy $ReuseSrc ""
  StrCpy $VerifyExe ""
  StrCpy $WantDesktop 0
  StrCpy $OldKeep 0
  Call SetGameDataDir
  StrCpy $DirText "Setup installs ${APPNAME} in the folder below. The game data extracted from your disc image (about 1 GB) is stored there too, in the gamedata subfolder. To install in a different folder, click Browse."
  ${If} $OldInstDir != ""
    StrCpy $DirText "Found ${OLD_APPNAME} (the old name of ${APPNAME}) in $OldInstDir. Setup installs ${APPNAME} in the folder below, moves the game data there and removes the old installation. Your saves are kept."
  ${EndIf}
FunctionEnd

; Sets $OldInstDir to an install under the old name "SMB2 PC Port" (its
; uninstall key's InstallLocation, else the old default folder, else a folder
; whose game data an earlier upgrade could not move: REGKEY OldInstallDir).
; Only folders with our exe + install.ini (or our game data) count.
Function FindOldInstall
  StrCpy $OldInstDir ""
  ReadRegStr $R2 ${REGROOT} "${OLD_UNINSTKEY}" "InstallLocation"
  ${If} $R2 != ""
  ${AndIf} ${FileExists} "$R2\${EXENAME}"
    StrCpy $OldInstDir $R2
    Return
  ${EndIf}
!ifdef TESTMODE
  StrCpy $R2 "$TestRoot\Program Files\${OLD_APPNAME}"
  ${If} ${FileExists} "$R2\${EXENAME}"
  ${AndIf} ${FileExists} "$R2\install.ini"
    StrCpy $OldInstDir $R2
    Return
  ${EndIf}
!else
  ${If} ${RunningX64}
    StrCpy $R2 "$PROGRAMFILES64\${OLD_APPNAME}"
    ${If} ${FileExists} "$R2\${EXENAME}"
    ${AndIf} ${FileExists} "$R2\install.ini"
      StrCpy $OldInstDir $R2
      Return
    ${EndIf}
  ${EndIf}
  StrCpy $R2 "$PROGRAMFILES\${OLD_APPNAME}"
  ${If} ${FileExists} "$R2\${EXENAME}"
  ${AndIf} ${FileExists} "$R2\install.ini"
    StrCpy $OldInstDir $R2
    Return
  ${EndIf}
!endif
  ReadRegStr $R2 ${REGROOT} "${REGKEY}" "OldInstallDir"
  ${If} $R2 != ""
  ${AndIf} ${FileExists} "$R2\gamedata\manifest.txt"
    StrCpy $OldInstDir $R2
  ${EndIf}
FunctionEnd

Function DirPageShow
  ${If} $OldInstDir != ""
    SendMessage $mui.DirectoryPage.Text ${WM_SETTEXT} 0 "STR:$DirText"
  ${EndIf}
FunctionEnd

Function DirPagePre
  ${If} $IsUpgrade == 1
    Abort   ; upgrade/reinstall: keep the existing location
  ${EndIf}
FunctionEnd

; The game data goes into the install folder, so its drive needs room for it.
Function DirPageLeave
  StrCpy $CanReuse ""   ; the install folder may have changed: check again
  Call SetGameDataDir
  GetInstDirError $0   ; 1 = invalid folder, 2 = not enough space (sizes include the game data: AddSize)
  ${If} $0 == 1
    MessageBox MB_ICONEXCLAMATION "Please choose a valid folder to install ${APPNAME} in." /SD IDOK
    Abort
  ${ElseIf} $0 == 2
    MessageBox MB_YESNO|MB_ICONEXCLAMATION "There is not enough free space on the drive of$\r$\n$INSTDIR$\r$\nfor the program and the game data extracted from your disc image (about ${FREE_NEEDED_MB} MB).$\r$\n$\r$\nContinue anyway? (You can skip the disc image step and free up space later.)" /SD IDYES IDYES +2
    Abort
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------- existing game data
; $0 = folder -> $0 = exit code of `<new exe> --verify-gamedata <folder>`
; (-1 without our manifest.txt: not run).  The exe is the one being installed,
; unpacked once to $PLUGINSDIR (the installed one may be old or missing).
Function VerifyGameData
  Push $1
  Call IsOurGameData
  ${If} $1 != 1
    StrCpy $0 -1
    Pop $1
    Return
  ${EndIf}
  ${If} $VerifyExe == ""
    InitPluginsDir
    SetOutPath "$PLUGINSDIR\verify"
    ${If} $Use64 == 1
      File /oname=${EXENAME} "${EXE64}"
    ${Else}
      File /oname=${EXENAME} "${EXE32}"
    ${EndIf}
    SetOutPath "$PLUGINSDIR"
    StrCpy $VerifyExe "$PLUGINSDIR\verify\${EXENAME}"
  ${EndIf}
  ${IfNot} ${Silent}
  ${AndIf} $HWNDPARENT != 0
    Banner::show /set 76 "Checking the existing game data..." "Please wait."
  ${EndIf}
  StrCpy $1 $0
  nsExec::Exec '"$VerifyExe" --verify-gamedata "$1"'
  Pop $0
  ${IfNot} ${Silent}
  ${AndIf} $HWNDPARENT != 0
    Banner::destroy
  ${EndIf}
  DetailPrint "Game data check of $1: exit code $0"
  Pop $1
FunctionEnd

; Sets $CanReuse to 1/0 (once) and $ReuseSrc: game data that verifies OK in the
; install folder ("inst"), in an install under the old name ("old") or, from
; 1.0.0, in %ProgramData% ("legacy"); "old"/"legacy" are moved into the
; install folder by MoveGameData.
Function CheckExistingGameData
  ${If} $CanReuse != ""
    Return
  ${EndIf}
  StrCpy $CanReuse 0
  StrCpy $ReuseSrc ""
  StrCpy $0 $GameDataDir
  Call VerifyGameData
  ${If} $0 == 0
    StrCpy $CanReuse 1
    StrCpy $ReuseSrc "inst"
    Return
  ${EndIf}
  ${If} $OldInstDir != ""
  ${AndIf} $OldInstDir != $INSTDIR
    StrCpy $0 "$OldInstDir\gamedata"
    Call VerifyGameData
    ${If} $0 == 0
      StrCpy $CanReuse 1
      StrCpy $ReuseSrc "old"
      Return
    ${EndIf}
  ${EndIf}
  StrCpy $0 $LegacyDir
  Call VerifyGameData
  ${If} $0 == 0
    StrCpy $CanReuse 1
    StrCpy $ReuseSrc "legacy"
  ${EndIf}
FunctionEnd

; Delete the 1.0.0 game data in %ProgramData% (only a folder with our
; manifest), then the %ProgramData%\SMB2 PC Port folder if it is empty.
Function RemoveLegacy
  ${If} ${FileExists} "$LegacyParent\*.*"
    StrCpy $0 $LegacyDir
    Call RemoveGameData
    RMDir "$LegacyParent"
  ${EndIf}
FunctionEnd

; Move verified game data from $MoveSrc ($LegacyDir from 1.0.0, or
; <old install>\gamedata) to $GameDataDir.
; Same drive: one rename.  Otherwise (or if the rename fails): copy to
; $GameDataDir.partial, verify the copy, rename it into place, then delete the
; old folder.  $0 = 1 on success; on failure the old folder is left as it was.
Function MoveGameData
  DetailPrint "Moving the game data from $MoveSrc to $GameDataDir"
  ; the target may only be a stale copy of ours (it did not verify) or empty
  StrCpy $0 $GameDataDir
  Call RemoveGameData
  ${If} ${FileExists} "$GameDataDir\*.*"
    DetailPrint "Cannot move the game data: $GameDataDir already exists."
    StrCpy $0 0
    Return
  ${EndIf}
  ClearErrors
!ifdef TESTMODE
  ${If} $NoRename == 1
    SetErrors   ; test: behave as if the folders were on different drives
  ${Else}
    Rename "$MoveSrc" "$GameDataDir"
  ${EndIf}
!else
  Rename "$MoveSrc" "$GameDataDir"
!endif
  ${IfNot} ${Errors}
    Call ResetGameDataAcl
    StrCpy $0 1
    Goto done
  ${EndIf}
  DetailPrint "Copying the game data (different drive); this may take a few minutes..."
  StrCpy $0 "$GameDataDir.partial"
  RMDir "$0"
  ${If} ${FileExists} "$0\${PARTIAL_MARKER}"
    RMDir /r "$0"
  ${EndIf}
  ${If} ${FileExists} "$0\*.*"
    DetailPrint "Cannot copy the game data: $0 already exists."
    StrCpy $0 0
    Return
  ${EndIf}
  CreateDirectory "$0"
  ClearErrors
  FileOpen $1 "$0\${PARTIAL_MARKER}" w   ; lets the port clean up an interrupted copy
  FileWrite $1 "partial$\n"
  FileClose $1
  ClearErrors
  ${If} ${Silent}
    CopyFiles /SILENT "$MoveSrc\*.*" "$0"
  ${Else}
    CopyFiles "$MoveSrc\*.*" "$0"
  ${EndIf}
  ${If} ${Errors}
    DetailPrint "Copying the game data failed."
    RMDir /r "$GameDataDir.partial"
    StrCpy $0 0
    Return
  ${EndIf}
  StrCpy $0 "$GameDataDir.partial"
  Call VerifyGameData
  ${If} $0 != 0
    DetailPrint "The copied game data did not verify (exit code $0)."
    RMDir /r "$GameDataDir.partial"
    StrCpy $0 0
    Return
  ${EndIf}
  Delete "$GameDataDir.partial\${PARTIAL_MARKER}"
  ClearErrors
  Rename "$GameDataDir.partial" "$GameDataDir"
  ${If} ${Errors}
    DetailPrint "Cannot rename $GameDataDir.partial to $GameDataDir."
    RMDir /r "$GameDataDir.partial"
    StrCpy $0 0
    Return
  ${EndIf}
  StrCpy $0 1
done:
  ; the source (after a copy) and its leftovers (.partial/.old); only our game data
  StrCpy $0 $MoveSrc
  Call RemoveGameData
  Call RemoveLegacy   ; the empty %ProgramData% folder
  StrCpy $0 1
FunctionEnd

; Remove an install under the old name ($OldInstDir, not the new $INSTDIR):
; only the files its Setup installed, its game data (our manifest only;
; kept if $OldKeep = 1: moving it failed) and the folder if it is empty
; then; its shortcuts and its registry keys, so that one entry is left.
Function RemoveOldInstall
  ${If} $OldInstDir == ""
    Return
  ${EndIf}
  ${If} $OldInstDir != $INSTDIR
    !insertmacro WaitGameClosedIn "$OldInstDir"
    DetailPrint "Removing the old ${OLD_APPNAME} program files from $OldInstDir"
    Delete "$OldInstDir\${EXENAME}"
    RMDir /r "$OldInstDir\assets"
    Delete "$OldInstDir\install.ini"
    Delete "$OldInstDir\README.txt"
    Delete "$OldInstDir\LICENSE.txt"
    Delete "$OldInstDir\smb2.ico"
    Delete "$OldInstDir\uninstall.exe"
    ${If} $OldKeep != 1
      StrCpy $0 "$OldInstDir\gamedata"
      Call RemoveGameData
    ${EndIf}
    RMDir "$OldInstDir"   ; only if empty: files the user added there are kept
  ${EndIf}
  ${If} $OldKeep == 1
    ; retried (and cleaned up) by the next Setup run or the uninstaller
    WriteRegStr ${REGROOT} "${REGKEY}" "OldInstallDir" "$OldInstDir"
  ${Else}
    DeleteRegValue ${REGROOT} "${REGKEY}" "OldInstallDir"
  ${EndIf}
  Delete "$LnkDir\${OLD_LNKNAME}"
  Delete "$DeskDir\${OLD_LNKNAME}"
  DeleteRegKey ${REGROOT} "${OLD_UNINSTKEY}"
  DeleteRegKey ${REGROOT} "${OLD_REGKEY}"
FunctionEnd

; A folder renamed from %ProgramData% keeps its old permissions (there, users
; may create files); make it inherit the install folder's read-only ones.
Function ResetGameDataAcl
!ifndef TESTMODE
  nsExec::Exec '"$SYSDIR\icacls.exe" "$GameDataDir" /reset /T /C /Q'
  Pop $1
  DetailPrint "Reset the game data permissions to the install folder's: exit code $1"
!endif
FunctionEnd

; Read the 6-byte disc ID at offset 0 into $1 (empty if unreadable).
Function ReadDiscId
  Push $5
  Push $6
  Push $7
  StrCpy $1 ""
  ClearErrors
  FileOpen $5 $0 r
  ${IfNot} ${Errors}
    ${For} $6 1 6
      FileReadByte $5 $7
      ${If} ${Errors}
        ${Break}
      ${EndIf}
      ${If} $7 < 32
      ${OrIf} $7 > 126
        StrCpy $7 63 ; '?'
      ${EndIf}
      IntFmt $7 "%c" $7
      StrCpy $1 "$1$7"
    ${Next}
    FileClose $5
  ${EndIf}
  Pop $7
  Pop $6
  Pop $5
FunctionEnd

; $0 = path.  Returns "" if acceptable, else an error text in $0.
Function PrecheckIso
  Push $1
  Push $2
  Push $3
  Push $4
  ${IfNot} ${FileExists} $0
    StrCpy $0 "The file was not found:$\r$\n$0"
    Goto done
  ${EndIf}
  ${GetFileExt} $0 $3
  Call ReadDiscId
  ${If} $3 == "iso"
  ${OrIf} $3 == "gcm"
    ${If} $1 == "GM2E8P"
      StrCpy $0 ""
    ${ElseIf} $1 == "GM2P8P"
      StrCpy $0 "This is the European (PAL) version of Super Monkey Ball 2 (disc ID GM2P8P).$\r$\nOnly the USA version (GM2E8P) is supported."
    ${ElseIf} $1 == "GM2J8P"
      StrCpy $0 "This is the Japanese version of Super Monkey Ball 2 (disc ID GM2J8P).$\r$\nOnly the USA version (GM2E8P) is supported."
    ${Else}
      StrCpy $0 "This file is not a Super Monkey Ball 2 (USA) GameCube disc image.$\r$\n(Disc ID found: $\"$1$\"; expected GM2E8P.)"
    ${EndIf}
  ${Else}
    StrCpy $0 ""   ; other formats: the port decides (RVZ/NKit/CISO support is optional)
  ${EndIf}
done:
  Pop $4
  Pop $3
  Pop $2
  Pop $1
FunctionEnd

; ---------------------------------------------------------------- disc image page
Function IsoPageCreate
  Call SetGameDataDir
  Call CheckExistingGameData
  !insertmacro MUI_HEADER_TEXT "Game data" "Choose your Super Monkey Ball 2 disc image."
  nsDialogs::Create 1018
  Pop $IsoDlg
  ${If} $IsoDlg == error
    Abort
  ${EndIf}
  ${NSD_CreateLabel} 0 0 100% 50u "${APPNAME} needs the game data from YOUR OWN copy of the game: a disc image of Super Monkey Ball 2 for the Nintendo GameCube, USA version (disc ID GM2E8P), as an .iso or .gcm file that you made from a disc you own. No game content is included with this program.$\r$\n$\r$\nSetup extracts about 1 GB into the install folder:  $GameDataDir"
  Pop $0
  ${NSD_CreateText} 0 56u 75% 13u "$IsoPath"
  Pop $IsoEdit
  ${NSD_OnChange} $IsoEdit IsoEditChanged
  ${NSD_CreateBrowseButton} 77% 55u 23% 15u "&Browse..."
  Pop $IsoBrowse
  ${NSD_OnClick} $IsoBrowse IsoBrowseClick
  ${NSD_CreateLabel} 0 74u 100% 26u ""
  Pop $IsoInfo
  ${NSD_CreateCheckbox} 0 102u 100% 12u "&Skip this step - I'll choose the disc image later (the game asks for it when it starts)"
  Pop $IsoSkip
  ${NSD_OnClick} $IsoSkip IsoUpdateState
  ${NSD_CreateCheckbox} 0 118u 100% 12u "&Keep the existing game data (it was checked and is complete)"
  Pop $IsoKeep
  ${NSD_OnClick} $IsoKeep IsoUpdateState
  ${If} $CanReuse == 1
    ${If} $ReuseSrc != "inst"
      ${NSD_SetText} $IsoKeep "&Keep the existing game data (it was checked and is complete; it is moved into the install folder)"
    ${EndIf}
    ${NSD_Check} $IsoKeep
  ${Else}
    ShowWindow $IsoKeep ${SW_HIDE}
  ${EndIf}
  ${If} $SkipIso == 1
    ${NSD_Check} $IsoSkip
  ${EndIf}
  Call IsoUpdateState
  Call IsoEditChanged
  nsDialogs::Show
FunctionEnd

Function IsoUpdateState
  Pop $0 ; control (ignored when called directly: harmless)
  ${NSD_GetChecked} $IsoKeep $1
  ${NSD_GetChecked} $IsoSkip $2
  ${If} $1 == ${BST_CHECKED}
    EnableWindow $IsoSkip 0
    StrCpy $2 ${BST_CHECKED}
  ${Else}
    EnableWindow $IsoSkip 1
  ${EndIf}
  ${If} $2 == ${BST_CHECKED}
    EnableWindow $IsoEdit 0
    EnableWindow $IsoBrowse 0
  ${Else}
    EnableWindow $IsoEdit 1
    EnableWindow $IsoBrowse 1
  ${EndIf}
FunctionEnd

Function IsoEditChanged
  Pop $0
  ${NSD_GetText} $IsoEdit $0
  ${If} $0 == ""
    ${NSD_SetText} $IsoInfo ""
    Return
  ${EndIf}
  ${IfNot} ${FileExists} $0
    ${NSD_SetText} $IsoInfo ""
    Return
  ${EndIf}
  Call ReadDiscId
  ${If} $1 == "GM2E8P"
    ${NSD_SetText} $IsoInfo "Disc ID GM2E8P: Super Monkey Ball 2 (USA). OK."
  ${Else}
    ${NSD_SetText} $IsoInfo "Disc ID $\"$1$\": this does not look like Super Monkey Ball 2 (USA, GM2E8P)."
  ${EndIf}
FunctionEnd

Function IsoBrowseClick
  Pop $0
  ${NSD_GetText} $IsoEdit $0
  nsDialogs::SelectFileDialog open "$0" "GameCube disc image (*.iso;*.gcm)|*.iso;*.gcm|All files (*.*)|*.*"
  Pop $0
  ${If} $0 != ""
  ${AndIf} $0 != "error"
    ${NSD_SetText} $IsoEdit $0
  ${EndIf}
FunctionEnd

Function IsoPageLeave
  StrCpy $Reuse 0
  StrCpy $SkipIso 0
  ${NSD_GetChecked} $IsoKeep $0
  ${If} $0 == ${BST_CHECKED}
  ${AndIf} $CanReuse == 1
    StrCpy $Reuse 1
    StrCpy $SkipIso 1
    Return
  ${EndIf}
  ${NSD_GetChecked} $IsoSkip $0
  ${If} $0 == ${BST_CHECKED}
    StrCpy $SkipIso 1
    Return
  ${EndIf}
  ${NSD_GetText} $IsoEdit $IsoPath
  ${If} $IsoPath == ""
    MessageBox MB_ICONEXCLAMATION "Please choose your Super Monkey Ball 2 disc image, or tick $\"Skip this step$\"."
    Abort
  ${EndIf}
  StrCpy $0 $IsoPath
  Call PrecheckIso
  ${If} $0 != ""
    MessageBox MB_ICONEXCLAMATION "$0$\r$\n$\r$\nPlease choose another file, or tick $\"Skip this step$\"."
    Abort
  ${EndIf}
  ; free space on the install drive (the game data goes to $INSTDIR\gamedata;
  ; new data is complete before old data is removed).  Root of a drive-letter
  ; path, or the install folder's nearest existing parent for UNC paths.
  StrCpy $2 $INSTDIR 3
  StrCpy $3 $INSTDIR 2
  ${If} $3 == "\\"
    StrCpy $2 $INSTDIR
    ${DoUntil} ${FileExists} "$2\*.*"
      ${GetParent} $2 $2
      ${IfThen} $2 == "" ${|} ${ExitDo} ${|}
    ${Loop}
  ${EndIf}
  System::Call 'kernel32::GetDiskFreeSpaceExW(w r2, *l .r1, *l, *l) i .r0'
  ${If} $0 != 0
    System::Int64Op $1 / 1048576
    Pop $1
    ${If} $1 < ${FREE_NEEDED_MB}
      MessageBox MB_YESNO|MB_ICONEXCLAMATION "There is only $1 MB free on the drive of $GameDataDir; about ${FREE_NEEDED_MB} MB are needed.$\r$\n$\r$\nContinue anyway?" IDYES +2
      Abort
    ${EndIf}
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------- sections
Section "${APPNAME} (required)" SecCore
  SectionIn RO
  AddSize ${GAMEDATA_KB}   ; the game data is extracted into $INSTDIR\gamedata
  Call SetGameDataDir
  SetOutPath "$INSTDIR"
  ${WaitGameClosed}

  ; upgrade: drop old assets so no stale files survive
  RMDir /r "$INSTDIR\assets"
  ${If} $Use64 == 1
    DetailPrint "Installing the 64-bit (x86_64) game executable"
    File /oname=${EXENAME} "${EXE64}"
  ${Else}
    DetailPrint "Installing the 32-bit (i686) game executable"
    File /oname=${EXENAME} "${EXE32}"
  ${EndIf}
  SetOutPath "$INSTDIR\assets"
  File /r "${ASSETS_DIR}\*.*"
  SetOutPath "$INSTDIR"
  File "README.txt"
  File "LICENSE.txt"
  File "smb2.ico"

  ; install.ini (UTF-8, key=value) - docs/installer.md
  ClearErrors
  FileOpen $9 "$INSTDIR\install.ini" w
  ${If} ${Errors}
    Abort "Cannot write $INSTDIR\install.ini"
  ${EndIf}
  !insertmacro WriteIniLine $9 "mode=installed"
  ; relative to the exe folder (install_mode.c): the game data is always the
  ; install folder's gamedata\, wherever that folder is
  !insertmacro WriteIniLine $9 "gamedata=gamedata"
  !insertmacro WriteIniLine $9 "userdata=$UserDataIni"
  !insertmacro WriteIniLine $9 "version=${VERSION}"
  FileClose $9

  WriteUninstaller "$INSTDIR\uninstall.exe"

  ; registry
  WriteRegStr ${REGROOT} "${REGKEY}" "InstallDir" "$INSTDIR"
  WriteRegStr ${REGROOT} "${REGKEY}" "Version" "${VERSION}"
  WriteRegStr ${REGROOT} "${REGKEY}" "GameDataDir" "$GameDataDir"
  ${If} $Use64 == 1
    WriteRegStr ${REGROOT} "${REGKEY}" "Arch" "x86_64"
  ${Else}
    WriteRegStr ${REGROOT} "${REGKEY}" "Arch" "i686"
  ${EndIf}
!ifdef TESTMODE
  WriteRegStr ${REGROOT} "${REGKEY}" "TestRoot" "$TestRoot"
!endif
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "DisplayName" "${APPNAME}"
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "DisplayVersion" "${VERSION}"
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "Publisher" "${PUBLISHER}"
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "Comments" "${COMMENTS}"
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
  WriteRegDWORD ${REGROOT} "${UNINSTKEY}" "NoModify" 1
  WriteRegDWORD ${REGROOT} "${UNINSTKEY}" "NoRepair" 1
  ; (EstimatedSize: UpdateShortcuts)

  ; silent mode: reuse verified data unless /REEXTRACT
  ${If} ${Silent}
  ${AndIf} $ForceReextract == 0
    Call CheckExistingGameData
    ${If} $CanReuse == 1
      StrCpy $Reuse 1
      StrCpy $SkipIso 1
      DetailPrint "Existing game data is complete; not extracting again."
    ${EndIf}
  ${EndIf}
  ; game data from 1.0.0 in %ProgramData% or from an install under the old
  ; name: move it into the install folder if it is kept, else delete it
  ; (install.ini no longer points there)
  ${If} $Reuse == 1
  ${AndIf} $ReuseSrc != "inst"
    ${If} $ReuseSrc == "legacy"
      StrCpy $MoveSrc $LegacyDir
    ${Else}
      StrCpy $MoveSrc "$OldInstDir\gamedata"
    ${EndIf}
    Call MoveGameData
    ${If} $0 != 1
      DetailPrint "The existing game data could not be moved; it has to be extracted again."
      StrCpy $Reuse 0
      ${If} $ReuseSrc == "old"
        StrCpy $OldKeep 1
        Call RemoveLegacy
      ${EndIf}
      StrCpy $MigrateMsg "The existing game data in $MoveSrc could not be moved into the install folder ($GameDataDir)."
      ${If} ${Silent}
      ${AndIf} $IsoPath != ""
        StrCpy $SkipIso 0   ; fall back to extracting the given image
      ${ElseIfNot} ${Silent}
        StrCpy $SkipIso 0   ; extraction page: choose a disc image or skip
      ${EndIf}
    ${EndIf}
  ${Else}
    Call RemoveLegacy
  ${EndIf}
  Call RemoveOldInstall
  ${If} $Reuse == 1
    StrCpy $ExtractState "ok"
  ${EndIf}
SectionEnd

Section "Start Menu shortcut" SecStartMenu
  SectionIn RO
SectionEnd

Section "Desktop shortcut" SecDesktop
  StrCpy $WantDesktop 1
SectionEnd

Section "-Shortcuts and silent extraction"
  Call UpdateShortcuts
  ${If} ${Silent}
  ${AndIf} $SkipIso == 0
    StrCpy $0 $IsoPath
    Call PrecheckIso
    ${If} $0 != ""
      DetailPrint "Disc image rejected: $0"
      SetErrorLevel 12
      Return
    ${EndIf}
    Call StartExtract
    ${If} $ExtractState != "running"
      SetErrorLevel 15
      Return
    ${EndIf}
    ${Do}
      System::Call 'kernel32::WaitForSingleObject(p $hProc, i 250) i .r0'
      Call PollExtract
    ${LoopUntil} $ExtractState != "running"
    ${If} $ExtractState == "ok"
      Call UpdateShortcuts
    ${Else}
      IntOp $0 $ExtractCode + 10
      SetErrorLevel $0
    ${EndIf}
  ${EndIf}
SectionEnd

Function ApplyDesktopOption
  ClearErrors
  ${GetOptions} $R0 "/DESKTOP=" $R1
  ${IfNot} ${Errors}
  ${AndIf} $R1 == "0"
    SectionSetFlags ${SecDesktop} 0
  ${EndIf}
FunctionEnd

; (Re)create shortcuts + DisplayIcon.  Uses the icon the port generated from
; the user's disc (gamedata\icon.ico) when present, else the generic icon.
Function UpdateShortcuts
  Push $0
  StrCpy $0 "$INSTDIR\smb2.ico"
  ${If} ${FileExists} "$GameDataDir\icon.ico"
  ${AndIf} $ExtractState == "ok"
    StrCpy $0 "$GameDataDir\icon.ico"
  ${EndIf}
  SetOutPath "$INSTDIR"   ; shortcut working directory
  CreateDirectory "$LnkDir"
  CreateShortcut "$LnkDir\${LNKNAME}" "$INSTDIR\${EXENAME}" "" "$0" 0 SW_SHOWNORMAL "" "Play ${APPNAME} (unofficial PC port)"
  ${If} $WantDesktop == 1
    CreateDirectory "$DeskDir"
    CreateShortcut "$DeskDir\${LNKNAME}" "$INSTDIR\${EXENAME}" "" "$0" 0 SW_SHOWNORMAL "" "Play ${APPNAME} (unofficial PC port)"
  ${EndIf}
  WriteRegStr ${REGROOT} "${UNINSTKEY}" "DisplayIcon" "$0"
  ; installed size incl. the game data in $INSTDIR\gamedata (program sizes are
  ; measured by build.sh; ${GetSize} hangs on some network paths)
  ${If} $Use64 == 1
    StrCpy $0 ${PROG_KB_64}
  ${Else}
    StrCpy $0 ${PROG_KB_32}
  ${EndIf}
  ${If} ${FileExists} "$GameDataDir\manifest.txt"
    IntOp $0 $0 + ${GAMEDATA_KB}
  ${EndIf}
  WriteRegDWORD ${REGROOT} "${UNINSTKEY}" "EstimatedSize" $0
  ; tell Explorer the icons changed
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
  Pop $0
FunctionEnd

; ---------------------------------------------------------------- extraction
; Starts `smb2_pc_port.exe --extract-iso $IsoPath --gamedata $GameDataDir --progress`
; with stdout/stderr redirected to a log file that PollExtract tails.
; (nsExec cannot report output while the child runs, so the System plugin,
; which ships with NSIS, calls CreateProcess directly - no extra plugins.)
Function StartExtract
  StrCpy $ExtractState "failed"
  StrCpy $ExtractCode 5
  StrCpy $ExtractMsg ""
  StrCpy $LastDone 0
  StrCpy $LastTotal 0
  InitPluginsDir
  CreateDirectory "$INSTDIR"
  StrCpy $R1 "$PLUGINSDIR\extract.log"
  Delete $R1
  ; inheritable write handle (SECURITY_ATTRIBUTES x86: 12 bytes)
  System::Call '*(i 12, p 0, i 1) p .R2'
  System::Call 'kernel32::CreateFileW(w R1, i 0x40000000, i 7, p R2, i 2, i 0x80, p 0) p .R3'
  System::Free $R2
  ${If} $R3 == -1
    StrCpy $ExtractMsg "Could not create $R1"
    Return
  ${EndIf}
  ; STARTUPINFOW (x86: 68 bytes): STARTF_USESTDHANDLES|STARTF_USESHOWWINDOW, SW_HIDE
  System::Call '*(i 68, p 0, p 0, p 0, i 0, i 0, i 0, i 0, i 0, i 0, i 0, i 0x101, &i2 0, &i2 0, p 0, p 0, p R3, p R3) p .R4'
  System::Call '*(p 0, p 0, i 0, i 0) p .R5'
  StrCpy $R6 '"$INSTDIR\${EXENAME}" --extract-iso "$IsoPath" --gamedata "$GameDataDir" --progress'
  DetailPrint "Running: $R6"
  ; CREATE_NO_WINDOW (0x08000000): the exe is a console app
  System::Call 'kernel32::CreateProcessW(p 0, w R6, p 0, p 0, i 1, i 0x08000000, p 0, w "$INSTDIR", p R4, p R5) i .R7 ?e'
  Pop $R8
  System::Call 'kernel32::CloseHandle(p R3)'
  ${If} $R7 == 0
    System::Free $R4
    System::Free $R5
    StrCpy $ExtractMsg "Could not start $INSTDIR\${EXENAME} (Windows error $R8)."
    Return
  ${EndIf}
  System::Call '*$R5(p .s, p .R9)'
  Pop $hProc
  System::Call 'kernel32::CloseHandle(p R9)'
  System::Free $R4
  System::Free $R5
  ; reader handle shared with the writer
  System::Call 'kernel32::CreateFileW(w R1, i 0x80000000, i 7, p 0, i 3, i 0x80, p 0) p .s'
  Pop $hLog
  StrCpy $ExtractState "running"
FunctionEnd

; Consume complete lines from the log; update state when the child exits.
Function PollExtract
  ${If} $ExtractState != "running"
    Return
  ${EndIf}
  ; exited?  (check first so we read every line written before exit)
  System::Call 'kernel32::WaitForSingleObject(p $hProc, i 0) i .R0'
  ${If} $hLog != -1
    ${Do}
      FileSeek $hLog 0 CUR $R1
      ClearErrors
      FileRead $hLog $R2
      ${If} ${Errors}
      ${OrIf} $R2 == ""
        ${Break}
      ${EndIf}
      StrCpy $R3 $R2 1 -1
      ${If} $R3 != "$\n"
      ${AndIf} $R0 != 0
        FileSeek $hLog $R1 SET   ; incomplete line, read it next time
        ${Break}
      ${EndIf}
      ${TrimNewLines} $R2 $R2
      ${WordFind} "$R2" " " "+1" $R3
      ${If} $R3 == "PROGRESS"
        ${WordFind} "$R2" " " "+2" $LastDone
        ${WordFind} "$R2" " " "+3" $LastTotal
      ${ElseIf} $R3 == "RESULT"
        ${WordFind} "$R2" " " "+2" $R4
        ${If} $R4 == "ERROR"
          ${WordFind} "$R2" " " "+3}" $ExtractMsg
          ${If} $ExtractMsg == $R2
            StrCpy $ExtractMsg ""
          ${EndIf}
        ${EndIf}
      ${EndIf}
      ${IfNot} ${Silent}
        ${If} $R3 != "PROGRESS"
          DetailPrint "$R2"
        ${EndIf}
      ${EndIf}
    ${Loop}
  ${EndIf}
  Call ShowProgress
  ${If} $R0 == 0   ; WAIT_OBJECT_0: finished
    System::Call 'kernel32::GetExitCodeProcess(p $hProc, *i .R5)'
    System::Call 'kernel32::CloseHandle(p $hProc)'
    System::Call 'kernel32::CloseHandle(p $hLog)'
    StrCpy $hProc 0
    StrCpy $hLog -1
    StrCpy $ExtractCode $R5
    DetailPrint "Game data extraction finished with exit code $R5"
    ${If} $R5 == 0
    ${AndIf} ${FileExists} "$GameDataDir\*.*"
      StrCpy $ExtractState "ok"
    ${Else}
      ${If} $R5 == 0
        StrCpy $ExtractCode 5
        StrCpy $ExtractMsg "The extracted game data folder is missing."
      ${EndIf}
      StrCpy $ExtractState "failed"
    ${EndIf}
  ${EndIf}
FunctionEnd

Function ShowProgress
  ${If} ${Silent}
    Return
  ${EndIf}
  ${If} $ExBar == ""
    Return
  ${EndIf}
  ${If} $LastTotal != 0
  ${AndIf} $LastTotal != ""
    System::Int64Op $LastDone * 1000
    Pop $R6
    System::Int64Op $R6 / $LastTotal
    Pop $R6
    SendMessage $ExBar ${PBM_SETPOS} $R6 0
    System::Int64Op $LastDone / 1048576
    Pop $R6
    System::Int64Op $LastTotal / 1048576
    Pop $R7
    ${NSD_SetText} $ExStatus "$R6 MB of $R7 MB"
  ${EndIf}
FunctionEnd

Function KillExtract
  ${If} $ExtractState == "running"
    System::Call 'kernel32::TerminateProcess(p $hProc, i 1)'
    System::Call 'kernel32::WaitForSingleObject(p $hProc, i 5000)'
    System::Call 'kernel32::CloseHandle(p $hProc)'
    System::Call 'kernel32::CloseHandle(p $hLog)'
    StrCpy $hLog -1
    StrCpy $ExtractState "skipped"
    RMDir /r "$GameDataDir.partial"
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------- extraction page
Function ExtractPageCreate
  ${If} $SkipIso == 1
  ${OrIf} $ExtractState == "ok"
  ${OrIf} $ExtractState == "skipped"
    Abort   ; nothing to do (also when coming Back from the finish page)
  ${EndIf}
  !insertmacro MUI_HEADER_TEXT "Extracting game data" "Copying the game data from your disc image."
  nsDialogs::Create 1018
  Pop $ExDlg
  ${NSD_CreateLabel} 0 0 100% 20u "From:  $IsoPath$\r$\nTo:  $GameDataDir"
  Pop $ExTitle
  ${NSD_CreateProgressBar} 0 26u 100% 12u ""
  Pop $ExBar
  SendMessage $ExBar ${PBM_SETRANGE32} 0 1000
  ${NSD_CreateLabel} 0 42u 100% 10u "Starting..."
  Pop $ExStatus
  ${NSD_CreateLabel} 0 56u 100% 56u ""
  Pop $ExError
  ${NSD_CreateButton} 0 116u 48% 15u "&Choose another disc image..."
  Pop $ExPick
  ${NSD_OnClick} $ExPick ExPickClick
  ${NSD_CreateButton} 52% 116u 48% 15u "&Skip - choose it later in the game"
  Pop $ExSkip
  ${NSD_OnClick} $ExSkip ExSkipClick
  ShowWindow $ExPick ${SW_HIDE}
  ShowWindow $ExSkip ${SW_HIDE}
  GetDlgItem $0 $HWNDPARENT 3   ; Back
  EnableWindow $0 0
  Call ExBegin
  nsDialogs::Show
  ${NSD_KillTimer} ExTimer
FunctionEnd

Function ExBegin
  GetDlgItem $0 $HWNDPARENT 1   ; Next
  EnableWindow $0 0
  ShowWindow $ExPick ${SW_HIDE}
  ShowWindow $ExSkip ${SW_HIDE}
  ${NSD_SetText} $ExError ""
  ${NSD_SetText} $ExTitle "From:  $IsoPath$\r$\nTo:  $GameDataDir"
  ${NSD_SetText} $ExStatus "Starting..."
  SendMessage $ExBar ${PBM_SETPOS} 0 0
  ${If} $IsoPath == ""   ; only after the old game data could not be moved
    StrCpy $ExtractState "failed"
    StrCpy $ExtractCode 0
    StrCpy $ExtractMsg ""
    ${NSD_SetText} $ExTitle "To:  $GameDataDir"
    Call ExFinished
    Return
  ${EndIf}
  Call StartExtract
  ${If} $ExtractState == "running"
    ${NSD_CreateTimer} ExTimer 200
  ${Else}
    Call ExFinished
  ${EndIf}
FunctionEnd

Function ExTimer
  Call PollExtract
  ${If} $ExtractState != "running"
    ${NSD_KillTimer} ExTimer
    Call ExFinished
  ${EndIf}
FunctionEnd

Function ExFinished
  GetDlgItem $0 $HWNDPARENT 1
  ${If} $ExtractState == "ok"
    SendMessage $ExBar ${PBM_SETPOS} 1000 0
    ${NSD_SetText} $ExStatus "Done. The game data was extracted and checked."
    Call UpdateShortcuts
    EnableWindow $0 1
    Return
  ${EndIf}
  ${If} $ExtractCode == 0
    StrCpy $1 "$MigrateMsg Please choose your disc image to extract the game data again."
  ${ElseIf} $ExtractCode == 2
    StrCpy $1 "This disc image is not Super Monkey Ball 2 for the GameCube, USA version (GM2E8P). Other games and other regions (Europe, Japan) are not supported."
  ${ElseIf} $ExtractCode == 3
    StrCpy $1 "The disc image could not be read. It may be damaged or incomplete, or in a format that is not supported (use a plain .iso or .gcm dump of the disc)."
  ${ElseIf} $ExtractCode == 4
    StrCpy $1 "The game data could not be written to $GameDataDir. Make sure the drive has at least ${FREE_NEEDED_MB} MB free and is working."
  ${Else}
    StrCpy $1 "Extracting the game data failed (error $ExtractCode)."
  ${EndIf}
  ${If} $ExtractMsg != ""
    StrCpy $1 "$1$\r$\n$\r$\nDetails: $ExtractMsg"
  ${EndIf}
  ${If} $ExtractCode == 0
    ${NSD_SetText} $ExStatus "No disc image chosen."
  ${Else}
    ${NSD_SetText} $ExStatus "Extraction failed."
  ${EndIf}
  ${NSD_SetText} $ExError "$1$\r$\n$\r$\nChoose another disc image, skip this step (the game asks for the disc image when it starts), or click Cancel."
  ShowWindow $ExPick ${SW_SHOW}
  ShowWindow $ExSkip ${SW_SHOW}
  EnableWindow $0 0
FunctionEnd

Function ExPickClick
  Pop $0
  nsDialogs::SelectFileDialog open "$IsoPath" "GameCube disc image (*.iso;*.gcm)|*.iso;*.gcm|All files (*.*)|*.*"
  Pop $0
  ${If} $0 == ""
  ${OrIf} $0 == "error"
    Return
  ${EndIf}
  StrCpy $IsoPath $0
  Call PrecheckIso
  ${If} $0 != ""
    ${NSD_SetText} $ExError "$0$\r$\n$\r$\nChoose another disc image, skip this step, or click Cancel."
    ${NSD_SetText} $ExTitle "From:  $IsoPath$\r$\nTo:  $GameDataDir"
    Return
  ${EndIf}
  Call ExBegin
FunctionEnd

Function ExSkipClick
  Pop $0
  StrCpy $ExtractState "skipped"
  StrCpy $SkipIso 1
  ShowWindow $ExPick ${SW_HIDE}
  ShowWindow $ExSkip ${SW_HIDE}
  ${NSD_SetText} $ExStatus "Skipped."
  ${NSD_SetText} $ExError "No game data was extracted. When you start ${APPNAME}, it asks for your disc image and extracts it for your user account."
  GetDlgItem $0 $HWNDPARENT 1
  EnableWindow $0 1
FunctionEnd

Function ExtractPageLeave
  ${If} $ExtractState == "running"
    Abort
  ${EndIf}
FunctionEnd

Function OnUserAbortHook
  ${If} $ExtractState == "running"
    MessageBox MB_YESNO|MB_ICONQUESTION "Stop extracting the game data and quit Setup?$\r$\n$\r$\n${APPNAME} is already installed; the game will ask for your disc image when it starts." IDYES +2
    Abort
    Call KillExtract
  ${ElseIf} $ExtractState == "failed"
    MessageBox MB_YESNO|MB_ICONQUESTION "Quit Setup without game data?$\r$\n$\r$\n${APPNAME} is installed; the game will ask for your disc image when it starts. (Use Settings > Apps to uninstall it.)" IDYES +2
    Abort
  ${Else}
    MessageBox MB_YESNO|MB_ICONQUESTION "Are you sure you want to quit ${APPNAME} Setup?" IDYES +2
    Abort
  ${EndIf}
FunctionEnd

; ---------------------------------------------------------------- finish
Function FinishPageShow
  GetDlgItem $0 $HWNDPARENT 3   ; no way back: everything is installed
  EnableWindow $0 0
  ${If} $ExtractState != "ok"
    ; no game data yet: say what happens on first start
    SendMessage $mui.FinishPage.Text ${WM_SETTEXT} 0 "STR:${APPNAME} has been installed on your computer.$\r$\n$\r$\nNo game data was extracted: when you start the game, it asks for your Super Monkey Ball 2 (USA) disc image.$\r$\n$\r$\nClick Finish to close Setup."
  ${EndIf}
FunctionEnd

; Setup runs elevated; start the game as the normal user through Explorer,
; via the shortcut so the working directory is the install dir.
Function LaunchGame
  Exec '"$WINDIR\explorer.exe" "$LnkDir\${LNKNAME}"'
FunctionEnd

Function ShowReadme
  Exec '"$WINDIR\explorer.exe" "$INSTDIR\README.txt"'
FunctionEnd

; ---------------------------------------------------------------- uninstaller
Function un.onInit
  !insertmacro CheckAdmin
  !insertmacro SetRegViewForOS
!ifdef TESTMODE
  ReadRegStr $TestRoot ${REGROOT} "${REGKEY}" "TestRoot"
!endif
  Call un.InitPaths
  Call un.SetGameDataDir   ; $INSTDIR = the uninstaller's folder
FunctionEnd

Section "Uninstall"
  ${WaitGameClosed}
  Delete "$LnkDir\${LNKNAME}"
  Delete "$DeskDir\${LNKNAME}"

  Delete "$INSTDIR\${EXENAME}"
  RMDir /r "$INSTDIR\assets"
  Delete "$INSTDIR\install.ini"
  Delete "$INSTDIR\README.txt"
  Delete "$INSTDIR\LICENSE.txt"
  Delete "$INSTDIR\smb2.ico"
  Delete "$INSTDIR\uninstall.exe"

  ; extracted game data incl. the icon generated from the disc: only a folder
  ; with our manifest.txt is deleted recursively (RemoveGameData)
  StrCpy $0 $GameDataDir
  Call un.RemoveGameData
  ; 1.0.0 layout leftovers in %ProgramData%
  StrCpy $0 $LegacyDir
  Call un.RemoveGameData
  RMDir "$LegacyParent"
  ; per-user copy made by the game's own recovery flow (derived data), also
  ; under the old name if the game has not moved it yet
  StrCpy $0 "$LocalDataDir\gamedata"
  Call un.RemoveGameData
  RMDir "$LocalDataDir"
  StrCpy $0 "$OldLocalDataDir\gamedata"
  Call un.RemoveGameData
  RMDir "$OldLocalDataDir"
  ; game data an upgrade from the old name could not move
  ReadRegStr $1 ${REGROOT} "${REGKEY}" "OldInstallDir"
  ${If} $1 != ""
    StrCpy $0 "$1\gamedata"
    Call un.RemoveGameData
    RMDir "$1"
  ${EndIf}

  RMDir "$INSTDIR"   ; only if empty: never delete files we did not install

  ; saves/settings: ask, default keep
  StrCpy $2 0
  StrCpy $3 ""   ; the folders that exist, for the question
  ${If} ${FileExists} "$UserDataDir\*.*"
    StrCpy $3 "$UserDataDir"
  ${EndIf}
  ${If} ${FileExists} "$OldUserDataDir\*.*"
    ${If} $3 != ""
      StrCpy $3 "$3$\r$\n"
    ${EndIf}
    StrCpy $3 "$3$OldUserDataDir"
  ${EndIf}
  ${GetParameters} $R0
  ClearErrors
  ${GetOptions} $R0 "/REMOVEUSERDATA" $R1
  ${IfNot} ${Errors}
    StrCpy $2 1
  ${ElseIf} $3 != ""
    MessageBox MB_YESNO|MB_ICONQUESTION|MB_DEFBUTTON2 "Also delete your saves and settings?$\r$\n$\r$\n$3$\r$\n$\r$\nChoose No to keep them (recommended if you may install the game again)." /SD IDNO IDNO +2
    StrCpy $2 1
  ${EndIf}
  ${If} $2 == 1
    ; only the installed game's folders: %APPDATA%\SMB2 itself also holds the
    ; portable build's settings (smb2_pc_port\), so it is removed only if empty
    DetailPrint "Removing saves and settings: $UserDataDir"
    RMDir /r "$UserDataDir"
    RMDir "$UserDataParent"
    ${If} ${FileExists} "$OldUserDataDir\*.*"
      DetailPrint "Removing saves and settings: $OldUserDataDir"
      RMDir /r "$OldUserDataDir"
    ${EndIf}
  ${Else}
    DetailPrint "Keeping saves and settings: $UserDataDir"
  ${EndIf}

  DeleteRegKey ${REGROOT} "${UNINSTKEY}"
  DeleteRegKey ${REGROOT} "${REGKEY}"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd
