"""Local-only preview server for the assembled offline demo."""
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from functools import partial
import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--port', type=int, default=8786)
args = parser.parse_args()
folder = Path(__file__).resolve().parents[1] / 'build'
if not (folder / 'Sixfold-Recoil-Beta.html').is_file():
    raise SystemExit('Build first: node scripts/build.mjs')

class Handler(SimpleHTTPRequestHandler):
    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

server = ThreadingHTTPServer(('127.0.0.1', args.port), partial(Handler, directory=str(folder)))
print(f'Local demo: http://127.0.0.1:{args.port}/Sixfold-Recoil-Beta.html', flush=True)
try:
    server.serve_forever()
except KeyboardInterrupt:
    server.server_close()
