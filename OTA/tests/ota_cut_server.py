"""Interrupt the first firmware GET (immediate close or power-cut pause)."""
import argparse
import time
from http.server import BaseHTTPRequestHandler, HTTPServer
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("image", type=Path)
    parser.add_argument("--bind", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8001)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--cut-after", type=int, default=65536)
    mode.add_argument("--pause-after", type=int, metavar="BYTES",
                      help="pause 30 seconds before closing the first GET")
    args = parser.parse_args()
    image = args.image.read_bytes()
    limit = args.cut_after if args.pause_after is None else args.pause_after
    if not 0 < limit < len(image):
        parser.error("interruption offset must fall inside the image")
    endpoint = "/" + args.image.name
    cut_pending = True

    class Handler(BaseHTTPRequestHandler):
        def headers_for_image(self):
            if self.path != endpoint:
                self.send_error(404)
                return False
            self.send_response(200)
            self.send_header("Content-Type", "application/octet-stream")
            self.send_header("Content-Length", str(len(image)))
            self.send_header("Connection", "close")
            self.end_headers()
            return True

        def do_HEAD(self):
            self.headers_for_image()

        def do_GET(self):
            nonlocal cut_pending
            if not self.headers_for_image():
                return
            pause = cut_pending and args.pause_after is not None
            count = limit if cut_pending else len(image)
            self.wfile.write(memoryview(image)[:count])
            self.wfile.flush()
            cut_pending = False
            if pause:
                print(f"PAUSED: sent {count}/{len(image)} bytes; cut ESP32 power NOW "
                      "(server holds connection for 30 s)", flush=True)
                time.sleep(30)
            self.log_message("sent %d/%d bytes; closing connection", count, len(image))

    with HTTPServer((args.bind, args.port), Handler) as server:
        print(f"http://{args.bind}:{server.server_port}{endpoint}", flush=True)
        try:
            server.serve_forever()
        except KeyboardInterrupt:
            pass


if __name__ == "__main__":
    main()
