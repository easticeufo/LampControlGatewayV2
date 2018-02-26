/*
    Post Controller for esp-legacy-mvc
 */
#include "esp.h"

static void create() { 
    if (updateRec(createRec("post", params()))) {
        flash("inform", "New Post created");
        renderView("post-list");
    } else {
        renderView("post-edit");
    }
}

static void destroy() { 
    if (removeRec("post", param("id"))) {
        flash("inform", "Post removed");
    }
    renderView("post-list");
}

static void edit() { 
    readRec("post", param("id"));
}

static void list() { }

static void init() { 
    createRec("post", 0);
    renderView("post-edit");
}

static void show() { 
    readRec("post", param("id"));
    renderView("post-edit");
}

static void update() { 
    if (updateFields("post", params())) {
        flash("inform", "Post updated successfully.");
        renderView("post-list");
    } else {
        renderView("post-edit");
    }
}

ESP_EXPORT int esp_controller_blog_post(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "post-create", create);
    espDefineAction(route, "post-destroy", destroy);
    espDefineAction(route, "post-edit", edit);
    espDefineAction(route, "post-init", init);
    espDefineAction(route, "post-list", list);
    espDefineAction(route, "post-show", show);
    espDefineAction(route, "post-update", update);

#if SAMPLE_VALIDATIONS
    Edi *edi = espGetRouteDatabase(route);
    ediAddValidation(edi, "present", "post", "title", 0);
    ediAddValidation(edi, "unique", "post", "title", 0);
    ediAddValidation(edi, "banned", "post", "body", "(swear|curse)");
    ediAddValidation(edi, "format", "post", "phone", "/^\(?([0-9]{3})\)?[-. ]?([0-9]{3})[-. ]?([0-9]{4})$/");
#endif
    return 0;
}
