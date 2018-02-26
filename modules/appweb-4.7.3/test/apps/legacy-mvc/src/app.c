/*
    app.c -- Legacy-mvc Source
 */
 #include "esp.h"

static void base(HttpConn *conn) {}

ESP_EXPORT int esp_app_legacy(HttpRoute *route, MprModule *module)
{
    espDefineBase(route, base);
    return 0;
}
