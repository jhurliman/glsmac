"""Cancel real FFmpeg open/probe I/O against a local deliberately stalled stream."""
import http.server
import pathlib
import subprocess
import sys
import threading

release = threading.Event()
requests = []

class StalledMovie(http.server.BaseHTTPRequestHandler):
    def do_GET(self):
        requests.append(self.path)
        self.send_response(200)
        self.send_header("Content-Type", "application/octet-stream")
        self.send_header("Content-Length", "10000000")
        self.end_headers()
        self.wfile.flush()
        release.wait(10)

    def log_message(self, *_):
        pass

server = http.server.ThreadingHTTPServer(("127.0.0.1", 0), StalledMovie)
thread = threading.Thread(target=server.serve_forever, daemon=True)
thread.start()
try:
    probe = str(pathlib.Path(sys.argv[1]).resolve())
    subprocess.run(
        [probe, f"http://127.0.0.1:{server.server_port}/opening.wve", "cancel"],
        check=True, timeout=5,
    )
    assert len(requests) == 4, f"Did not exercise all four actual stalled opens: {requests}"
    print("Four stalled FFmpeg I/O sessions canceled and joined within five seconds")
finally:
    release.set()
    server.shutdown()
    server.server_close()
    thread.join()
