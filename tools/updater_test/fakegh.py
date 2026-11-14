#!/usr/bin/env python3
"""Fake GitHub releases/latest server for the updater tests (127.0.0.1 only).
  fakegh.py <port> <setup_file> [<version>]
URL: http://127.0.0.1:<port>/<scenario>/latest
scenarios: same (=current 1.0.9), new, badsum, nosums, noasset, dlfail, slow, 404, 500"""
import sys, json, hashlib, time, http.server, socketserver, os
PORT=int(sys.argv[1]); SETUP=sys.argv[2]; VER=sys.argv[3] if len(sys.argv)>3 else "1.1.0"
data=open(SETUP,'rb').read(); sha=hashlib.sha256(data).hexdigest()
NOTES = """## What's new in %s

- **Updater**: the game checks for new versions and shows these notes.
- Fixed the *boat* sky reflection on Monkey Boat (see `data/boat/sky.gma` and https://example.invalid/issues/12).
- Better controller support: GameCube octagonal gate, calibration, dead zone.
  - nested item: Xbox Elite paddles map to Z
- Café, Übergröße, niño: accented letters à é ü ñ.

### Known issues
1. Target sometimes logs a heap warning.
2. a/b p/q c/0x12 /bc000000/ should print literally.

> Thanks to everyone who reported bugs!

Line 13
Line 14
Line 15
Line 16
Line 17 - the end of the notes (scroll test)."""
class H(http.server.BaseHTTPRequestHandler):
    def log_message(self, f, *a): sys.stderr.write("[fakegh] "+(f%a)+"\n")
    def send(self, code, body, ctype="application/json"):
        self.send_response(code); self.send_header("Content-Type",ctype); self.send_header("Content-Length",str(len(body))); self.end_headers(); self.wfile.write(body)
    def do_GET(self):
        sys.stderr.write("[fakegh] UA=%r\n" % self.headers.get("User-Agent"))
        parts=self.path.strip('/').split('/'); sc=parts[0]; base="http://127.0.0.1:%d/%s"%(PORT,sc)
        v = "1.0.9" if sc=="same" else VER
        if len(parts)>=2 and parts[1]=="latest":
            if sc=="404": return self.send(404,b'{"message":"Not Found"}')
            if sc=="500": return self.send(500,b'oops',"text/plain")
            setup="SMB2_PC_Setup_%s.exe"%v
            assets=[{"name":"SMB2_PC.zip","browser_download_url":base+"/dl/SMB2_PC.zip","size":3},
                    {"name":"SMB2_PC_x64.zip","browser_download_url":base+"/dl/SMB2_PC_x64.zip","size":3}]
            if sc!="noasset": assets.append({"name":setup,"browser_download_url":base+"/dl/"+setup,"size":len(data)})
            if sc not in ("nosums",): assets.append({"name":"SHA256SUMS.txt","browser_download_url":base+"/dl/SHA256SUMS.txt","size":200})
            j={"tag_name":"v"+v,"name":"SMB2 "+v+" - updater test","html_url":base+"/page","draft":False,"prerelease":False,"body":NOTES%v,"assets":assets}
            return self.send(200,json.dumps(j).encode())
        if len(parts)>=3 and parts[1]=="dl":
            name=parts[2]
            if name=="SHA256SUMS.txt":
                s = ("0"*64) if sc=="badsum" else sha
                body=("%s  SMB2_PC_Setup_%s.exe\n%s  SMB2_PC.zip\n"%(s,v,"1"*64)).encode()
                return self.send(200,body,"text/plain")
            if name.startswith("SMB2_PC_Setup_"):
                self.send_response(200); self.send_header("Content-Length",str(len(data))); self.send_header("Content-Type","application/octet-stream"); self.end_headers()
                step=max(len(data)//100,4096)
                for i in range(0,len(data),step):
                    if sc=="dlfail" and i>len(data)//3: self.wfile.flush(); self.connection.close(); return
                    self.wfile.write(data[i:i+step])
                    if sc=="slow": time.sleep(0.08)
                return
        self.send(404,b'{}')
class S(socketserver.ThreadingMixIn, http.server.HTTPServer): daemon_threads=True; allow_reuse_address=True
print("fakegh on 127.0.0.1:%d setup sha %s (%d bytes)"%(PORT,sha,len(data)),flush=True)
S(("127.0.0.1",PORT),H).serve_forever()
