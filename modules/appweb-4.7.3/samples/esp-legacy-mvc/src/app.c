/*
    app.c -- Blog Application Module (esp-legacy-mvc)
 */
 #include "esp.h"

static void base(HttpConn *conn) {}

ESP_EXPORT int esp_app_blog(HttpRoute *route, MprModule *module)
{
    espDefineBase(route, base);
    return 0;
}
