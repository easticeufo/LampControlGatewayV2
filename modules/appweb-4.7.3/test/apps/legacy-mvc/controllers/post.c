/*
    Post controller
 */
#include "esp.h"

static void common(HttpConn *conn) {
}

static void post_create() { 
    if (updateRec(createRec("post", params()))) {
        inform("New post created");
        redirect("@");
    } else {
        renderView("post-edit");
    }
}

static void post_destroy() { 
    if (removeRec("post", param("id"))) {
        inform("Post removed");
    }
    redirect("@");
}

static void post_edit() { 
    readRec("post", param("id"));
}

static void post_list() { }

static void post_init() { 
    createRec("post", 0);
    renderView("post-edit");
}

static void post_show() { 
    readRec("post", param("id"));
    renderView("post-edit");
}

static void post_update() { 
    if (updateFields("post", params())) {
        inform("Post updated successfully.");
        redirect("@");
    } else {
        renderView("post-edit");
    }
}

static void post_noview() {
    render("Check: OK\r\n");
    finalize();
    /* No view used */
}

static void post_view() {
    setParam("secret", "42");
    /* View will be rendered */
}

ESP_EXPORT int esp_controller_legacy_post(HttpRoute *route, MprModule *module) 
{
    Edi     *edi;

    espDefineBase(route, common);
    espDefineAction(route, "post-create", post_create);
    espDefineAction(route, "post-destroy", post_destroy);
    espDefineAction(route, "post-edit", post_edit);
    espDefineAction(route, "post-init", post_init);
    espDefineAction(route, "post-list", post_list);
    espDefineAction(route, "post-show", post_show);
    espDefineAction(route, "post-update", post_update);

    espDefineAction(route, "post-cmd-noview", post_noview);
    espDefineAction(route, "post-cmd-view", post_view);

    /*
        Add model validations
     */
    edi = espGetRouteDatabase(route);
    ediAddValidation(edi, "present", "post", "title", 0);
    ediAddValidation(edi, "unique", "post", "title", 0);
    ediAddValidation(edi, "format", "post", "body", "(fox|dog)");
    return 0;
}
