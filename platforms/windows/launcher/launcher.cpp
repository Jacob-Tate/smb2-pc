// SMB2 friend-build launcher (Windows), adapted from the Party Board launcher.
//
// tools/package_friends.sh appends the whole game build (exe + assets/), zstd-compressed, to this small
// program. On start it unpacks that build once into %LOCALAPPDATA%\SMB2\builds\<build id>\ and runs
// smb2_pc_port.exe from there with the same command line plus `--save-dir %APPDATA%\SMB2\friend`
// (unless the user passed --save-dir). The exit code is forwarded. Older unpacked builds are removed once
// nothing runs them. `SMB2.exe --uninstall` (or Shift held while starting) deletes what this mode stores.
//
// Appended data layout (end of this exe):
//   [zstd frame][u64 archive offset][u64 compressed size][16 chars build id]["SMBLNCH1"]
// Archive (after decompression):
//   "SMBPK001" u32 count, then per file: u16 path length, UTF-8 path ('/' separators), u64 size, bytes.
//
// Test switches: LOCALAPPDATA / APPDATA are read from the environment (known folders only as fallback);
// SMB2_LAUNCHER_ASSUME_YES=1 answers the uninstall confirmation with Yes and suppresses error dialogs.
// Progress lines ("[launcher] ...") go to stdout when it is redirected.

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

// Vendored zstd decoder (third_party/zstd/zstddeclib.c, linked statically).
extern "C" {
size_t ZSTD_decompress(void *dst, size_t dstCapacity, const void *src, size_t compressedSize);
unsigned long long ZSTD_getFrameContentSize(const void *src, size_t srcSize);
unsigned ZSTD_isError(size_t code);
}

namespace {

constexpr char kTrailerMagic[8] = {'S', 'M', 'B', 'L', 'N', 'C', 'H', '1'};
constexpr char kArchiveMagic[8] = {'S', 'M', 'B', 'P', 'K', '0', '0', '1'};
constexpr size_t kTrailerSize = 8 + 8 + 16 + 8;
constexpr size_t kBuildIdLength = 16;
constexpr wchar_t kTitle[] = L"SMB2";
constexpr wchar_t kGameExe[] = L"smb2_pc_port.exe";

bool assume_yes()
{
    wchar_t value[8];
    return GetEnvironmentVariableW(L"SMB2_LAUNCHER_ASSUME_YES", value, 8) > 0 && value[0] == L'1';
}

void logline(const std::wstring &text)
{
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    if (!out || out == INVALID_HANDLE_VALUE) {
        return;
    }
    const std::wstring line = L"[launcher] " + text + L"\n";
    const int n = WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string utf8(static_cast<size_t>(n > 0 ? n - 1 : 0), '\0');
    WideCharToMultiByte(CP_UTF8, 0, line.c_str(), -1, utf8.data(), n, nullptr, nullptr);
    DWORD wrote = 0;
    WriteFile(out, utf8.data(), static_cast<DWORD>(utf8.size()), &wrote, nullptr);
}

[[noreturn]] void fail(const std::wstring &message)
{
    logline(L"error: " + message);
    if (!assume_yes()) {
        MessageBoxW(nullptr, message.c_str(), kTitle, MB_OK | MB_ICONERROR);
    }
    ExitProcess(1);
}

std::wstring last_error_text()
{
    const DWORD code = GetLastError();
    wchar_t *buffer = nullptr;
    FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                   nullptr, code, 0, reinterpret_cast<wchar_t *>(&buffer), 0, nullptr);
    std::wstring text = buffer ? buffer : L"";
    LocalFree(buffer);
    while (!text.empty() && (text.back() == L'\n' || text.back() == L'\r' || text.back() == L' ')) {
        text.pop_back();
    }
    return text + L" (error " + std::to_wstring(code) + L")";
}

std::wstring widen(const char *text, size_t length)
{
    const int count = MultiByteToWideChar(CP_UTF8, 0, text, static_cast<int>(length), nullptr, 0);
    std::wstring result(static_cast<size_t>(count), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text, static_cast<int>(length), result.data(), count);
    return result;
}

bool read_at(HANDLE file, uint64_t offset, void *out, size_t size)
{
    LARGE_INTEGER position;
    position.QuadPart = static_cast<LONGLONG>(offset);
    if (!SetFilePointerEx(file, position, nullptr, FILE_BEGIN)) {
        return false;
    }
    auto *dst = static_cast<uint8_t *>(out);
    while (size > 0) {
        DWORD chunk = static_cast<DWORD>(size > (1u << 30) ? (1u << 30) : size);
        DWORD got = 0;
        if (!ReadFile(file, dst, chunk, &got, nullptr) || got == 0) {
            return false;
        }
        dst += got;
        size -= got;
    }
    return true;
}

