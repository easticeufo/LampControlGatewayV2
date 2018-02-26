/*
    ejsHandler.c -- Ejscript language request handler for the Ejscript Web Framework.

    Copyright (c) All Rights Reserved. See details at the end of the file.
 */
/********************************** Includes **********************************/

#include    "appweb.h"

#if ME_COM_EJS
    #include    "ejs.h"

/************************************* Data ***********************************/
/*
    Default Ejscript start.es script
 */
static char startup[] = "\
    require ejs.web; \
    let server: HttpServer = new HttpServer; \
    var router = Router(Router.Top); \
    server.on('readable', function (event, request) { \
        server.serve(request, router) \
    }); \
    server.listen(); \
";

/************************************* Code ***********************************/
/*
    Open handler for a new request
 */
static void openEjs(HttpQueue *q)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpRoute   *route;
    Ejs         *ejs;
    EjsPool     *pool;
    
    conn = q->conn;
    rx = conn->rx;
    route = rx->route;

    mprTrace(5, "Open EJS handler");
    if (conn->ejs) {
        return;
    }
    /*
        FUTURE OPT - check this pool is usable over all routes
     */
    if (!route->context) {
        if (route->script == 0 && route->scriptPath == 0) {
            route->script = sclone(startup);
        }
        if (route->workers < 0) {
            route->workers = mprGetMaxWorkers();
        }
        route->context = ejsCreatePool(route->workers, "require ejs.web", route->script, route->scriptPath,
            route->home, route->documents);
        mprTrace(5, "ejs: Create ejs pool for route %s", route->name);
    }
    pool = conn->pool = route->context;

    /*
        Allocate a VM engine to service the request
     */
    if ((ejs = ejsAllocPoolVM(pool, EJS_FLAG_HOSTED)) == 0) {
        httpError(conn, HTTP_CODE_SERVICE_UNAVAILABLE, "Cannot create Ejs interpreter");
        return;
    }
    conn->ejs = ejs;
    ejs->hosted = 1;

    /*
        Because we are using on-demand loading of ejs, the ejsHandler stage callbacks may not have been set when
        the Http pipeline needed them (first time). The loading of ejs.web above will have fully initialized them.
     */
    httpAssignQueue(q, conn->http->ejsHandler, HTTP_QUEUE_TX);
}


/*
    EjsAlias appName [path [script [workers]]]
 */
static int ejsAliasDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    char        *prefix, *path, *script;
    int         workers;
    
    route = state->route;
    if (!maTokenize(state, value, "%S ?P ?S ?N", &prefix, &path, &script, &workers)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    route = httpCreateInheritedRoute(state->route);
    httpSetRoutePattern(route, prefix, 0);
    httpSetRoutePrefix(route, prefix);
    httpSetRouteScript(route, 0, script);
    httpSetRouteSource(route, "");
    httpSetRouteDocuments(route, path);
    if (workers) {
        httpSetRouteWorkers(route, workers);
    }
    httpAddRouteHandler(route, "ejsHandler", "");
    httpFinalizeRoute(route);
    return 0;
}


/*
    EjsStartup script
 */
static int ejsStartupDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteScript(state->route, 0, strim(value, "\"", MPR_TRIM_BOTH));
    return 0;
}


static int ejsWorkersDirective(MaState *state, cchar *key, cchar *value)
{
    HttpStage   *stage;

    if ((stage = httpLookupStage(state->http, "ejsHandler")) == 0) {
        mprError("EjsHandler is not configured");
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteWorkers(state->route, atoi(value));
    return 0;
}


/*
    Loadable module initialization
 */
PUBLIC int maEjsHandlerInit(Http *http, MprModule *module)
{
    MaAppweb    *appweb;
    HttpStage   *stage;

    if ((stage = httpCreateHandler(http, "ejsHandler", module)) != 0) {
        http->ejsHandler = stage;
        stage->open = openEjs;
        /*
            The rest of the stage callbacks will be defined by ejsAddWebHandler when ejs.web is loaded from openEjs
         */
    }
    /*
        Add configuration file directives
     */
    appweb = httpGetContext(http);
    maAddDirective(appweb, "EjsAlias", ejsAliasDirective);
    maAddDirective(appweb, "EjsStartup", ejsStartupDirective);
    maAddDirective(appweb, "EjsWorkers", ejsWorkersDirective);
    return 0;
}
#else /* ME_COM_EJS */

PUBLIC int maEjsHandlerInit(Http *http, MprModule *module)
{
    mprNop(0);
    return 0;
}
#endif /* ME_COM_EJS */

/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
