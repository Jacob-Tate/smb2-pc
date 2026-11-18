// Online tab (E7): lobby UI on top of the non-blocking lobby API in src/netplay/netplay.h.
//
// States: disconnected -> connected (host / join) -> in a room (ports, delay, start) -> session running (info +
// Leave).  Every button calls one of the online_*() action functions below; the test hook
// --overlay-test-online-script drives the very same functions (docs/handoff/E7.md).
// online_tick() runs once per platform_poll_events (overlay_poll_tick): it turns the lobby's event queue into
// toasts (also while the overlay is closed), runs the test script and performs a deferred "leave session".
#include "overlay_ui.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "imgui.h"

#include "overlay.h"
#include "pc_settings.h"
extern "C" {
#include "netplay/netplay.h"
}

extern "C" long g_dbg_frame_counter;
extern "C" void platform_set_dump_ranges(const char* spec);

namespace ovl {

namespace {

char s_server[96];
char s_name[NETPLAY_NAME_MAX + 1];
char s_code[5];
int  s_delay_sel = -1;          // -1 = auto from the pings, else polls (frames)
bool s_inited;
bool s_confirm_leave;           // "Leave session?" confirmation is showing
bool s_leave_pending;           // leave the session at the next online_tick (not from inside an ImGui frame)

const int kDelayMin = 2, kDelayMax = 12;

void init_once() {
    if (s_inited) return;
    s_inited = true;
    snprintf(s_server, sizeof s_server, "%s", g_pcs.np_server[0] ? g_pcs.np_server : NETPLAY_DEFAULT_SERVER);
    snprintf(s_name, sizeof s_name, "%s", g_pcs.np_name[0] ? g_pcs.np_name : "Player");
}

void trim(char* s) {
    size_t n = strlen(s), a = 0;
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = 0;
    while (s[a] && isspace((unsigned char)s[a])) a++;
    if (a) memmove(s, s + a, strlen(s + a) + 1);
}

bool lobby_joined(NetplayLobbyState st) {
    return st == NP_LOBBY_JOINING || st == NP_LOBBY_IN_ROOM || st == NP_LOBBY_STARTING || st == NP_LOBBY_RESUMING ||
           st == NP_LOBBY_IN_GAME;
}

void delay_text(char* out, size_t n, int d, int suggested) {
    if (d < 0) snprintf(out, n, "Auto (%d frames, %d ms)", suggested, suggested * 1000 / 60);
    else snprintf(out, n, "%d frames (%d ms)", d, d * 1000 / 60);
}

void ping_text(char* out, size_t n, int ms) {
    if (ms > 0) snprintf(out, n, "%d ms", ms);
    else snprintf(out, n, "-");
}

// Big room code panel.
void code_banner(const char* code) {
    const float u = ImGui::GetFontSize();
    const float w = ImGui::GetContentRegionAvail().x;
    const float h = u * 5.0f;
    const ImVec2 p = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(p, ImVec2(p.x + w, p.y + h), IM_COL32(0, 0, 0, 70), 10.0f);
    dl->AddRect(p, ImVec2(p.x + w, p.y + h), IM_COL32(0x00, 0x9d, 0xda, 160), 10.0f, 0, 1.5f);
    dl->AddText(ImVec2(p.x + u * 0.9f, p.y + u * 0.5f), IM_COL32(0xE0, 0xDB, 0xC8, 150), "ROOM CODE");
    const float fs = u * 3.0f, cell = u * 3.0f;
    const float x0 = p.x + (w - cell * 4) * 0.5f;
    ImFont* font = ImGui::GetFont();
    for (int i = 0; i < 4 && code[i]; i++) {
        char ch[2] = { code[i], 0 };
        const ImVec2 sz = font->CalcTextSizeA(fs, 10000.0f, 0.0f, ch);
        const ImVec2 cp(x0 + cell * i + (cell - sz.x) * 0.5f, p.y + (h - sz.y) * 0.5f + u * 0.5f);
        dl->AddRectFilled(ImVec2(x0 + cell * i + 4, p.y + u * 1.4f), ImVec2(x0 + cell * (i + 1) - 4, p.y + h - u * 0.5f),
                          IM_COL32(255, 255, 255, 18), 8.0f);
        dl->AddText(font, fs, cp, IM_COL32(0xFF, 0xD9, 0x5A, 255), ch);
    }
    ImGui::Dummy(ImVec2(w, h));
    ImGui::Dummy(ImVec2(0, u * 0.3f));
}

// P1..P4 rows.  `pick`: activating a row moves yourself there.
void port_rows(const NetplayRoomInfo& ri, bool pick) {
    for (int port = 0; port < 4; port++) {
        char label[16], value[96] = "", help[260];
        snprintf(label, sizeof label, "Player %d", port + 1);
        const NetplayPlayer* who = nullptr;
        for (int i = 0; i < ri.count; i++)
            if (ri.players[i].port == port) who = &ri.players[i];
        bool gold = false;
        if (who) {
            char ping[24];
            ping_text(ping, sizeof ping, who->ping_ms);
            snprintf(value, sizeof value, "%s%s%s   %s", who->name, who->is_host ? "  [host]" : "", who->is_self ? "  (you)" : "", ping);
            gold = who->is_self;
        } else {
            snprintf(value, sizeof value, pick ? "free - press to take this port" : "free");
        }
        if (pick)
            snprintf(help, sizeof help,
                "Controller port %d of the game. Each player owns one port; the game's own menus then use it. "
                "Press to take this port if it is free.", port + 1);
        else
            snprintf(help, sizeof help, "Controller port %d and who plays on it.", port + 1);
        if (!pick) info_row(label, value, help);
        else if (action_row(label, value, help, !who || !who->is_self, gold)) online_pick_port(port);
    }
    for (int i = 0; i < ri.count; i++) {
        if (ri.players[i].port >= 0) continue;
        char t[96];
        snprintf(t, sizeof t, "%s is in the room without a port.", ri.players[i].name);
        note(t);
    }
}

void delay_row(const NetplayRoomInfo* ri) {
    char v[64];
    delay_text(v, sizeof v, s_delay_sel, ri ? ri->suggested_delay : 2);
    const int step = stepper_row("Input delay", v,
        "Frames of delay between pressing a button and the game reacting, the price of lockstep netplay. "
        "Auto picks it from the players' pings. Raise it if the game stutters or stalls; the host decides for everyone.");
    if (step) online_step_delay(step);
}

void tab_disconnected() {
    section("Server");
    text_row("Server address", s_server, (int)sizeof s_server,
        "Address of the relay server: host or host:port (default port 7855). Remembered for next time. "
        "Press A or click, then type; Enter finishes.");
    text_row("Your name", s_name, (int)sizeof s_name,
        "The name other players see. Remembered. Type with the keyboard.");
    if (button_row("Connect", nullptr, "Connect to the server. Then you can host a room or join one with its 4-letter code."))
        online_connect();
    note("Everyone playing needs the same game build and disc image.");
}

void tab_connecting() {
    section("Connecting");
    if (button_row("Cancel", nullptr, "Stop connecting.")) online_disconnect();
}

void tab_ready() {
    section("Host a room");
    delay_row(nullptr);
    if (button_row("Create room", nullptr,
            "Create a room and show its 4-letter code. Tell your friends the code; they use Join with code. You pick the ports and start the game."))
        online_host();

    section("Join a room");
    text_row("Room code", s_code, (int)sizeof s_code,
        "The 4 letters the host sees. Type or paste (Ctrl+V) with the keyboard. On a gamepad use the letter rows below.", true);
    for (int i = 0; i < 4; i++) {
        char label[24], v[8];
        snprintf(label, sizeof label, "Code letter %d", i + 1);
        v[0] = (size_t)i < strlen(s_code) ? s_code[i] : '_';
        v[1] = 0;
        const int step = stepper_row(label, v,
            "Gamepad entry: Left / Right (or A) change the letter. Going past Z on the last letter removes it.",
            (size_t)i <= strlen(s_code));
        if (step) online_code_letter(i, step);
    }
    if (button_row("Join room", nullptr, "Join the room with this code.", strlen(s_code) == 4)) online_join();

    section("Connection");
    if (button_row("Disconnect", nullptr, "Close the connection to the server.")) online_disconnect();
}

void tab_room(const NetplayRoomInfo& ri) {
    section("Room");
    code_banner(ri.code);
    if (action_row("Copy room code", nullptr, "Copy the 4-letter code to the clipboard so you can paste it into a message.")) {
        ImGui::SetClipboardText(ri.code);
        overlay_toast(OVERLAY_TOAST_SUCCESS, "Room code", "Copied to the clipboard");
    }

    section("Players");
    port_rows(ri, true);

    section(ri.self_is_host ? "Start" : "Waiting");
    if (ri.self_is_host) {
        delay_row(&ri);
        const bool can = ri.count >= 2 && netplay_lobby_server_can_start();
        const char* val = ri.count < 2 ? "needs 2 players" : nullptr;
        if (button_row("Start game", val,
                "Everyone's game restarts into the online session and the lockstep starts from the title screen. "
                "Needs at least 2 players.", can))
            online_start();
        if (!netplay_lobby_server_can_start())
            note("This server is too old to start a game from inside the game. Update the server.");
    } else {
        char v[64];
        delay_text(v, sizeof v, ri.delay, ri.suggested_delay);
        info_row("Input delay", ri.delay < 0 ? "chosen by the host at start" : v, "The host decides the input delay for everyone.");
        info_row("Status", "Waiting for the host to start", "The game starts when the host presses Start.");
    }
    if (button_row("Leave room", nullptr, "Leave this room but stay connected to the server.")) online_leave_room();
    if (button_row("Disconnect", nullptr, "Leave the room and close the connection.")) online_disconnect();
}

void tab_session(const NetplayRoomInfo* ri) {
    section("Session");
    if (ri) {
        char v[64];
        info_row("Room", ri->code, "The room this online session belongs to.");
        delay_text(v, sizeof v, ri->delay, ri->delay);
        info_row("Input delay", v, "Fixed for the whole session by the host.");
        port_rows(*ri, false);
    } else {
        note("Online session running (no room details available).");
    }
    section("");
    if (!s_confirm_leave) {
        if (button_row("Leave session", nullptr,
                "Leave the online game. The others continue without you. Your game restarts into normal offline play."))
            s_confirm_leave = true;
    } else {
        if (button_row("Yes, leave the session", nullptr, "Leave now.")) { s_confirm_leave = false; online_leave_session(); }
        if (button_row("No, keep playing", nullptr, "Cancel.")) s_confirm_leave = false;
    }
}

}  // namespace

// ---------------------------------------------------------------- actions (buttons and the test script)

bool online_locked() {
    return netplay_active() || lobby_joined(netplay_lobby_state());
}

void online_set_server_name(const char* server, const char* name) {
    init_once();
    if (server && server[0]) snprintf(s_server, sizeof s_server, "%s", server);
    if (name && name[0]) snprintf(s_name, sizeof s_name, "%s", name);
}

void online_connect() {
    init_once();
    trim(s_server);
    trim(s_name);
    if (!s_server[0]) snprintf(s_server, sizeof s_server, "%s", NETPLAY_DEFAULT_SERVER);
    if (!s_name[0]) snprintf(s_name, sizeof s_name, "Player");
    if (strcmp(s_server, g_pcs.np_server) != 0 || strcmp(s_name, g_pcs.np_name) != 0) pcs_set_netplay(s_server, s_name);
    netplay_lobby_connect(s_server, s_name);
}

void online_disconnect() {
    s_confirm_leave = false;
    netplay_lobby_disconnect();
}

void online_host() { netplay_lobby_create(s_delay_sel); }

void online_set_code(const char* code) {
    int n = 0;
    for (const char* c = code; c && *c && n < 4; c++)
        if (isalpha((unsigned char)*c)) s_code[n++] = (char)toupper((unsigned char)*c);
    s_code[n] = 0;
}

void online_code_letter(int i, int step) {
    const int len = (int)strlen(s_code);
    if (i < 0 || i > 3 || i > len) return;
    if (i == len) { s_code[i] = step > 0 ? 'A' : 'Z'; s_code[i + 1] = 0; return; }
    int c = s_code[i] - 'A' + step;
    if (c < 0 || c > 25) {
        if (i == len - 1) { s_code[i] = 0; return; }
        c = (c + 26) % 26;
    }
    s_code[i] = (char)('A' + c);
}

void online_join() { netplay_lobby_join(s_code); }
void online_pick_port(int port) { netplay_lobby_set_port(port); }
void online_start() { netplay_lobby_start(); }
void online_leave_room() { netplay_lobby_leave(); }
void online_leave_session() { s_leave_pending = true; }

void online_step_delay(int step) {
    int v = s_delay_sel < 0 ? kDelayMin - 1 : s_delay_sel;   // auto sits below the minimum
    v += step;
    if (v < kDelayMin - 1) v = kDelayMax;
    if (v > kDelayMax) v = kDelayMin - 1;
    s_delay_sel = v < kDelayMin ? -1 : v;
    netplay_lobby_set_delay(s_delay_sel);
}

// ---------------------------------------------------------------- test script (--overlay-test-online-script)

namespace {

struct Step { long tick; char verb[16]; char arg[300]; };
Step s_steps[32];
int s_nsteps, s_step_pos;
long s_script_tick;
char s_script_copy[2048];

}  // namespace

// "T:connect=HOST:PORT/NAME,T:host=DELAY,T:join=CODE|@FILE,T:codefile=FILE,T:port=P,T:start=N,T:leave,
//  T:disconnect,T:leave_session" with T = tick (platform_poll_events calls since start).  A step that has to
// wait (room not there yet, code file empty, too few players) blocks the following ones.
bool online_script_set(const char* spec) {
    snprintf(s_script_copy, sizeof s_script_copy, "%s", spec);
    s_nsteps = s_step_pos = 0;
    char* save = nullptr;
    for (char* p = strtok_r(s_script_copy, ",", &save); p && s_nsteps < 32; p = strtok_r(nullptr, ",", &save)) {
        char* colon = strchr(p, ':');
        if (!colon) return false;
        *colon = 0;
        Step& st = s_steps[s_nsteps];
        st.tick = atol(p);
        char* eq = strchr(colon + 1, '=');
        if (eq) *eq++ = 0;
        snprintf(st.verb, sizeof st.verb, "%s", colon + 1);
        snprintf(st.arg, sizeof st.arg, "%s", eq ? eq : "");
        s_nsteps++;
    }
    return s_nsteps > 0;
}

static void script_run() {
    s_script_tick++;
    if (netplay_active()) return;
    while (s_step_pos < s_nsteps) {
        Step& st = s_steps[s_step_pos];
        if (s_script_tick < st.tick) break;
        NetplayRoomInfo ri;
        const bool in_room = netplay_lobby_room(&ri) != 0;
        if (!strcmp(st.verb, "connect")) {
            char arg[300];
            snprintf(arg, sizeof arg, "%s", st.arg);
            char* slash = strchr(arg, '/');
            if (slash) *slash++ = 0;
            online_set_server_name(arg, slash);
            online_connect();
        } else if (!strcmp(st.verb, "host")) {
            if (netplay_lobby_state() != NP_LOBBY_READY) return;
            if (st.arg[0]) s_delay_sel = atoi(st.arg);
            online_host();
        } else if (!strcmp(st.verb, "join")) {
            if (netplay_lobby_state() != NP_LOBBY_READY) return;
            char code[16] = "";
            if (st.arg[0] == '@') {
                if (FILE* f = fopen(st.arg + 1, "rb")) {
                    if (fgets(code, sizeof code, f)) code[strcspn(code, "\r\n")] = 0;
                    fclose(f);
                }
                if (strlen(code) != 4) return;   // not written yet
            } else {
                snprintf(code, sizeof code, "%s", st.arg);
            }
            online_set_code(code);
            online_join();
        } else if (!strcmp(st.verb, "codefile")) {
            if (!in_room) return;
            if (FILE* f = fopen(st.arg, "wb")) { fprintf(f, "%s\n", ri.code); fclose(f); }
        } else if (!strcmp(st.verb, "port")) {
            if (!in_room) return;
            online_pick_port(atoi(st.arg));
        } else if (!strcmp(st.verb, "start")) {
            if (!in_room || !ri.self_is_host || ri.count < atoi(st.arg)) return;
            online_start();
        } else if (!strcmp(st.verb, "leave")) {
            online_leave_room();
        } else if (!strcmp(st.verb, "disconnect")) {
            online_disconnect();
        } else if (!strcmp(st.verb, "leave_session")) {
            online_leave_session();
        } else if (!strcmp(st.verb, "wait")) {   // block the following steps for N ticks from now
            static long s_wait_until = -1;
            if (s_wait_until < 0) s_wait_until = s_script_tick + atol(st.arg);
            if (s_script_tick < s_wait_until) return;
            s_wait_until = -1;
        } else if (!strcmp(st.verb, "players")) {   // block until the room has at least N players
            if (!in_room || ri.count < atoi(st.arg)) return;
        } else if (!strcmp(st.verb, "open")) {
            overlay_test_open_frames(-1, -1);   // the script owns the overlay now (cancels --overlay-open-frames)
            overlay_set_open(true);
        } else if (!strcmp(st.verb, "close")) {
            overlay_test_open_frames(-1, -1);
            overlay_set_open(false);
        } else if (!strcmp(st.verb, "tab")) {
            select_tab(st.arg);
        } else if (!strcmp(st.verb, "dump")) {   // frame dump (with the overlay) of a frame a few swaps from now: frame_NNNNN.ppm in the cwd
            const long f = g_dbg_frame_counter + 4;
            char spec[32];
            snprintf(spec, sizeof spec, "%ld-%ld", f, f);
            platform_set_dump_ranges(spec);
            printf("[overlay-test-online] dump '%s' = frame %ld\n", st.arg, f);
        } else {
            fprintf(stderr, "[overlay-test-online] unknown step '%s'\n", st.verb);
        }
        printf("[overlay-test-online] tick %ld: %s %s\n", s_script_tick, st.verb, st.arg);
        fflush(stdout);
        s_step_pos++;
    }
}

// Once per platform_poll_events (overlay_poll_tick).
void online_tick() {
    script_run();
    // START_GAME arrived: the exe is about to restart.  An open overlay pauses the game loop, which would then never quit.
    if (netplay_lobby_state() == NP_LOBBY_STARTING && overlay_is_open()) overlay_set_open(false);
    if (s_leave_pending) {
        s_leave_pending = false;
        netplay_session_leave();
    }
    NetplayEvent ev;
    while (netplay_lobby_next_event(&ev)) {
        const int type = ev.kind == NP_EV_ERROR ? OVERLAY_TOAST_ERROR : ev.kind == NP_EV_WARN ? OVERLAY_TOAST_WARNING
                                                                                             : OVERLAY_TOAST_INFO;
        overlay_toast(type, ev.kind == NP_EV_ERROR ? "Online error" : "Online", ev.text);
    }
}

// ---------------------------------------------------------------- the tab

void tab_online() {
    init_once();
    const NetplayLobbyState st = netplay_lobby_state();
    NetplayRoomInfo ri;
    const bool have_room = netplay_lobby_room(&ri) != 0;

    section("Online play");
    info_row("Status", netplay_lobby_status(),
        "What the online lobby is doing right now. Messages about joins, leaves and errors also pop up as notices in the game.");

    if (netplay_active()) { tab_session(have_room ? &ri : nullptr); return; }
    s_confirm_leave = false;
    switch (st) {
    case NP_LOBBY_OFFLINE: tab_disconnected(); break;
    case NP_LOBBY_CONNECTING: tab_connecting(); break;
    case NP_LOBBY_READY: tab_ready(); break;
    case NP_LOBBY_IN_ROOM:
        if (have_room) tab_room(ri);
        break;
    case NP_LOBBY_JOINING:
    case NP_LOBBY_STARTING:
    case NP_LOBBY_RESUMING:
        section("Please wait");
        if (st == NP_LOBBY_JOINING && button_row("Cancel", nullptr, "Give up and disconnect.")) online_disconnect();
        break;
    default: break;
    }
}

}  // namespace ovl
