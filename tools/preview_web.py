#!/usr/bin/env python3
"""Build embedded HTML and serve a local, hardware-disconnected UI preview."""
import argparse
import json
import re
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'ESP32_S3_Gateway'
OUTPUT = ROOT / '.build' / 'web'


def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    defines = dict(re.findall(r'^#define\s+(\w+)\s+"([^"]*)"', (SOURCE / 'firmware_build.h').read_text(), re.M))
    for source, target in [('dashboard_html.h', 'index.html'), ('index_html.h', 'calibration.html'), ('wheel_tests_html.h', 'wheel-tests.html')]:
        text = (SOURCE / source).read_text()
        start = text.index('=') + 1
        text = text[start:]
        parts = []
        pattern = r'R"([^\s()]*)\((.*?)\)\1"|\b(TWIP_WEB_\w+)\b'
        for match in re.finditer(pattern, text, re.S):
            parts.append(match[2] if match[1] is not None else defines[match[3]])
        html = ''.join(parts)
        if '<!doctype html>' not in html.lower() or '</html>' not in html.lower():
            raise ValueError(f'Invalid HTML extracted from {source}')
        if target == 'index.html':
            html = html.replace('<main>', '<main><div class="preview-notice" style="background:#76edc118;border:1px solid #76edc144;color:#76edc1;font-size:11px;padding:9px 12px;border-radius:10px;margin-bottom:9px;text-align:center">📱 BẢN XEM TRƯỚC · Đã mở tương tác Joystick trên điện thoại</div>', 1)
            html = html.replace("function canDrive(){", "function canDrive(){return true; /* preview */\n")
            html = html.replace("ws.send('$MOVE8,'+driveHeld+','+driveTurn+','+driveEpoch+','+(++driveSequence)+','+drivePreset+'*');",
                                "if(ws&&wsOK)ws.send('$MOVE8,'+driveHeld+','+driveTurn+','+driveEpoch+','+(++driveSequence)+','+drivePreset+'*');")
        (OUTPUT / target).write_text(html)
        print(f'Built {target}: {len(html.encode())} bytes', flush=True)
    return defines


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(OUTPUT), **kwargs)

    def end_headers(self):
        self.send_header('Cache-Control', 'no-store')
        super().end_headers()

    def do_HEAD(self):
        self.do_GET()

    def do_GET(self):
        path = self.path.split('?', 1)[0].rstrip('/')
        if not path:
            path = '/'
        routes = {'/': '/index.html', '/ui/r12': '/index.html', '/ui/bal-r6': '/index.html', '/calibration': '/calibration.html', '/wheel-tests': '/wheel-tests.html'}
        if path in routes:
            self.path = routes[path]
        elif path == '/firmware-info':
            self.send_json(200, {'build': DEFINES['TWIP_WEB_BUILD'], 'preview': True})
            return
        elif path == '/balance-status':
            mock_row = [0] * 38
            mock_row[2] = 5000
            mock_row[4] = 3
            mock_row[5] = 1
            mock_row[6] = 0
            mock_row[14] = 0; mock_row[15] = 0; mock_row[16] = 4096
            mock_row[22] = 0.0
            mock_row[23] = 0.0
            mock_row[27] = 12
            mock_row[28] = 0
            mock_row[32] = 1
            mock_row[33] = 1
            row_str = ",".join(str(x) for x in mock_row)
            self.send_json(200, {
                'seen': True,
                'age_ms': 20,
                'row': row_str,
                'meta': '0,BAL-R12,0,0,0,0,1,100,0,0,0,0,0,0,0,0,0,0,0.998',
                'samples': 0, 'capacity': 10000,
                'crc': 0, 'gaps': 0, 'overflow': 0, 'resets': 0,
                'owner': 1, 'owner_token': 1, 'control_revision': 1,
                'drive_epoch': 1, 'stop_source': ''
            })
            return
        elif path in ('/hardware-status', '/measurement-status'):
            self.send_json(200, {'seen': True, 'age_ms': 20, 'balance_live': True})
            return
        super().do_GET()

    def do_POST(self):
        self.send_json(200, {'message': 'Đã nhận lệnh xem trước.'})

    def send_json(self, status, value):
        body = json.dumps(value).encode()
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Content-Length', str(len(body)))
        self.end_headers()
        self.wfile.write(body)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, default=8000)
    parser.add_argument('--host', default='127.0.0.1', help='Use 0.0.0.0 to preview from a phone on the same Wi-Fi')
    parser.add_argument('--build-only', action='store_true')
    args = parser.parse_args()
    DEFINES = build()
    if not args.build_only:
        server = ThreadingHTTPServer((args.host, args.port), Handler)
        print(f'UI preview: http://localhost:{args.port}/ui/r12', flush=True)
        server.serve_forever()
