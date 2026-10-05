// Runs on the page and in every worker. The game streams its files with range requests (WasmFS fetch
// backend): keep each range in the browser's Cache Storage so later visits load from disk, and report
// bytes on the 'halo-progress' channel for the page's loading bar.
;(() => {  // leading ; : emscripten's preceding line has none
  if (globalThis.haloFetchWrapped) {
    return;
  }
  globalThis.haloFetchWrapped = true;

  const CACHE_NAME = 'halo-files-v1';
  const progress = new BroadcastChannel('halo-progress');
  const networkFetch = globalThis.fetch.bind(globalThis);
  let cachePromise;
  const openCache = () => (cachePromise ??= typeof caches === 'undefined' ? Promise.resolve(null)
    : caches.open(CACHE_NAME).catch(() => null));  // no Cache Storage outside secure contexts

  globalThis.fetch = async (input, init) => {
    const url = new URL(typeof input === 'string' ? input : input.url, self.location.href);
    const range = init?.headers?.Range;
    if (!range || !url.pathname.startsWith('/halo/')) {
      return networkFetch(input, init);
    }
    const head = init.method === 'HEAD';
    const key = url.origin + url.pathname + (head ? '?head' : '?range=' + range);
    const cache = await openCache();

    const cached = cache && await cache.match(key);
    if (cached) {
      const body = head ? null : await cached.arrayBuffer();
      progress.postMessage({ cached: body ? body.byteLength : 0 });
      return new Response(body, { status: head ? 200 : 206, headers: cached.headers });
    }

    const response = await networkFetch(input, init);
    if (!response.ok) {
      return response;
    }
    const body = head ? null : await response.arrayBuffer();
    progress.postMessage({ downloaded: body ? body.byteLength : 0 });
    if (cache) {
      cache.put(key, new Response(body, { headers: response.headers })).catch(() => {});  // quota: just don't cache
    }
    return new Response(body, { status: response.status, headers: response.headers });
  };
})();
