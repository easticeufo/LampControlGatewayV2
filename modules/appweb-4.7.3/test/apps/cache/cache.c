/*
    Test caching
 */
#include "esp.h"

//  This is configured for caching by API below
static void api() {
    render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
}

static void sml() {
    int     i;
    for (i = 0; i < 1; i++) {
        render("Line: %05d %s", i, "aaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbccccccccccccccccccddddddd<br/>\r\n");
    }
    render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
}

static void medium() {
    int     i;
    //  This will emit ~8K (under the item limit)
    for (i = 0; i < 100; i++) {
        render("Line: %05d %s", i, "aaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbccccccccccccccccccddddddd<br/>\r\n");
    }
    render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
}

static void big() {
    int     i;
    //  This will emit ~76K (under the item limit)
    for (i = 0; i < 1000; i++) {
        render("Line: %05d %s", i, "aaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbccccccccccccccccccddddddd<br/>\r\n");
    }
}

static void huge() { 
    int     i;
    //  This will emit ~762K (over the item limit)
    for (i = 0; i < 10000; i++) {
        render("Line: %05d %s", i, "aaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbccccccccccccccccccddddddd<br/>\r\n");
    }
    render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
}

static void clear() { 
    espUpdateCache(getConn(), "/cache/manual", 0, 0);
    espUpdateCache(getConn(), "/cache/big", 0, 0);
    espUpdateCache(getConn(), "/cache/medium", 0, 0);
    espUpdateCache(getConn(), "/cache/small", 0, 0);
    espUpdateCache(getConn(), "/cache/api", 0, 0);
    render("done");
}

static void client() { 
    render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
}

static void manual() { 
    if (smatch(getQuery(), "send")) {
        setHeader("X-SendCache", "true");
        finalize();
    } else if (!espRenderCached(getConn())) {
        render("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
    }
}

static void update() { 
    cchar   *data = sfmt("{ when: %Ld, uri: '%s', query: '%s' }\r\n", mprGetTicks(), getUri(), getQuery());
    espUpdateCache(getConn(), "/cache/manual", data, 86400);
    render("done");
}

ESP_EXPORT int esp_controller_cache(HttpRoute *route, MprModule *module) {
    HttpRoute   *rp;

    espDefineAction(route, "cache-api", api);
    espDefineAction(route, "cache-big", big);
    espDefineAction(route, "cache-small", sml);
    espDefineAction(route, "cache-medium", medium);
    espDefineAction(route, "cache-clear", clear);
    espDefineAction(route, "cache-client", client);
    espDefineAction(route, "cache-huge", huge);
    espDefineAction(route, "cache-manual", manual);
    espDefineAction(route, "cache-update", update);

    //  This is not required for unit tests
    if ((rp = httpLookupRoute(route->host, "/cache/")) != 0) {
        espCache(rp, "/cache/{action}", 0, 0);
    }
    return 0;
}
