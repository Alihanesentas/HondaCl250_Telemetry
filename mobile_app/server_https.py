#!/usr/bin/env python3
"""
Live HTTPS Web Server for Honda CL250 Mobile Web Bluetooth App
Serves mobile_app/ directory over HTTPS (SSL) on port 8443.
Required for mobile Web Bluetooth API access in Safari/Chrome.
"""

import http.server
import ssl
import os
import socket
import subprocess

PORT = 8443
CERT_FILE = "cert.pem"
KEY_FILE = "key.pem"

def get_local_ip():
    try:
        s = socket.socket(socket.AF_INET, socket.DGRAM)
        s.connect(("8.8.8.8", 80))
        ip = s.getsockname()[0]
        s.close()
        return ip
    except Exception:
        return "127.0.0.1"

def generate_self_signed_cert():
    if not os.path.exists(CERT_FILE) or not os.path.exists(KEY_FILE):
        print("[HTTPS SERVER] Generating self-signed SSL certificate for Web Bluetooth...")
        cmd = [
            "openssl", "req", "-x509", "-newkey", "rsa:2048",
            "-keyout", KEY_FILE, "-out", CERT_FILE,
            "-days", "365", "-nodes",
            "-subj", "/CN=Honda-CL250-Telemetry"
        ]
        subprocess.run(cmd, check=True)

def run_https_server():
    os.chdir(os.path.dirname(os.path.abspath(__file__)))
    generate_self_signed_cert()

    local_ip = get_local_ip()
    handler = http.server.SimpleHTTPRequestHandler
    httpd = http.server.HTTPServer(("0.0.0.0", PORT), handler)

    context = ssl.SSLContext(ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain(certfile=CERT_FILE, keyfile=KEY_FILE)
    httpd.socket = context.wrap_socket(httpd.socket, server_side=True)

    print("\n==========================================================")
    print("   HONDA CL250 MOBILE TELEMETRY LIVE HTTPS SERVER ACTIVE  ")
    print("==========================================================")
    print(f" -> Mobile Phone URL: https://{local_ip}:{PORT}/index.html")
    print(f" -> Local PC URL:    https://localhost:{PORT}/index.html")
    print("==========================================================")
    print("Open the URL on your mobile phone and accept the SSL certificate to enable Web Bluetooth pairing.\n")

    httpd.serve_forever()

if __name__ == "__main__":
    run_https_server()
