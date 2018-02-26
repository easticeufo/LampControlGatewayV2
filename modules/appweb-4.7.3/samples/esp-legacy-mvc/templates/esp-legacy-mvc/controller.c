/*
    ${UCONTROLLER} Controller for esp-legacy-mvc
 */
#include "esp.h"

static void create() { 
    if (updateRec(createRec("${CONTROLLER}", params()))) {
        flash("inform", "New ${UCONTROLLER} created");
        renderView("${CONTROLLER}-list");
    } else {
        renderView("${CONTROLLER}-edit");
    }
}

static void destroy() { 
    if (removeRec("${CONTROLLER}", param("id"))) {
        flash("inform", "${UCONTROLLER} removed");
    }
    renderView("${CONTROLLER}-list");
}

static void edit() { 
    readRec("${CONTROLLER}", param("id"));
}

static void list() { }

static void init() { 
    createRec("${CONTROLLER}", 0);
    renderView("${CONTROLLER}-edit");
}

static void show() { 
    readRec("${CONTROLLER}", param("id"));
    renderView("${CONTROLLER}-edit");
}

static void update() { 
    if (updateFields("${CONTROLLER}", params())) {
        flash("inform", "${UCONTROLLER} updated successfully.");
        renderView("${CONTROLLER}-list");
    } else {
        renderView("${CONTROLLER}-edit");
    }
}

ESP_EXPORT int esp_controller_${APP}_${CONTROLLER}(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "${CONTROLLER}-create", create);
    espDefineAction(route, "${CONTROLLER}-destroy", destroy);
    espDefineAction(route, "${CONTROLLER}-edit", edit);
    espDefineAction(route, "${CONTROLLER}-init", init);
    espDefineAction(route, "${CONTROLLER}-list", list);
    espDefineAction(route, "${CONTROLLER}-show", show);
    espDefineAction(route, "${CONTROLLER}-update", update);
${DEFINE_ACTIONS}
#if SAMPLE_VALIDATIONS
    Edi *edi = espGetRouteDatabase(route);
    ediAddValidation(edi, "present", "${CONTROLLER}", "title", 0);
    ediAddValidation(edi, "unique", "${CONTROLLER}", "title", 0);
    ediAddValidation(edi, "banned", "${CONTROLLER}", "body", "(swear|curse)");
    ediAddValidation(edi, "format", "${CONTROLLER}", "phone", "/^\(?([0-9]{3})\)?[-. ]?([0-9]{3})[-. ]?([0-9]{4})$/");
#endif
    return 0;
}
