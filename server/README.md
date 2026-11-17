# SMB2 PC relay server

Relays pad input between SMB2 PC players for online play. It never runs game code: each poll it
collects every human port's pad, then sends the bundle to everyone in the room (lockstep). It also
compares periodic checksums from each player; if anyone drifts out of sync, everyone gets DESYNC and the
room returns to its lobby.

- Protocol: [`include/netplay/protocol.h`](../include/netplay/protocol.h), shared with the game;
  message table in `docs/handoff/E5.md`
- Transport: [ENet](http://enet.bespin.org/) over UDP, default port **7855** (`--port N` or `SMB_SERVER_PORT`)
- Rooms: 4-letter codes, up to 4 players; members must have the same build id, disc image hash and
  language (checked at JOIN and START_GAME). Rooms persist until the last member leaves.
- A player who leaves or drops mid-game keeps the game going: their port reads neutral from a
  server-chosen poll on, the same everywhere. The game ends when no human remains.

## Build and test (Linux / WSL)

```sh
tools/server_test.sh          # builds, starts a server on a free port, runs smb2-server-test
# or by hand:
cmake -S server -B build/server -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/server
build/server/smb2-server --port 7855
build/server/smb2-server-test 127.0.0.1 7855
```

ENet 1.3.18 is fetched at configure time (needs network access and git). Docker and CI: task E10.
