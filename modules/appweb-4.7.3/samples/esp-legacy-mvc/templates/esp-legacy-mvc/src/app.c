/*
    app.c -- ${UAPP} Application Module (esp-legacy-mvc)
 */
 #include "esp.h"

static void base(HttpConn *conn) {}

ESP_EXPORT int esp_app_${APP}(HttpRoute *route, MprModule *module)
{
    espDefineBase(route, base);
    return 0;
}