bool write_file(const std::wstring &path, const uint8_t *data, uint64_t size)
{
    HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return false;
    }
    bool ok = true;
    while (size > 0 && ok) {
        DWORD chunk = static_cast<DWORD>(size > (1u << 30) ? (1u << 30) : size);
        DWORD wrote = 0;
        ok = WriteFile(file, data, chunk, &wrote, nullptr) && wrote == chunk;
        data += chunk;
        size -= chunk;
    }
    return CloseHandle(file) && ok;
}

bool exists(const std::wstring &path)
{
    return GetFileAttributesW(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

void create_parents(const std::wstring &path)
{
    for (size_t i = 0; i < path.size(); i++) {
        if ((path[i] == L'\\' || path[i] == L'/') && i > 2) {
            CreateDirectoryW(path.substr(0, i).c_str(), nullptr);
        }
    }
}

// Best effort: a folder still in use (a game from that build running) just stays. Links are removed, never
// followed.
void delete_tree(const std::wstring &dir)
{
    const DWORD attrs = GetFileAttributesW(dir.c_str());
    if (attrs != INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_REPARSE_POINT)) {
        RemoveDirectoryW(dir.c_str());
        return;
    }
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW((dir + L"\\*").c_str(), &data);
    if (find != INVALID_HANDLE_VALUE) {
        do {
            const std::wstring name = data.cFileName;
            if (name == L"." || name == L"..") {
                continue;
            }
            const std::wstring child = dir + L"\\" + name;
            if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
                !(data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)) {
                delete_tree(child);
            } else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                RemoveDirectoryW(child.c_str());
            } else {
                SetFileAttributesW(child.c_str(), FILE_ATTRIBUTE_NORMAL);
                DeleteFileW(child.c_str());
            }
        } while (FindNextFileW(find, &data));
        FindClose(find);
    }
    RemoveDirectoryW(dir.c_str());
}

// Removes older versions (and their leftover unpack folders). Runs on every start: a version that was still
// running when an update arrived is cleaned up the next time. Folders of this build id are left alone, so a
// second launcher unpacking the same build at the same moment isn't disturbed.
void delete_other_builds(const std::wstring &buildsDir, const std::wstring &keep)
{
    WIN32_FIND_DATAW data;
    HANDLE find = FindFirstFileW((buildsDir + L"\\*").c_str(), &data);
    if (find == INVALID_HANDLE_VALUE) {
        return;
    }
    do {
        const std::wstring name = data.cFileName;
        if (name == L"." || name == L".." || name.compare(0, keep.size(), keep) == 0 ||
            !(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
            continue;
        }
        // A running game's exe can't be deleted. Leave that version whole.
        const std::wstring dir = buildsDir + L"\\" + name;
        const std::wstring exe = dir + L"\\" + kGameExe;
        if (exists(exe) && !DeleteFileW(exe.c_str())) {
            continue;
        }
        logline(L"removing old build " + dir);
        delete_tree(dir);
    } while (FindNextFileW(find, &data));
    FindClose(find);
}

// Environment first (so tests can redirect everything), known folder as fallback.
std::wstring app_folder(const wchar_t *envName, REFKNOWNFOLDERID id)
{
    wchar_t buffer[MAX_PATH * 4];
    const DWORD n = GetEnvironmentVariableW(envName, buffer, static_cast<DWORD>(std::size(buffer)));
    if (n > 0 && n < std::size(buffer)) {
        std::wstring result = buffer;
        while (result.size() > 3 && (result.back() == L'\\' || result.back() == L'/')) {
            result.pop_back();
        }
        return result;
    }
    PWSTR path = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, 0, nullptr, &path))) {
        fail(L"Couldn't find your app data folder.");
    }
    std::wstring result = path;
    CoTaskMemFree(path);
    return result;
}

// Everything this mode stores on the PC. Nothing else is ever deleted.
std::wstring builds_root() { return app_folder(L"LOCALAPPDATA", FOLDERID_LocalAppData) + L"\\SMB2\\builds"; }
std::wstring friend_save_dir() { return app_folder(L"APPDATA", FOLDERID_RoamingAppData) + L"\\SMB2\\friend"; }

