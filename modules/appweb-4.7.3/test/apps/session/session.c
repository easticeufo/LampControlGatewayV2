/*
    Session controller
 */
#include "esp.h"

static void login() { 
    if (getSessionVar("id")) {
        render("Logged in");
        finalize();

    } else if (smatch(param("username"), "admin") && smatch(param("password"), "secret")) {
        render("Valid Login");
        finalize();
        setSessionVar("id", "admin");

    } else if (smatch(getMethod(), "POST")) {
        render("Invalid login, please retry.");
        finalize();

    } else {
        createSession();
        securityToken();
        render("Please Login");
        finalize();
    }
}

ESP_EXPORT int esp_controller_session(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "session-login", login);
    return 0;
}
