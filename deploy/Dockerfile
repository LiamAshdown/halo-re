# Build context: the repository root (needs build/web from an emscripten build).
# Halo files are not in the image: mount them at /data (/data/halo = the install, /data/fx.bin = converted shaders).
FROM python:3.13-slim

WORKDIR /app
COPY tools/serve_web.py tools/halo_ui_assets.py ./
COPY build/web/halo.html build/web/halo.js build/web/halo.wasm web/

USER nobody
ENV SERVE_WEB_QUIET=1
EXPOSE 8080
CMD ["python", "serve_web.py", "--host", "0.0.0.0", "--web", "web", "--halo", "/data/halo", "--fx", "/data/fx.bin"]