// SMB2.exe --uninstall (or Shift held while starting it).
int uninstall()
{
    const std::wstring folders[] = {builds_root(), friend_save_dir()};
    std::wstring list;
    for (const auto &folder : folders) {
        if (exists(folder)) {
            list += L"\n  " + folder;
        }
    }
    if (list.empty()) {
        logline(L"nothing to remove");
        if (!assume_yes()) {
            MessageBoxW(nullptr, L"SMB2 (this single-file version) has nothing stored on this PC.", kTitle,
                        MB_OK | MB_ICONINFORMATION);
        }
        return 0;
    }
    const std::wstring question =
        L"Delete everything this SMB2.exe stores on this PC?\n\nThis removes the unpacked game and your "
        L"settings, memory card and logs from this version:\n" +
        list +
        L"\n\nThe installed SMB2 (installer) and the portable zip version keep their own data in other "
        L"folders: that is NOT touched. Neither is your disc image or this SMB2.exe. Close the game first.";
    if (!assume_yes() &&
        MessageBoxW(nullptr, question.c_str(), kTitle, MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2) != IDYES) {
        return 0;
    }
    std::wstring leftovers;
    for (const auto &folder : folders) {
        logline(L"deleting " + folder);
        delete_tree(folder);
        if (exists(folder)) {
            leftovers += L"\n  " + folder;
        }
    }
    if (!leftovers.empty()) {
        logline(L"could not delete everything:" + leftovers);
        if (!assume_yes()) {
            MessageBoxW(nullptr,
                        (L"Some files couldn't be deleted (is the game still running?). Close it and try again, "
                         L"or delete these folders yourself:" +
                         leftovers)
                            .c_str(),
                        kTitle, MB_OK | MB_ICONWARNING);
        }
        return 1;
    }
    if (!assume_yes()) {
        MessageBoxW(nullptr, L"Done. The data of this SMB2 version has been removed from this PC.", kTitle,
                    MB_OK | MB_ICONINFORMATION);
    }
    return 0;
}

bool has_arg(const wchar_t *name)
{
    int count = 0;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &count);
    bool found = false;
    for (int i = 1; argv && i < count; i++) {
        found |= _wcsicmp(argv[i], name) == 0;
    }
    LocalFree(argv);
    return found;
}

bool wants_uninstall()
{
    return (GetKeyState(VK_SHIFT) & 0x8000) != 0 || has_arg(L"--uninstall");
}

void unpack(HANDLE self, uint64_t offset, uint64_t size, const std::wstring &target)
{
    std::vector<uint8_t> compressed(static_cast<size_t>(size));
    if (!read_at(self, offset, compressed.data(), compressed.size())) {
        fail(L"Couldn't read the game data inside this file: " + last_error_text());
    }
    const unsigned long long rawSize = ZSTD_getFrameContentSize(compressed.data(), compressed.size());
    if (rawSize == 0 || rawSize >= (3ull << 30)) {
        fail(L"The game data inside this file is damaged. Please download it again.");
    }
    std::vector<uint8_t> archive(static_cast<size_t>(rawSize));
    const size_t got = ZSTD_decompress(archive.data(), archive.size(), compressed.data(), compressed.size());
    if (ZSTD_isError(got) || got != archive.size()) {
        fail(L"The game data inside this file is damaged. Please download it again.");
    }
    compressed = {};

    const uint8_t *p = archive.data();
    const uint8_t *end = p + archive.size();
    auto take = [&](void *out, size_t n) {
        if (static_cast<size_t>(end - p) < n) {
            fail(L"The game data inside this file is damaged. Please download it again.");
        }
        std::memcpy(out, p, n);
        p += n;
    };
    char magic[8];
    take(magic, sizeof(magic));
    if (std::memcmp(magic, kArchiveMagic, sizeof(magic)) != 0) {
        fail(L"The game data inside this file is damaged. Please download it again.");
    }
    uint32_t count = 0;
    take(&count, sizeof(count));
    for (uint32_t i = 0; i < count; i++) {
        uint16_t pathLength = 0;
        take(&pathLength, sizeof(pathLength));
        std::string path(pathLength, '\0');
        take(path.data(), pathLength);
        uint64_t fileSize = 0;
        take(&fileSize, sizeof(fileSize));
        if (static_cast<uint64_t>(end - p) < fileSize || path.find("..") != std::string::npos) {
            fail(L"The game data inside this file is damaged. Please download it again.");
        }
        std::wstring out = target + L"\\" + widen(path.data(), path.size());
        for (auto &c : out) {
            if (c == L'/') {
                c = L'\\';
            }
        }
        create_parents(out);
        if (!write_file(out, p, fileSize)) {
            fail(L"Couldn't unpack the game to\n" + target + L"\n\n" + last_error_text());
        }
        p += fileSize;
    }
}

