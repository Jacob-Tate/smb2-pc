# Minimal DSU (cemuhook) server feeding a scripted pad to Dolphin.
# usage: dsu_server.py <frames_dir> <script>   script: "frame:len:BTN+BTN,..." frames = dolphin framedump index
import socket, struct, zlib, sys, os, time, select
FR, SCRIPT = sys.argv[1], sys.argv[2]
BITS = {  # (byte index in [b1,b2], mask)
    'LEFT': (0, 0x80), 'DOWN': (0, 0x40), 'RIGHT': (0, 0x20), 'UP': (0, 0x10), 'START': (0, 0x08),
    'B': (1, 0x80), 'A': (1, 0x40), 'X': (1, 0x20), 'Y': (1, 0x10), 'Z': (1, 0x08), 'R': (1, 0x08), 'L': (1, 0x04)}
sched = []; sched_mtime = -1
def load_sched():
    global sched, sched_mtime
    try:
        m = os.path.getmtime(SCRIPT)
        if m == sched_mtime: return
        txt = open(SCRIPT).read().replace('\n', ',')
    except OSError: return
    new = []
    for e in filter(None, (x.strip() for x in txt.split(','))):
        try:
            f, l, b = e.split(':'); new.append((int(f), int(f) + int(l), b.split('+')))
        except ValueError: pass
    sched = new; sched_mtime = m
SID = 0x1234abcd
def pkt(mtype, payload):
    body = struct.pack('<I', mtype) + payload
    hdr = b'DSUS' + struct.pack('<HHII', 1001, len(body), 0, SID)
    crc = zlib.crc32(hdr + body) & 0xffffffff
    return b'DSUS' + struct.pack('<HHII', 1001, len(body), crc, SID) + body
def shared(slot, connected):
    return struct.pack('<BBBB6sB', slot, 2 if connected else 0, 2 if connected else 0, 1 if connected else 0, b'\x00\x00\x00\x00\x00\x01' if connected else b'\0'*6, 5 if connected else 0)
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM); s.bind(('127.0.0.1', int(os.environ.get('DSU_PORT', '26760'))))
clients = {}; num = 0; frame = 0; last_scan = 0; logf = open(os.path.join(os.path.dirname(FR.rstrip('/')), 'dsu.log'), 'a')
def cur_frame():
    global frame
    try:
        for f in os.listdir(FR):
            if f.startswith('framedump_'):
                n = int(f[10:-4]); frame = max(frame, n)
    except OSError: pass
    return frame
while True:
    r, _, _ = select.select([s], [], [], 0.004)
    if r:
        data, addr = s.recvfrom(2048)
        if data[:4] == b'DSUC' and len(data) >= 20:
            mtype, = struct.unpack('<I', data[16:20])
            if mtype == 0x100000:
                s.sendto(pkt(0x100000, struct.pack('<H', 1001)), addr)
            elif mtype == 0x100001:
                cnt, = struct.unpack('<i', data[20:24])
                for i in range(cnt):
                    slot = data[24 + i]
                    s.sendto(pkt(0x100001, shared(slot, slot == 0) + b'\0'), addr)
            elif mtype == 0x100002:
                if addr not in clients: logf.write('client %r\n' % (addr,)); logf.flush()
                clients[addr] = time.time()
    now = time.time()
    if now - last_scan > 0.01:
        cur_frame(); load_sched(); last_scan = now
        try: open(FR.rstrip('/') + '/../curframe.txt', 'w').write(str(frame))
        except OSError: pass
    b = [0, 0]; names = []; lx = ly = 128
    STICK = {'SU': (128, 255), 'SD': (128, 0), 'SL': (0, 128), 'SR': (255, 128)}
    for a, z, bl in sched:
        if a <= frame < z:
            for n in bl:
                if n in STICK: lx, ly = STICK[n]; continue
                if n[:2] in ('LX', 'LY') and n[2:].isdigit():
                    if n[1] == 'X': lx = int(n[2:])
                    else: ly = int(n[2:])
                    continue
                i, m = BITS[n]; b[i] |= m; names.append(n)
    num += 1
    an = bytes([255 if (b[0] & 0x80) else 0, 255 if (b[0] & 0x40) else 0, 255 if (b[0] & 0x20) else 0, 255 if (b[0] & 0x10) else 0,
                255 if (b[1] & 0x80) else 0, 255 if (b[1] & 0x40) else 0, 255 if (b[1] & 0x20) else 0, 255 if (b[1] & 0x10) else 0, 0, 0, 0, 0])
    payload = shared(0, True) + struct.pack('<BIBBBB', 1, num, b[0], b[1], 0, 0) + bytes([lx, ly, 128, 128]) + an + b'\0' * 12 + struct.pack('<Q', int(now * 1e6)) + struct.pack('<6f', 0, -1, 0, 0, 0, 0)
    for addr, t in list(clients.items()):
        if now - t > 10: del clients[addr]; continue
        s.sendto(pkt(0x100002, payload), addr)
