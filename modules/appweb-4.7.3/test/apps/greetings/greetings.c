#include "esp.h"

static void hello() {
    render("Hello World\n");
}

ESP_EXPORT int esp_controller_greetings(HttpRoute *route) {
    espDefineAction(route, "greetings-hello", hello);
    return 0;
}
