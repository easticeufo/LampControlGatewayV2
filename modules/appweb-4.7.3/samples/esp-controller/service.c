/*
    service.c - Test ESP controller
 */
 
#include    "esp.h"

/*
    Controller action
 */
static void hello() {
    render("Hello World\n");
    finalize();
}

/*
    Controller initialization. Invoked when the controller is loaded.
 */
ESP_EXPORT int esp_controller_service(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "test-hello", hello);
    return 0;
}
