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

ENet 1.3.18 is fetched at configure time (needs network access and git).

## Run with Docker

Every push to the `online` branch that passes the integration test publishes
`ghcr.io/jacob-tate/smb2-server` (tags: `latest` and the short commit hash).

```sh
docker run -d --name smb2-server --restart unless-stopped -p 7855:7855/udp \
    ghcr.io/jacob-tate/smb2-server:latest
docker logs -f smb2-server
```

Update to the newest build:

```sh
docker pull ghcr.io/jacob-tate/smb2-server:latest
docker rm -f smb2-server
docker run -d --name smb2-server --restart unless-stopped -p 7855:7855/udp \
    ghcr.io/jacob-tate/smb2-server:latest
```

## Docker Swarm / Portainer

[`portainer-stack.yml`](portainer-stack.yml) runs one instance pinned to the node labelled
`partyboard=true` (the same node that runs the partyboard server), with the UDP port published in host mode.

1. If the node is not labelled yet (from a manager):
   `docker node update --label-add partyboard=true <node-name>`
2. Portainer: **Stacks > Add stack**, name it `smb2`, paste the file (or point **Repository** at
   `server/portainer-stack.yml` on the `online` branch), **Deploy the stack**.
3. Open UDP 7855 on that node (firewall and router). Players connect to that node's address, port 7855.

To update: open the stack in Portainer, **Update the stack** with **Re-pull image** enabled.
Check it from the node: `docker exec $(docker ps -qf name=smb2_relay) smb2-server-test 127.0.0.1 7855`
(replace `smb2` with your stack name).

Keep `replicas: 1`: rooms live in memory, so a second instance would split players between them.

## Build the image yourself

From the repository root: `docker build -f server/Dockerfile -t smb2-server .`

To use another port: `-p 9000:9000/udp -e SMB_SERVER_PORT=9000`.

Check a running deployment with the bundled test client (two fake players, ~1 second):

```sh
docker exec smb2-server smb2-server-test 127.0.0.1 7855
```
