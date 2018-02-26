/*
    ${CONTROLLER} Controller for esp-html-mvc (esp-html-mvc)
 */
#include "esp.h"

/*
    Create a new resource in the database
 */ 
static void create${UCONTROLLER}() { 
    if (updateRec(createRec("${CONTROLLER}", params()))) {
        flash("inform", "New ${CONTROLLER} Created");
        renderView("${CONTROLLER}/${CONTROLLER}-list");
    } else {
        flash("error", "Cannot Create ${UCONTROLLER}");
        renderView("${CONTROLLER}/${CONTROLLER}-edit");
    }
}

/*
    Prepare a template for editing a resource
 */
static void edit${UCONTROLLER}() { 
    readRec("${CONTROLLER}", param("id"));
}

/*
    Get a resource
 */ 
static void get${UCONTROLLER}() { 
    readRec("${CONTROLLER}", param("id"));
    renderView("${CONTROLLER}/${CONTROLLER}-edit");
}

/*
    Initialize a new resource for the client to complete
 */
static void init${UCONTROLLER}() { 
    createRec("${CONTROLLER}", 0);
    renderView("${CONTROLLER}/${CONTROLLER}-edit");
}

/*
    List the resources in this group
 */ 
static void list${UCONTROLLER}() { 
    renderView("${CONTROLLER}/${CONTROLLER}-list");
}

/*
    Remove a resource identified by the "id" parameter
 */
static void remove${UCONTROLLER}() { 
    if (removeRec("${CONTROLLER}", param("id"))) {
        flash("inform", "${UCONTROLLER} Removed");
    }
    redirect("list");
}

/*
    Update an existing resource in the database
    If "id" is not defined, this is the same as a create
    Also we tunnel delete here if the user clicked delete
 */
static void update${UCONTROLLER}() { 
    if (smatch(param("submit"), "Delete")) {
        removePost();
    } else {
        if (updateFields("${CONTROLLER}", params())) {
            flash("inform", "${UCONTROLLER} Updated Successfully");
            redirect("list");
        } else {
            flash("error", "Cannot Update ${UCONTROLLER}");
            renderView("${CONTROLLER}/${CONTROLLER}-edit");
        }
    }
}

static void common(HttpConn *conn) {
}

/*
    Dynamic module initialization
 */
ESP_EXPORT int esp_controller_${APP}_${CONTROLLER}(HttpRoute *route, MprModule *module) {
    espDefineBase(route, common);
    espDefineAction(route, "${CONTROLLER}-create", create${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-remove", remove${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-edit", edit${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-get", get${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-init", init${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-list", list${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-update", update${UCONTROLLER});
    espDefineAction(route, "${CONTROLLER}-cmd-", list${UCONTROLLER});
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