// The build id is whatever the packager stored in the trailer (16 chars); F2 can change what goes there
// without touching the launcher.
std::wstring read_build_id(const uint8_t *trailer)
{
    return widen(reinterpret_cast<const char *>(trailer + 16), kBuildIdLength);
}

} // namespace

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int)
{
    if (wants_uninstall()) {
        return uninstall();
    }

    wchar_t selfPath[MAX_PATH * 4];
    const DWORD selfLength = GetModuleFileNameW(nullptr, selfPath, static_cast<DWORD>(std::size(selfPath)));
    if (selfLength == 0 || selfLength >= std::size(selfPath)) {
        fail(L"Couldn't find this program's own file.");
    }
    HANDLE self = CreateFileW(selfPath, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (self == INVALID_HANDLE_VALUE) {
        fail(L"Couldn't open this program's own file: " + last_error_text());
    }
    LARGE_INTEGER fileSize{};
    GetFileSizeEx(self, &fileSize);
    uint8_t trailer[kTrailerSize];
    if (static_cast<uint64_t>(fileSize.QuadPart) < kTrailerSize ||
        !read_at(self, static_cast<uint64_t>(fileSize.QuadPart) - kTrailerSize, trailer, kTrailerSize) ||
        std::memcmp(trailer + kTrailerSize - 8, kTrailerMagic, 8) != 0) {
        fail(L"This launcher has no game inside it (it must be built with tools/package_friends.sh).");
    }
    uint64_t offset = 0;
    uint64_t size = 0;
    std::memcpy(&offset, trailer, 8);
    std::memcpy(&size, trailer + 8, 8);
    const std::wstring buildId = read_build_id(trailer);

    const std::wstring buildsDir = builds_root();
    const std::wstring buildDir = buildsDir + L"\\" + buildId;
    const std::wstring exePath = buildDir + L"\\" + kGameExe;
    const std::wstring saveDir = friend_save_dir();

    if (!exists(buildDir + L"\\.complete")) {
        logline(L"unpacking build " + buildId + L" to " + buildDir);
        create_parents(buildDir + L"\\");
        // Unpack beside the final folder and rename it into place, so two copies started at once (or one
        // interrupted half way) never leave a half-written build that looks finished.
        const std::wstring staging =
            buildsDir + L"\\" + buildId + L".unpacking-" + std::to_wstring(GetCurrentProcessId());
        delete_tree(staging);
        CreateDirectoryW(staging.c_str(), nullptr);
        unpack(self, offset, size, staging);
        write_file(staging + L"\\.complete", reinterpret_cast<const uint8_t *>("ok"), 2);
        if (exists(buildDir) && !exists(buildDir + L"\\.complete")) {
            delete_tree(buildDir);
        }
        if (!MoveFileExW(staging.c_str(), buildDir.c_str(), 0)) {
            delete_tree(staging); // another copy of the launcher finished first
            if (!exists(buildDir + L"\\.complete")) {
                fail(L"Couldn't unpack the game to\n" + buildDir + L"\n\n" + last_error_text());
            }
        }
        logline(L"unpacked");
    } else {
        logline(L"build " + buildId + L" already unpacked, skipping");
    }
    CloseHandle(self);
    delete_other_builds(buildsDir, buildId);

    // Pass our command line on unchanged (minus our own program name).
    const wchar_t *args = GetCommandLineW();
    if (*args == L'"') {
        args++;
        while (*args && *args != L'"') {
            args++;
        }
        if (*args) {
            args++;
        }
    } else {
        while (*args && *args != L' ' && *args != L'\t') {
            args++;
        }
    }
    std::wstring commandLine = L"\"" + exePath + L"\"" + args;
    if (!has_arg(L"--save-dir")) {
        create_parents(saveDir + L"\\");
        commandLine += L" --save-dir \"" + saveDir + L"\"";
    }

    // Hand our standard handles on when they exist (redirected output, tests); a double-click has none.
    STARTUPINFOW startup{};
    startup.cb = sizeof(startup);
    HANDLE in = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    HANDLE err = GetStdHandle(STD_ERROR_HANDLE);
    const bool inherit = out && out != INVALID_HANDLE_VALUE;
    if (inherit) {
        for (HANDLE h : {in, out, err}) {
            if (h && h != INVALID_HANDLE_VALUE) {
                SetHandleInformation(h, HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);
            }
        }
        startup.dwFlags = STARTF_USESTDHANDLES;
        startup.hStdInput = in;
        startup.hStdOutput = out;
        startup.hStdError = err ? err : out;
    }
    PROCESS_INFORMATION process{};
    if (!CreateProcessW(exePath.c_str(), commandLine.data(), nullptr, nullptr, inherit ? TRUE : FALSE, 0, nullptr,
                        buildDir.c_str(), &startup, &process)) {
        fail(L"Couldn't start the game:\n" + exePath + L"\n\n" + last_error_text());
    }
    AllowSetForegroundWindow(process.dwProcessId);
    CloseHandle(process.hThread);
    WaitForSingleObject(process.hProcess, INFINITE);
    DWORD exitCode = 0;
    GetExitCodeProcess(process.hProcess, &exitCode);
    CloseHandle(process.hProcess);
    return static_cast<int>(exitCode);
}
