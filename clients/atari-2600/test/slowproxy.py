#!/usr/bin/env python3
"""slowproxy.py -- the rig's server as the internet delivers it: late, and in
pieces. Forwards GETs to the server and sends each reply's body in chunks
with pauses between, so a client that reads before the whole reply is in
reads it short.

usage: slowproxy.py LISTEN_PORT SERVER_PORT [DELAY_MS] [CHUNK]
"""
import http.server
import sys
import time
import urllib.request

LISTEN, SERVER = int(sys.argv[1]), int(sys.argv[2])
DELAY = (int(sys.argv[3]) if len(sys.argv) > 3 else 150) / 1000.0
CHUNK = int(sys.argv[4]) if len(sys.argv) > 4 else 200


class H(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"

    def do_GET(self):
        time.sleep(DELAY)
        with urllib.request.urlopen("http://127.0.0.1:%d%s" % (SERVER, self.path)) as r:
            body, ctype = r.read(), r.headers.get("Content-Type", "application/octet-stream")
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.flush()
        for i in range(0, len(body), CHUNK):
            self.wfile.write(body[i:i + CHUNK])
            self.wfile.flush()
            time.sleep(DELAY)

    def log_message(self, *a):
        pass


http.server.ThreadingHTTPServer(("127.0.0.1", LISTEN), H).serve_forever()
