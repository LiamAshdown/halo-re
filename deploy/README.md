# Deploying the browser build

The image holds `serve_web.py` and the web build. Halo's files stay on the VPS in `/srv/halo-data`, mounted read-only:

    /srv/halo-data/halo/     the Halo PC install folder
    /srv/halo-data/fx.bin    converted shaders (build/cxx/Release/override/shaders/fx.bin)

## 1. Upload the Halo files (once)

From PowerShell on your PC (Windows ships `tar` and `scp`):

    tar -czf halo-data.tgz -C "C:\Program Files (x86)\Microsoft Games" Halo
    scp halo-data.tgz build\cxx\Release\override\shaders\fx.bin root@YOUR_VPS_IP:/tmp/

On the VPS:

    mkdir -p /srv/halo-data
    tar -xzf /tmp/halo-data.tgz -C /srv/halo-data      # gives /srv/halo-data/Halo
    mv /srv/halo-data/Halo /srv/halo-data/halo
    mv /tmp/fx.bin /srv/halo-data/
    chmod -R a+rX /srv/halo-data && rm /tmp/halo-data.tgz

## 2. Upload the server (after each web build)

    tar -czf halo-re.tgz deploy tools/serve_web.py build/web/halo.html build/web/halo.js build/web/halo.wasm
    scp halo-re.tgz root@YOUR_VPS_IP:/tmp/

On the VPS (Docker installed: `curl -fsSL https://get.docker.com | sh`):

    mkdir -p /srv/halo-re && tar -xzf /tmp/halo-re.tgz -C /srv/halo-re && cd /srv/halo-re
    echo "HALO_DOMAIN=halo.example.com" > deploy/.env   # once
    docker compose -f deploy/compose.yaml up -d --build

Point the domain's A record at the VPS first; Caddy fetches the HTTPS certificate on start.
Logs: `docker compose -f deploy/compose.yaml logs -f`.
