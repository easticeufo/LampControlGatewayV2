/**
    config.c - Parse the configuration file.
    Copyright (c) All Rights Reserved. See details at the end of the file.
 */

/********************************* Includes ***********************************/

#include    "appweb.h"
#include    "pcre.h"

/***************************** Forward Declarations ****************************/

static int addCondition(MaState *state, cchar *name, cchar *condition, int flags);
static int addUpdate(MaState *state, cchar *name, cchar *details, int flags);
static bool conditionalDefinition(MaState *state, cchar *key);
static int configError(MaState *state, cchar *key);
static MaState *createState(MaServer *server, int flags);
static char *getDirective(char *line, char **valuep);
static int getint(cchar *value);
static int64 getnum(cchar *value);
static void manageState(MaState *state, int flags);
static int parseFileInner(MaState *state, cchar *path);
static int setTarget(MaState *state, cchar *name, cchar *details);

/******************************************************************************/

PUBLIC int maOpenConfig(MaState *state, cchar *path)
{
    assert(state);
    assert(path && *path);

    state->filename = sclone(path);
    state->configDir = mprGetAbsPath(mprGetPathDir(state->filename));
    mprLog(3, "Open config file \"%s\"", mprGetAbsPath(state->filename));
    if ((state->file = mprOpenFile(mprGetRelPath(path, NULL), O_RDONLY | O_TEXT, 0444)) == 0) {
        mprError("Cannot open %s for config directives", path);
        return MPR_ERR_CANT_OPEN;
    }
    mprTrace(5, "Parsing config file: %s", state->filename);
    return 0;
}


PUBLIC int maParseConfig(MaServer *server, cchar *path, int flags)
{
    MaState     *state;
    HttpRoute   *route;
    cchar       *dir;

    assert(server);
    assert(path && *path);

    mprLog(2, "Using config file: \"%s\"", mprGetAbsPath(path));

    state = createState(server, flags);
    route = state->route;
    dir = mprGetAbsPath(mprGetPathDir(path));

    httpSetRouteHome(route, dir);
    httpSetRouteDocuments(route, dir);
    httpSetRouteVar(route, "LOG_DIR", ".");
    httpSetRouteVar(route, "INC_DIR", ME_VAPP_PREFIX "/inc");
    httpSetRouteVar(route, "SPL_DIR", ME_SPOOL_PREFIX);
    httpSetRouteVar(route, "BIN_DIR", mprJoinPath(server->appweb->platformDir, "bin"));

#if DEPRECATED || 1
    /* DEPRECATED */ httpSetRouteVar(route, "LIBDIR", mprJoinPath(server->appweb->platformDir, "bin"));
    /* DEPRECATED */ httpSetRouteVar(route, "BINDIR", mprJoinPath(server->appweb->platformDir, "bin"));
#endif

    if (maParseFile(state, path) < 0) {
        server->state = 0;
        return MPR_ERR_BAD_SYNTAX;
    }
    httpFinalizeRoute(state->route);
    server->state = 0;

    if (!maValidateServer(server)) {
        server->state = 0;
        return MPR_ERR_BAD_ARGS;
    }
    if (mprHasMemError()) {
        mprError("Memory allocation error when initializing");
        return MPR_ERR_MEMORY;
    }
    return 0;
}


PUBLIC int maParseFile(MaState *state, cchar *path)
{
    MaAppweb    *appweb;
    MaState     *topState;
    int         rc, lineNumber;

    assert(path && *path);
    if (!state) {
        lineNumber = 0;
        appweb = MPR->appwebService;
        topState = state = createState(appweb->defaultServer, 0);
    } else {
        topState = 0;
        lineNumber = state->lineNumber;
        state = maPushState(state);
    }
    rc = parseFileInner(state, path);
    if (topState) {
        state->server->state = 0;
    } else {
        state = maPopState(state);
        state->lineNumber = lineNumber;
    }
    return rc;
}


static int parseFileInner(MaState *state, cchar *path)
{
    MaDirective *directive;
    char        *tok, *key, *line, *value;
    
    assert(state);
    assert(path && *path);

    if (maOpenConfig(state, path) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    for (state->lineNumber = 1; state->file && (line = mprReadLine(state->file, 0, NULL)) != 0; state->lineNumber++) {
        for (tok = line; isspace((uchar) *tok); tok++) ;
        if (*tok == '\0' || *tok == '#') {
            continue;
        }
        state->key = 0;
        key = getDirective(line, &value);
        if (!state->enabled) {
            if (key[0] != '<') {
                mprTrace(8, "Skip: %s %s", key, value);
                continue;
            }
        }
        if ((directive = mprLookupKey(state->appweb->directives, key)) == 0) {
            mprError("Unknown directive \"%s\"\nAt line %d in %s\n\n", key, state->lineNumber, state->filename);
            return MPR_ERR_BAD_SYNTAX;
        }
        state->key = key;
        mprTrace(8, "Line %d, Parse %s %s", state->lineNumber, state->key, value ? value : "");
        /*
            Allow directives to run commands and yield without worring about holding references.
         */
        mprPauseGC();
        if ((*directive)(state, key, value) < 0) {
            mprResumeGC();
            mprError("Error with directive \"%s\"\nAt line %d in %s\n\n", state->key, state->lineNumber, state->filename);
            return MPR_ERR_BAD_SYNTAX;
        }
        mprResumeGC();
        mprYield(0);
        state = state->top->current;
    }
    /* EOF */
    if (state->prev && state->file == state->prev->file) {
        mprError("Unclosed directives in %s", state->filename);
        while (state->prev && state->file == state->prev->file) {
            state = state->prev;
        }
    }
    mprCloseFile(state->file);
    return 0;
}


#if KEEP
static int parseLine(MaState *state, cchar *line)
{
    MaDirective *directive;
    char        *key, *value;

    key = getDirective(sclone(line), &value);
    if (!state->enabled) {
        if (key[0] != '<') {
            mprTrace(8, "Skip: %s %s", key, value);
            return 0;
        }
    }
    if ((directive = mprLookupKey(state->appweb->directives, key)) == 0) {
        mprError("Unknown directive \"%s\"\nAt line %d in %s\n\n", key, state->lineNumber, state->filename);
        return MPR_ERR_BAD_SYNTAX;
    }
    state->key = key;
    mprTrace(8, "Line %d, Parse %s %s", state->lineNumber, key, value ? value : "");
    if ((*directive)(state, key, value) < 0) {
        mprError("Error with directive \"%s\"\nAt line %d in %s\n\n", key, state->lineNumber, state->filename);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}
#endif


#if !ME_ROM
/*
    AccessLog path
    AccessLog conf/log.conf size=10K, backup=5, append, anew
 */
static int accessLogDirective(MaState *state, cchar *key, cchar *value)
{
    char        *option, *ovalue, *tok, *path;
    ssize       size;
    int         flags, backup;

    size = MAXINT;
    backup = 0;
    flags = 0;
    path = 0;
    
    for (option = maGetNextArg(sclone(value), &tok); option; option = maGetNextArg(tok, &tok)) {
        if (!path) {
            path = sclone(option);
        } else {
            option = stok(option, " =\t,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
            if (smatch(option, "size")) {
                size = (ssize) getnum(ovalue);

            } else if (smatch(option, "backup")) {
                backup = atoi(ovalue);

            } else if (smatch(option, "append")) {
                flags |= MPR_LOG_APPEND;

            } else if (smatch(option, "anew")) {
                flags |= MPR_LOG_ANEW;

            } else {
                mprError("Unknown AccessLog option %s", option);
            }
        }
    }
    if (size < (10 * 1024)) {
        mprError("Size is too small. Must be larger than 10K");
        return MPR_ERR_BAD_SYNTAX;
    }
    if (path == 0) {
        mprError("Missing filename");
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteLog(state->route, httpMakePath(state->route, state->configDir, path), size, backup, ME_HTTP_LOG_FORMAT, flags);
    return 0;
}
#endif


/*
    AddFilter filter [ext ext ext ...]
 */
static int addFilterDirective(MaState *state, cchar *key, cchar *value)
{
    char    *filter, *extensions;

    if (!maTokenize(state, value, "%S ?*", &filter, &extensions)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpAddRouteFilter(state->route, filter, extensions, HTTP_STAGE_RX | HTTP_STAGE_TX) < 0) {
        mprError("Cannot add filter %s", filter);
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    AddInputFilter filter [ext ext ext ...]
 */
static int addInputFilterDirective(MaState *state, cchar *key, cchar *value)
{
    char    *filter, *extensions;

    if (!maTokenize(state, value, "%S ?*", &filter, &extensions)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpAddRouteFilter(state->route, filter, extensions, HTTP_STAGE_RX) < 0) {
        mprError("Cannot add filter %s", filter);
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    AddLanguageSuffix lang ext [position]
    AddLanguageSuffix en .en before
 */
static int addLanguageSuffixDirective(MaState *state, cchar *key, cchar *value)
{
    char    *lang, *ext, *position;
    int     flags;

    if (!maTokenize(state, value, "%S %S ?S", &lang, &ext, &position)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    flags = 0;
    if (scaselessmatch(position, "after")) {
        flags |= HTTP_LANG_AFTER;
    } else if (scaselessmatch(position, "before")) {
        flags |= HTTP_LANG_BEFORE;
    }
    httpAddRouteLanguageSuffix(state->route, lang, ext, flags);
    return 0;
}


/*
    AddLanguageDir lang path
 */
static int addLanguageDirDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    char        *lang, *path;

    route = state->route;
    if (!maTokenize(state, value, "%S %S", &lang, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if ((path = stemplate(path, route->vars)) == 0) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (mprIsPathRel(path)) {
        path = mprJoinPath(route->documents, path);
    }
    httpAddRouteLanguageDir(route, lang, mprGetAbsPath(path));
    return 0;
}


/*
    AddOutputFilter filter [ext ext ...]
 */
static int addOutputFilterDirective(MaState *state, cchar *key, cchar *value)
{
    char    *filter, *extensions;

    if (!maTokenize(state, value, "%S ?*", &filter, &extensions)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpAddRouteFilter(state->route, filter, extensions, HTTP_STAGE_TX) < 0) {
        mprError("Cannot add filter %s", filter);
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    AddHandler handler [ext ext ...]
 */
static int addHandlerDirective(MaState *state, cchar *key, cchar *value)
{
    char        *handler, *extensions, *path;
    static int  espLoaded = 0;

    if (!maTokenize(state, value, "%S ?*", &handler, &extensions)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (!extensions) {
        extensions = "";
    } else if (smatch(extensions, "*")) {
        extensions = "";
    }
    if (httpAddRouteHandler(state->route, handler, extensions) < 0) {
        mprError("Cannot add handler %s", handler);
        return MPR_ERR_CANT_CREATE;
    }
    if (smatch(handler, "espHandler") && !espLoaded) {
        path = "./esp.conf";
        if (!mprPathExists(path, R_OK)) {
            path = httpMakePath(state->route, state->configDir, "${BIN_DIR}/esp.conf");
        }
        if (mprPathExists(path, R_OK)) {
            espLoaded = 1;
            if (maParseFile(state, path) < 0) {
                return MPR_ERR_CANT_OPEN;
            }
        } else {
            mprLog(0, "Cannot find esp.conf at %s", path);
        }
    }
    return 0;
}


/*
    AddType mimeType ext
 */
static int addTypeDirective(MaState *state, cchar *key, cchar *value)
{
    char    *ext, *mimeType;

    if (!maTokenize(state, value, "%S %S", &mimeType, &ext)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    mprAddMime(state->route->mimeTypes, mimeType, ext);
    return 0;
}


/*
    Alias /uriPrefix /path
 */
static int aliasDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *alias;
    MprPath     info;
    char        *prefix, *path;

    if (!maTokenize(state, value, "%S %P", &prefix, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    mprGetPathInfo(path, &info);
    if (info.isDir) {
        alias = httpCreateAliasRoute(state->route, prefix, path, 0);
        if (sends(prefix, "/")) {
            httpSetRoutePattern(alias, sfmt("^%s(.*)$", prefix), 0);
        } else {
            /* Add a non-capturing optional trailing "/" */
            httpSetRoutePattern(alias, sfmt("^%s(?:/)*(.*)$", prefix), 0);
        }
        httpSetRouteTarget(alias, "run", "$1");
    } else {
        alias = httpCreateAliasRoute(state->route, sjoin("^", prefix, NULL), 0, 0);
        httpSetRouteTarget(alias, "run", path);
    }
    httpFinalizeRoute(alias);
    return 0;
}


/*
    Allow
 */
static int allowDirective(MaState *state, cchar *key, cchar *value)
{
    char    *from, *spec;

    if (!maTokenize(state, value, "%S %S", &from, &spec)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetAuthAllow(state->auth, spec);
    return addCondition(state, "allowDeny", 0, 0);
}


/*
    AuthGroupFile path
 */
static int authGroupFileDirective(MaState *state, cchar *key, cchar *value)
{
    mprError("The AuthGroupFile directive is deprecated. Use new User/Group directives instead.");
    return 0;
}


/*
    AuthStore NAME
 */
static int authStoreDirective(MaState *state, cchar *key, cchar *value)
{
    if (httpSetAuthStore(state->auth, value) < 0) {
        mprError("The %s AuthStore is not available on this platform", value);
        return configError(state, key);
    }
    return 0;
}


/*
    AuthRealm name
    DEPRECATED
 */
static int authRealmDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetAuthRealm(state->auth, strim(value, "\"'", MPR_TRIM_BOTH));
    return 0;
}


/*
    AuthType basic|digest [realm]
    AuthType form realm login-form [login-service logout-service logged-in]
 */
static int authTypeDirective(MaState *state, cchar *key, cchar *value)
{
    char    *type, *details, *loginPage, *loginService, *logoutService, *loggedIn, *realm;

    if (!maTokenize(state, value, "%S ?S ?*", &type, &realm, &details)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpSetAuthType(state->auth, type, details) < 0) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (realm) {
        httpSetAuthRealm(state->auth, strim(realm, "\"'", MPR_TRIM_BOTH));
    } else if (!state->auth->realm) {
        /* Try to detect users forgetting to define a realm */
        mprError("Must define an AuthRealm before defining the AuthType");
    }
    if (smatch(type, "form")) {
        if (!maTokenize(state, details, "%S ?S ?S ?S", &loginPage, &loginService, &logoutService, &loggedIn)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        httpSetAuthForm(state->route, loginPage, loginService, logoutService, loggedIn);
    }
    return addCondition(state, "auth", 0, 0);
}


/*
    AuthUserFile path
 */
static int authUserFileDirective(MaState *state, cchar *key, cchar *value)
{
    mprError("The AuthGroupFile directive is deprecated. Use new User/Group directives instead.");
    return 0;
}


/*
    AuthAutoLogin username
 */
static int authAutoLoginDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *username;

    if (!maTokenize(state, value, "%S", &username)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetAuthUsername(state->auth, username);
    return 0;
}


/*
    AuthDigestQop none|auth
    Note: auth-int is unsupported
 */
static int authDigestQopDirective(MaState *state, cchar *key, cchar *value)
{
    if (!scaselessmatch(value, "none") && !scaselessmatch(value, "auth")) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetAuthQop(state->auth, value);
    return 0;
}


/*
    Cache options
    Options:
        server=lifespan
        client=lifespan
        extensions="html,gif,..."
        methods="GET,PUT,*,..."
        types="mime-type,*,..."
        unique
 */
static int cacheDirective(MaState *state, cchar *key, cchar *value)
{
    MprTicks    lifespan, clientLifespan, serverLifespan;
    char        *option, *ovalue, *tok;
    char        *methods, *extensions, *types, *uris;
    int         flags;

    flags = 0;
    lifespan = clientLifespan = serverLifespan = 0;
    methods = uris = extensions = types = 0;

    for (option = stok(sclone(value), " \t", &tok); option; option = stok(0, " \t", &tok)) {
        if (*option == '/') {
            uris = option;
            if (tok) {
                /* Join the rest of the options back into one list of URIs */
                tok[-1] = ',';
            }
            break;
        }
        option = stok(option, " =\t,", &ovalue);
        ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
        if ((int) isdigit((uchar) *option)) {
            lifespan = httpGetTicks(option);

        } else if (smatch(option, "client")) {
            flags |= HTTP_CACHE_CLIENT;
            if (ovalue) {
                clientLifespan = httpGetTicks(ovalue);
            }

        } else if (smatch(option, "server")) {
            flags |= HTTP_CACHE_SERVER;
            if (ovalue) {
                serverLifespan = httpGetTicks(ovalue);
            }

        } else if (smatch(option, "extensions")) {
            extensions = ovalue;

        } else if (smatch(option, "types")) {
            types = ovalue;

        } else if (smatch(option, "unique")) {
            flags |= HTTP_CACHE_UNIQUE;

        } else if (smatch(option, "manual")) {
            flags |= HTTP_CACHE_MANUAL;

        } else if (smatch(option, "methods")) {
            methods = ovalue;

        } else {
            mprError("Unknown Cache option '%s'", option);
            return MPR_ERR_BAD_SYNTAX;
        }
    }
    if (lifespan > 0 && !uris && !extensions && !types && !methods) {
        state->route->lifespan = lifespan;
    } else {
        httpAddCache(state->route, methods, uris, extensions, types, clientLifespan, serverLifespan, flags);
    }
    return 0;
}


/*
    Chroot path
 */
static int chrootDirective(MaState *state, cchar *key, cchar *value)
{
#if ME_UNIX_LIKE
    MprKey  *kp;
    cchar   *oldConfigDir;
    char    *home;
    
    home = httpMakePath(state->route, state->configDir, value);
    if (chdir(home) < 0) {
        mprError("Cannot change working directory to %s", home);
        return MPR_ERR_CANT_OPEN;
    }
    if (state->http->flags & HTTP_UTILITY) {
        /* Not running a web server but rather a utility like the "esp" generator program */
        mprLog(MPR_INFO, "Change directory to: \"%s\"", home);
    } else {
        if (chroot(home) < 0) {
            if (errno == EPERM) {
                mprError("Must be super user to use chroot\n");
            } else {
                mprError("Cannot change change root directory to %s, errno %d\n", home, errno);
            }
            return MPR_ERR_BAD_SYNTAX;
        }
        /*
            Remap directories
         */
        oldConfigDir = state->configDir;
        state->configDir = mprGetAbsPath(mprGetRelPath(state->configDir, home));
        state->route->documents = mprGetAbsPath(mprGetRelPath(state->route->documents, home));
        state->route->home = state->route->documents;
        for (ITERATE_KEYS(state->route->vars, kp)) {
            if (sstarts(kp->data, oldConfigDir)) {
                kp->data = mprGetAbsPath(mprGetRelPath(kp->data, oldConfigDir));
            }
        }
        mprLog(MPR_INFO, "Chroot to: \"%s\"", home);
    }
    return 0;
#else
    mprError("Chroot directive not supported on this operating system\n");
    return MPR_ERR_BAD_SYNTAX;
#endif
}


/*
    </Route>, </Location>, </Directory>, </VirtualHost>, </If>
 */
static int closeDirective(MaState *state, cchar *key, cchar *value)
{
    /*
        The order of route finalization will be from the inside. Route finalization causes the route to be added
        to the enclosing host. This ensures that nested routes are defined BEFORE outer/enclosing routes.
     */
    if (state->route != state->prev->route) {
        httpFinalizeRoute(state->route);
    }
    maPopState(state);
    return 0;
}


#if DEPRECATED || 1
/*
    Compress [gzip|none]
 */
static int compressDirective(MaState *state, cchar *key, cchar *value)
{
    char    *format;

    if (!maTokenize(state, value, "%S", &format)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scaselessmatch(format, "gzip") || scaselessmatch(format, "on")) {
        httpSetRouteCompression(state->route, HTTP_ROUTE_GZIP);

    } else if (scaselessmatch(format, "none") || scaselessmatch(format, "off")) {
        httpSetRouteCompression(state->route, 0);
    }
    return 0;
}
#endif


/*
    Condition [!] auth
    Condition [!] condition
    Condition [!] exists string
    Condition [!] directory string
    Condition [!] match string valuePattern
    Condition [!] secure
    Condition [!] unauthorized

    Strings can contain route->pattern and request ${tokens}
 */
static int conditionDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *details;
    int     not;

    if (!maTokenize(state, value, "%! ?S ?*", &not, &name, &details)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    return addCondition(state, name, details, not ? HTTP_ROUTE_NOT : 0);
}


/*
    CrossOrigin  origin=[client|all|*|NAME] [credentials=[yes|no]] [headers=HDR,...] [age=NN]
 */
static int crossOriginDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    char        *option, *ovalue, *tok;

    route = state->route;
    tok = sclone(value);
    while ((option = maGetNextArg(tok, &tok)) != 0) {
        option = stok(option, " =\t,", &ovalue);
        ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
        if (scaselessmatch(option, "origin")) {
            route->corsOrigin = sclone(ovalue);

        } else if (scaselessmatch(option, "credentials")) {
            route->corsCredentials = httpGetBoolToken(ovalue);

        } else if (scaselessmatch(option, "headers")) {
            route->corsHeaders = sclone(ovalue);

        } else if (scaselessmatch(option, "age")) {
            route->corsAge = atoi(ovalue);

        } else if (scaselessmatch(option, "methods")) {
            route->corsMethods = sclone(ovalue);

        } else {
            mprError("Unknown CrossOrigin option %s", option);
            return MPR_ERR_BAD_SYNTAX;
        }
    }
    /*
        Need the options method for pre-flight requests
     */
    httpAddRouteMethods(route, "OPTIONS");
    route->flags |= HTTP_ROUTE_CORS;
    return 0;
}


/*
    Defense name [Arg=Value]...

    Remedies: ban, cmd, delay, email, http, log
    Args: CMD, DELAY, FROM, IP, MESSAGE, PERIOD, STATUS, SUBJECT, TO, METHOD, URI
    Examples:
    Examples:
        Defense block REMEDY=ban PERIOD=30mins
        Defense report REMEDY=http URI=http://example.com/report
        Defense alarm REMEDY=cmd CMD="afplay klaxon.mp3"
        Defense slow REMEDY=delay PERIOD=10mins DELAY=1sec
        Defense fix REMEDY=cmd CMD="${MESSAGE} | sendmail admin@example.com"
        Defense notify REMEDY=email TO=info@example.com
        Defense firewall REMEDY=cmd CMD="iptables -A INPUT -s ${IP} -j DROP"
        Defense reboot REMEDY=restart 
 */
static int defenseDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *name, *args; 

    if (!maTokenize(state, value, "%S ?*", &name, &args)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddDefense(name, NULL, args);
    return 0;
}


static int defaultLanguageDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteDefaultLanguage(state->route, value);
    return 0;
}


/*
    Deny "from" address
 */
static int denyDirective(MaState *state, cchar *key, cchar *value)
{
    char    *from, *spec;

    if (!maTokenize(state, value, "%S %S", &from, &spec)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetAuthDeny(state->auth, spec);
    return addCondition(state, "allowDeny", 0, 0);
}


/*
    <Directory path>
 */
static int directoryDirective(MaState *state, cchar *key, cchar *value)
{
    /*
        Directory must be deprecated because Auth directives inside a directory block applied to physical filenames.
        The router and Route directives can't emulate this. The user needs to migrate such configurations to apply
        Auth directives to route URIs instead.
     */
    mprError("The <Directory> directive is deprecated. Use <Route> with a Documents directive instead.");
    return MPR_ERR_BAD_SYNTAX;
}


/*
    DirectoryIndex paths...
 */
static int directoryIndexDirective(MaState *state, cchar *key, cchar *value)
{
    char   *path, *tok;

    for (path = stok(sclone(value), " \t,", &tok); path; path = stok(0, " \t,", &tok)) {
        httpAddRouteIndex(state->route, path);
    }
    return 0;
}


/*
    Documents path
    DocumentRoot path
 */
static int documentsDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *path;

    if (!maTokenize(state, value, "%T", &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    path = mprJoinPath(state->configDir, httpExpandRouteVars(state->route, path));
    httpSetRouteDocuments(state->route, path);
    return 0;
}


/*
    <else>
 */
static int elseDirective(MaState *state, cchar *key, cchar *value)
{
    state->enabled = !state->enabled;
    return 0;
}


/*
    ErrorDocument status URI
 */
static int errorDocumentDirective(MaState *state, cchar *key, cchar *value)
{
    char    *uri;
    int     status;

    if (!maTokenize(state, value, "%N %S", &status, &uri)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddRouteErrorDocument(state->route, status, uri);
    return 0;
}


/*
    ErrorLog path
        [size=bytes] 
        [level=0-9] 
        [backup=count] 
        [append] 
        [anew]
        [stamp=period]
 */
static int errorLogDirective(MaState *state, cchar *key, cchar *value)
{
    MprTicks    stamp;
    char        *option, *ovalue, *tok, *path;
    ssize       size;
    int         level, flags, backup;

    if (mprGetCmdlineLogging()) {
        mprLog(4, "Already logging. Ignoring ErrorLog directive");
        return 0;
    }
    size = MAXINT;
    stamp = 0;
    level = 0;
    backup = 0;
    path = 0;
    flags = 0;

    for (option = maGetNextArg(sclone(value), &tok); option; option = maGetNextArg(tok, &tok)) {
        if (!path) {
            path = mprJoinPath(httpGetRouteVar(state->route, "LOG_DIR"), httpExpandRouteVars(state->route, option));
        } else {
            option = stok(option, " =\t,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
            if (smatch(option, "size")) {
                size = (ssize) getnum(ovalue);

            } else if (smatch(option, "level")) {
                level = atoi(ovalue);

            } else if (smatch(option, "backup")) {
                backup = atoi(ovalue);

            } else if (smatch(option, "append")) {
                flags |= MPR_LOG_APPEND;

            } else if (smatch(option, "anew")) {
                flags |= MPR_LOG_ANEW;

            } else if (smatch(option, "stamp")) {
                stamp = httpGetTicks(ovalue);

            } else {
                mprError("Unknown ErrorLog option %s", option);
            }
        }
    }
    if (size < (10 * 1000)) {
        mprError("Size is too small. Must be larger than 10K");
        return MPR_ERR_BAD_SYNTAX;
    }
    if (path == 0) {
        mprError("Missing filename");
        return MPR_ERR_BAD_SYNTAX;
    }
    mprSetLogBackup(size, backup, flags);

    if (sncmp(path, "stdout", 6) != 0 && sncmp(path, "stderr", 6) != 0) {
        path = httpMakePath(state->route, state->configDir, path);
    }
    if (mprStartLogging(path, 0) < 0) {
        mprError("Cannot write to ErrorLog: %s", path);
        return MPR_ERR_BAD_SYNTAX;
    }
    mprSetLogLevel(level);
    mprLogHeader();
    if (stamp) {
        httpSetTimestamp(stamp);
    }
    return 0;
}


/*
    ExitTimeout timeout
 */
static int exitTimeoutDirective(MaState *state, cchar *key, cchar *value)
{
    mprSetExitTimeout(httpGetTicks(value));
    return 0;
}


/*
    GroupAccount groupName
 */
static int groupAccountDirective(MaState *state, cchar *key, cchar *value)
{
    if (!smatch(value, "_unchanged_") && !mprGetDebugMode()) {
        maSetHttpGroup(state->appweb, value);
    }
    return 0;
}


/*
    Header [add|append|remove|set] name value
 */
static int headerDirective(MaState *state, cchar *key, cchar *value)
{
    char    *cmd, *header, *hvalue;
    int     op;

    if (!maTokenize(state, value, "%S %S ?*", &cmd, &header, &hvalue)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scaselessmatch(cmd, "add")) {
        op = HTTP_ROUTE_ADD_HEADER;
    } else if (scaselessmatch(cmd, "append")) {
        op = HTTP_ROUTE_APPEND_HEADER;
    } else if (scaselessmatch(cmd, "remove")) {
        op = HTTP_ROUTE_REMOVE_HEADER;
    } else if (scaselessmatch(cmd, "set")) {
        op = HTTP_ROUTE_SET_HEADER;
    } else {
        mprError("Unknown Header directive operation: %s", cmd);
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddRouteResponseHeader(state->route, op, header, hvalue);
    return 0;
}


/*
    Home path
 */
static int homeDirective(MaState *state, cchar *key, cchar *value)
{
    char    *path;

    if (!maTokenize(state, value, "%T", &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteHome(state->route, path);
    mprLog(MPR_INFO, "Server Root \"%s\"", path);
    return 0;
}


/*
    IgnoreEncodingErrors [on|off]
 */
static int ignoreEncodingErrorsDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteIgnoreEncodingErrors(state->route, on);
    return 0;
}


/*
    <Include pattern>
 */
static int includeDirective(MaState *state, cchar *key, cchar *value)
{
    MprList     *includes;
    char        *path, *pattern, *include;
    int         next;

    /*
        Must use %S and not %P because the path is relative to the appweb.conf file and not to the route home
     */
    if (!maTokenize(state, value, "%S", &value)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    value = mprGetAbsPath(mprJoinPath(state->configDir, httpExpandRouteVars(state->route, value)));

    if (strpbrk(value, "^$*+?([|{") == 0) {
        if (maParseFile(state, value) < 0) {
            return MPR_ERR_CANT_OPEN;
        }
    } else {
        path = mprGetPathDir(mprJoinPath(state->route->home, value));
        path = stemplate(path, state->route->vars);
        pattern = mprGetPathBase(value);
        includes = mprGlobPathFiles(path, pattern, 0);
        for (ITERATE_ITEMS(includes, include, next)) {
            if (maParseFile(state, include) < 0) {
                return MPR_ERR_CANT_OPEN;
            }
        }
    }
    return 0;
}


/*
    <If DEFINITION>
 */
static int ifDirective(MaState *state, cchar *key, cchar *value)
{
    state = maPushState(state);
    if (state->enabled) {
        state->enabled = conditionalDefinition(state, value);
        if (!state->enabled) {
            mprTrace(7, "If \"%s\" conditional is false at %s:%d", value, state->filename, state->lineNumber);
        }
    }
    return 0;
}


/*
    InactivityTimeout secs
 */
static int inactivityTimeoutDirective(MaState *state, cchar *key, cchar *value)
{
    if (! mprGetDebugMode()) {
        httpGraduateLimits(state->route, 0);
        state->route->limits->inactivityTimeout = httpGetTicks(value);
    }
    return 0;
}


/*
    LimitBuffer bytes
    DEPRECATED: LimitStageBuffer bytes
 */
static int limitBufferDirective(MaState *state, cchar *key, cchar *value)
{
    int     size;

    httpGraduateLimits(state->route, 0);
    size = getint(value);
    if (size > (1024 * 1024)) {
        size = (1024 * 1024);
    }
    state->route->limits->bufferSize = size;
    return 0;
}


/*
    LimitCache bytes
 */
static int limitCacheDirective(MaState *state, cchar *key, cchar *value)
{
    mprSetCacheLimits(state->host->responseCache, 0, 0, getnum(value), 0);
    return 0;
}


/*
    LimitCacheItem bytes
 */
static int limitCacheItemDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->cacheItemSize = getint(value);
    return 0;
}


/*
    LimitChunk bytes
 */
static int limitChunkDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->chunkSize = getint(value);
    return 0;
}


/*
    LimitClients count
 */
static int limitClientsDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->clientMax = getint(value);
    return 0;
}


/*
    LimitConnections count
 */
static int limitConnectionsDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->connectionsMax = getint(value);
    return 0;
}


/*
    LimitFiles count
 */
static int limitFilesDirective(MaState *state, cchar *key, cchar *value)
{
#if ME_UNIX_LIKE
    mprSetFilesLimit(getint(value));
#endif
    return 0;
}


/*
    LimitMemory size

    Redline set to 85%
 */
static int limitMemoryDirective(MaState *state, cchar *key, cchar *value)
{
    ssize   maxMem;

    maxMem = (ssize) getnum(value);
    mprSetMemLimits(maxMem / 100 * 85, maxMem, -1);
    return 0;
}


/*
    LimitProcesses count
 */
static int limitProcessesDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->processMax = getint(value);
    return 0;
}


/*
    LimitRequests count
    DEPRECATED 4.4
 */
static int limitRequestsDirective(MaState *state, cchar *key, cchar *value)
{
    mprError("The LimitRequests directive is deprecated. Use LimitConnections or LimitRequestsPerClient instead.");
    return 0;
}


/*
    LimitRequestsPerClient count
 */
static int limitRequestsPerClientDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->requestsPerClientMax = getint(value);
    return 0;
}


/*
    LimitRequestBody bytes
 */
static int limitRequestBodyDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->receiveBodySize = getnum(value);
    return 0;
}


/*
    LimitRequestForm bytes
 */
static int limitRequestFormDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->receiveFormSize = getnum(value);
    return 0;
}


/*
    LimitRequestHeaderLines count
 */
static int limitRequestHeaderLinesDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->headerMax = getint(value);
    return 0;
}


/*
    LimitRequestHeader bytes
 */
static int limitRequestHeaderDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->headerSize = getint(value);
    return 0;
}


/*
    LimitResponseBody bytes
 */
static int limitResponseBodyDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->transmissionBodySize = getnum(value);
    return 0;
}




/*
    LimitSessions count
 */
static int limitSessionDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->sessionMax = getint(value);
    return 0;
}


/*
    LimitUri bytes
 */
static int limitUriDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->uriSize = getint(value);
    return 0;
}


/*
    LimitUpload bytes
 */
static int limitUploadDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->uploadSize = getnum(value);
    return 0;
}


/*
    Listen ip:port      Listens only on the specified interface
    Listen ip           Listens only on the specified interface with the default port
    Listen port         Listens on both IPv4 and IPv6

    Where ip may be "::::::" for ipv6 addresses or may be enclosed in "[::]" if appending a port.
    Can provide http:// and https:// prefixes.
 */
static int listenDirective(MaState *state, cchar *key, cchar *value)
{
    HttpEndpoint    *endpoint;
    char            *address, *ip;
    int             port;

    if (!maTokenize(state, value, "%S", &address)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    mprParseSocketAddress(address, &ip, &port, NULL, 80);
    if (port == 0) {
        mprError("Bad or missing port %d in Listen directive", port);
        return -1;
    }
    endpoint = httpCreateEndpoint(ip, port, NULL);
    mprAddItem(state->server->endpoints, endpoint);
    if (!state->host->defaultEndpoint) {
        httpSetHostDefaultEndpoint(state->host, endpoint);
    }
    /*
        Single stack networks cannot support IPv4 and IPv6 with one socket. So create a specific IPv6 endpoint.
        This is currently used by VxWorks and Windows versions prior to Vista (i.e. XP)
     */
    if (!schr(address, ':') && mprHasIPv6() && !mprHasDualNetworkStack()) {
        mprAddItem(state->server->endpoints, httpCreateEndpoint("::", port, NULL));
    }
    return 0;
}


/*
    ListenSecure ip:port
    ListenSecure ip
    ListenSecure port

    Where ip may be "::::::" for ipv6 addresses or may be enclosed in "[]" if appending a port.
 */
static int listenSecureDirective(MaState *state, cchar *key, cchar *value)
{
#if ME_COM_SSL
    HttpEndpoint    *endpoint;
    char            *address, *ip;
    int             port;

    if (!maTokenize(state, value, "%S", &address)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    mprParseSocketAddress(address, &ip, &port, NULL, 443);
    if (port == 0) {
        mprError("Bad or missing port %d in ListenSecure directive", port);
        return -1;
    }
    endpoint = httpCreateEndpoint(ip, port, NULL);
    mprAddItem(state->server->endpoints, endpoint);
    if (state->route->ssl == 0) {
        if (state->route->parent && state->route->parent->ssl) {
            state->route->ssl = mprCloneSsl(state->route->parent->ssl);
        } else {
            state->route->ssl = mprCreateSsl(1);
        }
    }
    httpSecureEndpoint(endpoint, state->route->ssl);
    if (!state->host->secureEndpoint) {
        httpSetHostSecureEndpoint(state->host, endpoint);
    }
    /*
        Single stack networks cannot support IPv4 and IPv6 with one socket. So create a specific IPv6 endpoint.
        This is currently used by VxWorks and Windows versions prior to Vista (i.e. XP)
     */
    if (!schr(address, ':') && mprHasIPv6() && !mprHasDualNetworkStack()) {
        endpoint = httpCreateEndpoint("::", port, NULL);
        mprAddItem(state->server->endpoints, endpoint);
        httpSecureEndpoint(endpoint, state->route->ssl);
    }
    return 0;
#else
    mprError("Configuration lacks SSL support");
    return -1;
#endif
}


/*
    Log options
    Options:
        tx|rx
        conn=NN
        first=NN
        header=NN
        body=NN
        time=NN
        size=NN
        include="ext,ext,ext..."
        exclude="ext,ext,ext..."
 */
static int logDirective(MaState *state, cchar *key, cchar *value)
{
    char        *option, *ovalue, *tok, *include, *exclude;
    ssize       size;
    int         i, dir, levels[HTTP_TRACE_MAX_ITEM];

    include = exclude = 0;
    dir = HTTP_TRACE_RX;
    size = MAXINT;
    
    for (i = 0; i < HTTP_TRACE_MAX_ITEM; i++) {
        levels[i] = 0;
    }
    for (option = stok(sclone(value), " \t", &tok); option; option = stok(0, " \t", &tok)) {
        option = stok(option, " =\t,", &ovalue);
        ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
        if (smatch(option, "tx")) {
            dir = HTTP_TRACE_TX;

        } else if (smatch(option, "rx")) {
            dir = HTTP_TRACE_RX;

        } else if (smatch(option, "conn")) {
            levels[HTTP_TRACE_CONN] = atoi(ovalue);

        } else if (smatch(option, "first")) {
            levels[HTTP_TRACE_FIRST] = atoi(ovalue);

        } else if (smatch(option, "headers")) {
            levels[HTTP_TRACE_HEADER] = atoi(ovalue);

        } else if (smatch(option, "body")) {
            levels[HTTP_TRACE_BODY] = atoi(ovalue);

        } else if (smatch(option, "limits")) {
            levels[HTTP_TRACE_LIMITS] = atoi(ovalue);

        } else if (smatch(option, "time")) {
            levels[HTTP_TRACE_TIME] = atoi(ovalue);

        } else if (smatch(option, "size")) {
            size = (ssize) getnum(ovalue);

        } else if (smatch(option, "include")) {
            include = ovalue;

        } else if (smatch(option, "exclude")) {
            exclude = ovalue;

        } else {
            mprError("Unknown Cache option '%s'", option);
            return MPR_ERR_BAD_SYNTAX;
        }
    }
    httpSetRouteTraceFilter(state->route, dir, levels, size, include, exclude);
    return 0;
}


/*
    LogRoutes [full]
    Support two formats line for one line, and multiline with more fields
 */
static int logRoutesDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *full;

    if (!maTokenize(state, value, "?S", &full)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (!(state->flags & MA_PARSE_NON_SERVER)) {
        mprRawLog(0, "\nHTTP Routes for the '%s' host:\n\n", state->host->name ? state->host->name : "default");
        httpLogRoutes(state->host, smatch(full, "full"));
    }
    return 0;
}


/*
    LoadModulePath searchPath
 */
static int loadModulePathDirective(MaState *state, cchar *key, cchar *value)
{
    char    *sep, *path;

    if (!maTokenize(state, value, "%T", &value)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    /*
         Search path is: USER_SEARCH : exeDir : /usr/lib/appweb/bin
     */
    sep = MPR_SEARCH_SEP;
    value = stemplate(value, state->route->vars);
    path = sjoin(value, sep, mprGetAppDir(), sep, ME_VAPP_PREFIX "/bin", NULL);
    mprSetModuleSearchPath(path);
    return 0;
}


/*
    LoadModule name path
 */
static int loadModuleDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *path;

    if (!maTokenize(state, value, "%S %S", &name, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (maLoadModule(state->appweb, name, path) < 0) {
        /*  Error messages already done */
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    LimitKeepAlive count
 */
static int limitKeepAliveDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->keepAliveMax = getint(value);
    return 0;
}


/*
    LimitWorkers count
 */
static int limitWorkersDirective(MaState *state, cchar *key, cchar *value)
{
    int     count;

    count = atoi(value);
    if (count < 1) {
        count = MAXINT;
    }
    mprSetMaxWorkers(count);
    return 0;
}


static int userToID(cchar *user)
{
#if ME_UNIX_LIKE
    struct passwd   *pp;
    if ((pp = getpwnam(user)) == 0) {
        mprError("Bad user: %s", user);
        return 0;
    }
    return pp->pw_uid;
#else
    return 0;
#endif
}


static int groupToID(cchar *group)
{
#if ME_UNIX_LIKE
    struct group    *gp;
    if ((gp = getgrnam(group)) == 0) {
        mprError("Bad group: %s", group);
        return MPR_ERR_CANT_ACCESS;
    }
    return gp->gr_gid;
#else
    return 0;
#endif
}


/*
    MakeDir owner:group:perms dir, ...
 */
static int makeDirDirective(MaState *state, cchar *key, cchar *value)
{
    MprPath info;
    char    *auth, *dirs, *path, *perms, *tok;
    cchar   *dir, *group, *owner;
    int     gid, mode, uid; 

    if (!maTokenize(state, value, "%S ?*", &auth, &dirs)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    uid = gid = 0;
    mode = 0750;
    if (schr(auth, ':')) {
        owner = stok(auth, ":", &tok);
        if (owner && *owner) {
            if (snumber(owner)) {
                uid = (int) stoi(owner);
            } else if (smatch(owner, "APPWEB")) {
                uid = state->appweb->uid;
            } else {
                uid = userToID(owner);
            }
        }
        group = stok(tok, ":", &perms);
        if (group && *group) {
            if (snumber(group)) {
                gid = (int) stoi(group);
            } else if (smatch(owner, "APPWEB")) {
                gid = state->appweb->gid;
            } else {
                gid = groupToID(group);
            }
        }
        if (perms && snumber(perms)) {
            mode = (int) stoiradix(perms, -1, NULL);
        } else {
            mode = 0;
        }
        if (gid < 0 || uid < 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
    } else {
        dirs = auth;
        auth = 0;
    }
    tok = dirs;
    for (tok = sclone(dirs); (dir = stok(tok, ",", &tok)) != 0; ) {
        path = httpMakePath(state->route, state->configDir, dir);
        if (mprGetPathInfo(path, &info) == 0 && info.isDir) {
            continue;
        }
        mprLog(MPR_INFO, "Create directory: \"%s\"", path);
        if (mprMakeDir(path, mode, uid, gid, 1) < 0) {
            return MPR_ERR_BAD_SYNTAX;
        }
    }
    return 0;
}


/*
    Map "ext,ext,..." "newext, newext, newext"
    Map compressed
    Example: Map "css,html,js,less,xml" min.${1}.gz, min.${1}, ${1}.gz
 */
static int mapDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *extensions, *mappings;

    if (!maTokenize(state, value, "%S ?*", &extensions, &mappings)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (smatch(extensions, "compressed")) {
        httpAddRouteMapping(state->route, "css,html,js,less,txt,xml", "${1}.gz, min.${1}.gz, min.${1}");
    } else {
        httpAddRouteMapping(state->route, extensions, mappings);
    }
    return 0;
}


/*
    MemoryPolicy continue|restart
 */
static int memoryPolicyDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *policy;
    int     flags;

    flags = MPR_ALLOC_POLICY_EXIT;

    if (!maTokenize(state, value, "%S", &policy)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scmp(value, "restart") == 0) {
#if VXWORKS
        flags = MPR_ALLOC_POLICY_RESTART;
#else
        /* Appman will restart */
        flags = MPR_ALLOC_POLICY_EXIT;
#endif
        
    } else if (scmp(value, "continue") == 0) {
        flags = MPR_ALLOC_POLICY_PRUNE;

#if DEPRECATED || 1
    } else if (scmp(value, "exit") == 0) {
        flags = MPR_ALLOC_POLICY_EXIT;

    } else if (scmp(value, "prune") == 0) {
        flags = MPR_ALLOC_POLICY_PRUNE;
#endif

    } else {
        mprError("Unknown memory depletion policy '%s'", policy);
        return MPR_ERR_BAD_SYNTAX;
    }
    mprSetMemPolicy(flags);
    return 0;
}


/*
    Methods [add|remove|set] method, ...
 */
static int methodsDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *cmd, *methods;

    if (!maTokenize(state, value, "%S %*", &cmd, &methods)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (smatch(cmd, "add")) {
        httpAddRouteMethods(state->route, methods);
    } else if (smatch(cmd, "remove")) {
        httpRemoveRouteMethods(state->route, methods);
    } else if (smatch(cmd, "set")) {
        httpSetRouteMethods(state->route, methods);
    }
    return 0;
}


/*
    MinWorkers count
 */
static int minWorkersDirective(MaState *state, cchar *key, cchar *value)
{
    mprSetMinWorkers((int) stoi(value));
    return 0;
}


/*
    Monitor Expression Period Defenses ....
 */
static int monitorDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *counter, *expr, *limit, *period, *relation, *defenses;

    if (!maTokenize(state, value, "%S %S %*", &expr, &period, &defenses)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    expr = strim(expr, "\"", MPR_TRIM_BOTH);
    if (!maTokenize(state, expr, "%S %S %S", &counter, &relation, &limit)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddMonitor(counter, relation, getnum(limit), httpGetTicks(period), defenses);
    return 0;
}


/*
    Name routeName
 */
static int nameDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteName(state->route, value);
    return 0;
}


/*
    NameVirtualHost ip[:port]
 */
static int nameVirtualHostDirective(MaState *state, cchar *key, cchar *value)
{
    char    *ip;
    int     port;

    mprTrace(4, "NameVirtual Host: %s ", value);
    mprParseSocketAddress(value, &ip, &port, NULL, -1);
    httpConfigureNamedVirtualEndpoints(state->http, ip, port);
    return 0;
}


/*
    Order Allow,Deny
    Order Deny,Allow
 */
static int orderDirective(MaState *state, cchar *key, cchar *value)
{
    if (scaselesscmp(value, "Allow,Deny") == 0) {
        httpSetAuthOrder(state->auth, HTTP_ALLOW_DENY);
    } else if (scaselesscmp(value, "Deny,Allow") == 0) {
        httpSetAuthOrder(state->auth, HTTP_DENY_ALLOW);
    } else {
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


/*
    Param [!] name valuePattern
 */
static int paramDirective(MaState *state, cchar *key, cchar *value)
{
    char    *field;
    int     not;

    if (!maTokenize(state, value, "?! %S %*", &not, &field, &value)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddRouteParam(state->route, field, value, not ? HTTP_ROUTE_NOT : 0);
    return 0;
}


/*
    Prefix /URI-PREFIX
    NOTE: For nested routes, the prefix value will be appended out any existing parent route prefix.
    NOTE: Prefixes do append, but route patterns do not.
 */
static int prefixDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRoutePrefix(state->route, value);
    httpSetRouteVar(state->route, "PREFIX", value);
    return 0;
}


#if DEPRECATED
/*
    Protocol HTTP/1.0
    Protocol HTTP/1.1
 */
static int protocolDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteProtocol(state->host, value);
    if (!scaselessmatch(value, "HTTP/1.0") && !scaselessmatch(value, "HTTP/1.1")) {
        mprError("Unknown http protocol %s. Should be HTTP/1.0 or HTTP/1.1", value);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}
#endif


#if DEPRECATED || 1
/*
    PutMethod on|off
 */
static int putMethodDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (on) {
        httpAddRouteMethods(state->route, "DELETE, PUT");
    } else {
        httpRemoveRouteMethods(state->route, "DELETE, PUT");
    }
    return 0;
}
#endif


/*
    Redirect [status|permanent|temp|seeother|gone] from to
    Redirect secure
 */
static int redirectDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *alias;
    char        *code, *uri, *path, *target;
    int         status;

    status = 0;
    if (smatch(value, "secure")) {
        uri = "/";
        path = "https://";

    } else if (value[0] == '/' || sncmp(value, "http://", 6) == 0) {
        if (!maTokenize(state, value, "%S %S", &uri, &path)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        status = HTTP_CODE_MOVED_TEMPORARILY;
    } else {
        if (!maTokenize(state, value, "%S %S ?S", &code, &uri, &path)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        if (scaselessmatch(code, "permanent")) {
            status = 301;
        } else if (scaselessmatch(code, "temp")) {
            status = 302;
        } else if (scaselessmatch(code, "seeother")) {
            status = 303;
        } else if (scaselessmatch(code, "gone")) {
            status = 410;
        } else if (scaselessmatch(code, "all")) {
            status = 0;
        } else if (snumber(code)) {
            status = atoi(code);
        } else {
            return configError(state, key);
        }
    }
    if (300 <= status && status <= 399 && (!path || *path == '\0')) {
        return configError(state, key);
    }
    if (status < 0 || uri == 0) {
        return configError(state, key);
    }
    alias = httpCreateAliasRoute(state->route, uri, 0, status);
    target = (path) ? sfmt("%d %s", status, path) : code;
    httpSetRouteTarget(alias, "redirect", target);
    if (smatch(value, "secure")) {
        /* 
            Accept this route if !secure. That will then do a redirect.
            Set details to null to avoid creating Strict-Transport-Security header 
         */
        httpAddRouteCondition(alias, "secure", 0, HTTP_ROUTE_NOT);
    }
    httpFinalizeRoute(alias);
    return 0;
}


/*
    RequestParseTimeout secs
 */
static int requestParseTimeoutDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->requestParseTimeout = httpGetTicks(value);
    return 0;
}


/*
    RequestTimeout secs
 */
static int requestTimeoutDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->requestTimeout = httpGetTicks(value);
    return 0;
}


/*
    Require ability|role|user|valid-user
    Require secure [age=secs] [domains]
 */
static int requireDirective(MaState *state, cchar *key, cchar *value)
{
    char    *age, *type, *rest, *option, *ovalue, *tok;
    int     domains;

    if (!maTokenize(state, value, "%S ?*", &type, &rest)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scaselesscmp(type, "ability") == 0) {
        httpSetAuthRequiredAbilities(state->auth, rest);

    /* Support require group for legacy support */
    } else if (scaselesscmp(type, "group") == 0 || scaselesscmp(type, "role") == 0) {
        httpSetAuthRequiredAbilities(state->auth, rest);

    } else if (scaselesscmp(type, "secure") == 0) {
        domains = 0;
        age = 0;
        for (option = stok(sclone(rest), " \t", &tok); option; option = stok(0, " \t", &tok)) {
            option = stok(option, " =\t,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
            if (smatch(option, "age")) {
                age = sfmt("%lld", (int64) httpGetTicks(ovalue));
            } else if (smatch(option, "domains")) {
                domains = 1;
            }
        }
        if (age) {
            if (domains) {
                /* Negative age signifies subdomains */
                age = sjoin("-1", age, NULL);
            }
        }
        addCondition(state, "secure", age, 0);

#if DEPRECATED || 1 
    } else if (scaselesscmp(type, "user") == 0) {
        /*
            Achieve this via abilities
         */
        httpSetAuthPermittedUsers(state->auth, rest);

    } else if (scaselesscmp(type, "valid-user") == 0) {
        /*
            Achieve this via abilities
         */
        httpSetAuthAnyValidUser(state->auth);

    } else if (scaselesscmp(type, "acl") == 0) {
        mprError("The Require acl directive is deprecated. Use Require ability instead.");
#endif
    } else {
        return configError(state, key);
    }
    return 0;
}


/*
    <Reroute pattern>
    Open an existing route
 */
static int rerouteDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    cchar       *pattern;
    int         not;

    state = maPushState(state);
    if (state->enabled) {
        if (!maTokenize(state, value, "%!%S", &not, &pattern)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        if (strstr(pattern, "${")) {
            pattern = sreplace(pattern, "${inherit}", state->route->pattern);
        }
        pattern = httpExpandRouteVars(state->route, pattern);
        if ((route = httpLookupRouteByPattern(state->host, pattern)) != 0) {
            state->route = route;
        } else {
            mprError("Cannot open route %s", pattern);
            return MPR_ERR_CANT_OPEN;
        }
        /* Routes are added when the route block is closed (see closeDirective) */
        state->auth = state->route->auth;
    }
    return 0;
}


/*
    Reset routes
    Reset pipeline
 */
static int resetDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name;

    if (!maTokenize(state, value, "%S", &name)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (scaselessmatch(name, "routes")) {
        httpResetRoutes(state->host);

    } else if (scaselessmatch(name, "pipeline")) {
        httpResetRoutePipeline(state->route);

    } else {
        return configError(state, name);
    }
    return 0;
}


/*
    ResetPipeline (alias for Reset routes)
    DEPRECATED
 */
static int resetPipelineDirective(MaState *state, cchar *key, cchar *value)
{
    httpResetRoutePipeline(state->route);
    return 0;
}


/*
    Role name abilities...
 */
static int roleDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *abilities;

    if (!maTokenize(state, value, "%S ?*", &name, &abilities)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpAddRole(state->auth, name, abilities) < 0) {
        mprError("Cannot add role %s", name);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


/*
    <Route pattern>
    NOTE: The route pattern should include the prefix when declared. When compiling the pattern, the prefix is removed.
 */
static int routeDirective(MaState *state, cchar *key, cchar *value)
{
    cchar       *pattern;
    int         not;

    state = maPushState(state);
    if (state->enabled) {
        if (!maTokenize(state, value, "%!%S", &not, &pattern)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        if (strstr(pattern, "${")) {
            pattern = sreplace(pattern, "${inherit}", state->route->pattern);
        }
        pattern = httpExpandRouteVars(state->route, pattern);
        state->route = httpCreateInheritedRoute(state->route);
        httpSetRoutePattern(state->route, pattern, not ? HTTP_ROUTE_NOT : 0);
        httpSetRouteHost(state->route, state->host);
        /* Routes are added when the route block is closed (see closeDirective) */
        state->auth = state->route->auth;
    }
    return 0;
}


/*
    RequestHeader [!] name valuePattern

    The given header must [not] be present for the route to match
 */
static int requestHeaderDirective(MaState *state, cchar *key, cchar *value)
{
    char    *header;
    int     not;

    if (!maTokenize(state, value, "?! %S %*", &not, &header, &value)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpAddRouteRequestHeaderCheck(state->route, header, value, not ? HTTP_ROUTE_NOT : 0);
    return 0;
}


/*
    ServerName URI
 */
static int serverNameDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetHostName(state->host, strim(value, "http://", MPR_TRIM_START));
    return 0;
}


/*
    SessionCookie [name=NAME] [visible=true] [persist=true]
    DEPRECATED:
        SessionCookie visible|invisible
 */
static int sessionCookieDirective(MaState *state, cchar *key, cchar *value)
{
    char    *options, *option, *ovalue, *tok;

    if (!maTokenize(state, value, "%*", &options)) {
        return MPR_ERR_BAD_SYNTAX;
    }
#if DEPRECATED || 1
    if (scaselessmatch(value, "visible")) {
        httpSetRouteSessionVisibility(state->route, 1);
    } else if (scaselessmatch(value, "invisible")) {
        httpSetRouteSessionVisibility(state->route, 0);
    }
#endif
    for (option = maGetNextArg(options, &tok); option; option = maGetNextArg(tok, &tok)) {
        option = stok(option, " =\t,", &ovalue);
        ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
        if (!ovalue || *ovalue == '\0') continue;
        if (smatch(option, "visible")) {
            httpSetRouteSessionVisibility(state->route, scaselessmatch(ovalue, "visible"));
        } else if (smatch(option, "name")) {
            httpSetRouteCookie(state->route, ovalue);
        } else if (smatch(option, "persist")) {
            httpSetRouteCookiePersist(state->route, smatch(ovalue, "true"));
        } else {
            mprError("Unknown SessionCookie option %s", option);
            return MPR_ERR_BAD_SYNTAX;
        }
    }
    return 0;
}


/*
    SessionTimeout secs
 */
static int sessionTimeoutDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->sessionTimeout = httpGetTicks(value);
    return 0;
}


/*
    Set var value
 */
static int setDirective(MaState *state, cchar *key, cchar *value)
{
    char    *var;

    if (!maTokenize(state, value, "%S %S", &var, &value)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteVar(state->route, var, value);
    return 0;
}


/*
    SetConnector connector
 */
static int setConnectorDirective(MaState *state, cchar *key, cchar *value)
{
    if (httpSetRouteConnector(state->route, value) < 0) {
        mprError("Cannot add handler %s", value);
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    SetHandler handler
 */
static int setHandlerDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name;

    if (!maTokenize(state, value, "%S", &name)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpSetRouteHandler(state->route, name) < 0) {
        mprError("Cannot add handler %s", name);
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


/*
    ShowErrors on|off
 */
static int showErrorsDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteShowErrors(state->route, on);
    return 0;
}


/*
    Source path
 */
static int sourceDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteSource(state->route, value);
    return 0;
}


/*
    Stealth on|off
 */
static int stealthDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteStealth(state->route, on);
    return 0;
}



/*
    StreamInput [!] mimeType [uri]
 */
static int streamInputDirective(MaState *state, cchar *key, cchar *value)
{
    cchar   *mime, *uri;
    int     disable;

    if (!maTokenize(state, value, "%! ?S ?S", &disable, &mime, &uri)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetStreaming(state->host, mime, uri, !disable);
    return 0;
}


/*
    Target close [immediate]
    Target redirect status URI
    Target run ${DOCUMENT_ROOT}/${request:uri}
    Target run ${controller}-${name} 
    Target write [-r] status "Hello World\r\n"
 */
static int targetDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *details;

    if (!maTokenize(state, value, "%S ?*", &name, &details)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    return setTarget(state, name, details);
}


/*
    Template routeName
 */
static int templateDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteTemplate(state->route, value);
    return 0;
}


/*
    ThreadStack bytes
 */
static int threadStackDirective(MaState *state, cchar *key, cchar *value)
{
    mprSetThreadStackSize(getint(value));
    return 0;
}


#if DEPRECATED || 1
/*
    TraceMethod on|off
 */
static int traceMethodDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (on) {
        httpAddRouteMethods(state->route, "TRACE");
    } else {
        httpRemoveRouteMethods(state->route, "TRACE");
    }
    return 0;
}
#endif


/*  
    TypesConfig path
 */
static int typesConfigDirective(MaState *state, cchar *key, cchar *value)
{
    char    *path;

    path = httpMakePath(state->route, state->configDir, value);
    if ((state->route->mimeTypes = mprCreateMimeTypes(path)) == 0) {
        mprError("Cannot open TypesConfig mime file %s", path);
        state->route->mimeTypes = mprCreateMimeTypes(NULL);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


/*
    UnloadModule name [timeout]
 */
static int unloadModuleDirective(MaState *state, cchar *key, cchar *value)
{
    MprModule   *module;
    HttpStage   *stage;
    char        *name, *timeout;

    timeout = MA_UNLOAD_TIMEOUT;
    if (!maTokenize(state, value, "%S ?S", &name, &timeout)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if ((module = mprLookupModule(name)) == 0) {
        mprError("Cannot find module stage %s", name);
        return MPR_ERR_BAD_SYNTAX;
    }
    if ((stage = httpLookupStage(state->http, module->name)) != 0 && stage->match) {
        mprError("Cannot unload module %s due to match routine", module->name);
        return MPR_ERR_BAD_SYNTAX;
    } else {
        module->timeout = httpGetTicks(timeout);
    }
    return 0;
}


/*
   Update param var value
   Update cmd commandLine
 */
static int updateDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *rest;

    if (!maTokenize(state, value, "%S %*", &name, &rest)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    return addUpdate(state, name, rest, 0);
}


/*
    UploadDir path
 */
static int uploadDirDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteUploadDir(state->route, httpMakePath(state->route, state->configDir, value));
    return 0;
}


/*
    UploadAutoDelete on|off
 */
static int uploadAutoDeleteDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteAutoDelete(state->route, on);
    return 0;
}


/*
    User name password abilities...
 */
static int userDirective(MaState *state, cchar *key, cchar *value)
{
    char    *name, *password, *abilities;

    if (!maTokenize(state, value, "%S %S ?*", &name, &password, &abilities)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (httpAddUser(state->auth, name, password, abilities) == 0) {
        mprError("Cannot add user %s", name);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


/*
    UserAccount username
 */
static int userAccountDirective(MaState *state, cchar *key, cchar *value)
{
    if (!smatch(value, "_unchanged_") && !mprGetDebugMode()) {
        maSetHttpUser(state->appweb, value);
    }
    return 0;
}


/*
    <VirtualHost ip[:port] ...>
 */
static int virtualHostDirective(MaState *state, cchar *key, cchar *value)
{
    state = maPushState(state);
    if (state->enabled) {
        /*
            Inherit the current default route configuration (only)
            Other routes are not inherited due to the reset routes below
         */
        state->route = httpCreateInheritedRoute(httpGetHostDefaultRoute(state->host));
        state->route->ssl = 0;
        state->auth = state->route->auth;
        state->host = httpCloneHost(state->host);
        httpResetRoutes(state->host);
        httpSetRouteHost(state->route, state->host);
        httpSetHostDefaultRoute(state->host, state->route);

        /* Set a default host and route name */
        httpSetHostName(state->host, stok(sclone(value), " \t,", NULL));
        httpSetRouteName(state->route, sfmt("default-%s", state->host->name));

        /* Save the endpoints until the close of the vhost. Do this so the vhost block can do the Listen */
        state->endpoints = sclone(value);
    }
    return 0;
}


/*
    </VirtualHost>
 */
static int closeVirtualHostDirective(MaState *state, cchar *key, cchar *value)
{
    HttpEndpoint    *endpoint;
    char            *address, *ip, *addresses, *tok;
    int             port;

    if (state->enabled && state->endpoints) {
        addresses = state->endpoints;
        while ((address = stok(addresses, " \t,", &tok)) != 0) {
            addresses = 0;
            mprParseSocketAddress(address, &ip, &port, NULL, -1);
            if ((endpoint = httpLookupEndpoint(state->http, ip, port)) == 0) {
                mprError("Cannot find listen directive for virtual host %s", address);
                return MPR_ERR_BAD_SYNTAX;
            } else {
                httpAddHostToEndpoint(endpoint, state->host);
            }
        }
    }
    closeDirective(state, key, value);
    return 0;
}


/*
    PreserveFrames [on|off]
 */
static int preserveFramesDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRoutePreserveFrames(state->route, on);
    return 0;
}


static int limitWebSocketsDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->webSocketsMax = getint(value);
    return 0;
}


static int limitWebSocketsMessageDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->webSocketsMessageSize = getint(value);
    return 0;
}


static int limitWebSocketsFrameDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->webSocketsFrameSize = getint(value);
    return 0;
}


static int limitWebSocketsPacketDirective(MaState *state, cchar *key, cchar *value)
{
    httpGraduateLimits(state->route, 0);
    state->route->limits->webSocketsPacketSize = getint(value);
    return 0;
}


static int webSocketsProtocolDirective(MaState *state, cchar *key, cchar *value)
{
    state->route->webSocketsProtocol = sclone(value);
    return 0;
}


static int webSocketsPingDirective(MaState *state, cchar *key, cchar *value)
{
    state->route->webSocketsPingPeriod = httpGetTicks(value);
    return 0;
}


PUBLIC bool maValidateServer(MaServer *server)
{
    MaAppweb        *appweb;
    Http            *http;
    HttpHost        *host, *defaultHost;
    HttpEndpoint    *endpoint;
    HttpRoute       *route;
    int             nextHost, nextEndpoint, nextRoute;

    appweb = server->appweb;
    http = appweb->http;
    defaultHost = server->defaultHost;
    assert(defaultHost);

    /*
        Add the default host to the unassigned endpoints
     */
    for (nextEndpoint = 0; (endpoint = mprGetNextItem(http->endpoints, &nextEndpoint)) != 0; ) {
        if (mprGetListLength(endpoint->hosts) == 0) {
            /* Add the defaultHost */
            httpAddHostToEndpoint(endpoint, defaultHost);
            if (!defaultHost->name) {
                httpSetHostName(defaultHost, sfmt("%s:%d", endpoint->ip, endpoint->port));
            }
        }
    }
    /*
        Ensure the host home directory is set and the file handler is defined
     */
    for (nextHost = 0; (host = mprGetNextItem(http->hosts, &nextHost)) != 0; ) {
        for (nextRoute = 0; (route = mprGetNextItem(host->routes, &nextRoute)) != 0; ) {
            if (!mprLookupKey(route->extensions, "")) {
                mprLog(3, "Route %s in host %s is missing a catch-all handler. "
                    "Adding: AddHandler fileHandler \"\"", route->name, host->name);
                httpAddRouteHandler(route, "fileHandler", "");
                httpAddRouteIndex(route, "index.html");
            }
        }
    }
    return 1;
}


PUBLIC int maParsePlatform(cchar *platform, cchar **os, cchar **arch, cchar **profile)
{
    char   *rest;

    if (platform == 0 || *platform == '\0') {
        return MPR_ERR_BAD_ARGS;
    }
    *os = stok(sclone(platform), "-", &rest);
    *arch = sclone(stok(NULL, "-", &rest));
    *profile = sclone(rest);
    if (*os == 0 || *arch == 0 || *profile == 0 || **os == '\0' || **arch == '\0' || **profile == '\0') {
        return MPR_ERR_BAD_ARGS;
    }
    return 0;
}


static bool conditionalDefinition(MaState *state, cchar *key)
{
    cchar   *arch, *os, *profile;
    int     result, not;

    result = 0;
    not = (*key == '!') ? 1 : 0;
    if (not) {
        for (++key; isspace((uchar) *key); key++) {}
    }
    maParsePlatform(state->appweb->platform, &os, &arch, &profile);

    if (scaselessmatch(key, arch)) {
        result = 1;

    } else if (scaselessmatch(key, os)) {
        result = 1;

    } else if (scaselessmatch(key, profile)) {
        result = 1;

    } else if (scaselessmatch(key, state->appweb->platform)) {
        result = 1;

#if ME_DEBUG
    } else if (scaselessmatch(key, "ME_DEBUG")) {
        result = ME_DEBUG;
#endif

    } else if (scaselessmatch(key, "dynamic")) {
        result = !state->appweb->staticLink;

    } else if (scaselessmatch(key, "static")) {
        result = state->appweb->staticLink;

    } else if (scaselessmatch(key, "IPv6")) {
        result = mprHasIPv6();

    } else if (state->appweb->skipModules) {
        /* ESP utility needs to be able to load mod_esp */
        if ((sstarts(mprGetAppName(), "esp") || sstarts(mprGetAppName(), "appesp")) && scaselessmatch(key, "ESP_MODULE")) {
            result = ME_COM_ESP;
        }

    } else {
        if (scaselessmatch(key, "CGI_MODULE")) {
            result = ME_COM_CGI;

        } else if (scaselessmatch(key, "DIR_MODULE")) {
            result = ME_COM_DIR;

        } else if (scaselessmatch(key, "EJS_MODULE")) {
            result = ME_COM_EJS;

        } else if (scaselessmatch(key, "ESP_MODULE")) {
            result = ME_COM_ESP;

        } else if (scaselessmatch(key, "PHP_MODULE")) {
            result = ME_COM_PHP;

        } else if (scaselessmatch(key, "SSL_MODULE")) {
            result = ME_COM_SSL;
        }
    }
    return (not) ? !result : result;
}


/*
    Tokenizes a line using %formats. Mandatory tokens can be specified with %. Optional tokens are specified with ?. 
    Supported tokens:
        %B - Boolean. Parses: on/off, true/false, yes/no.
        %N - Number. Parses numbers in base 10.
        %S - String. Removes quotes.
        %T - Template String. Removes quotes and expand ${PathVars}
        %P - Path string. Removes quotes and expands ${PathVars}. Resolved relative to route->home.
        %W - Parse words into a list
        %! - Optional negate. Set value to HTTP_ROUTE_NOT present, otherwise zero.
 */
PUBLIC bool maTokenize(MaState *state, cchar *line, cchar *fmt, ...)
{
    va_list     ap;

    va_start(ap, fmt);
    if (!httpTokenizev(state->route, line, fmt, ap)) {
        mprError("Bad \"%s\" directive at line %d in %s\nLine: %s %s\n", 
                state->key, state->lineNumber, state->filename, state->key, line);
        return 0;
    }
    va_end(ap);
    return 1;
}


static int addCondition(MaState *state, cchar *name, cchar *details, int flags)
{
    if (httpAddRouteCondition(state->route, name, details, flags) < 0) {
        mprError("Bad \"%s\" directive at line %d in %s\nLine: %s %s\n", 
            state->key, state->lineNumber, state->filename, state->key, details);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


static int addUpdate(MaState *state, cchar *name, cchar *details, int flags)
{
    if (httpAddRouteUpdate(state->route, name, details, flags) < 0) {
        mprError("Bad \"%s\" directive at line %d in %s\nLine: %s %s %s\n", 
                state->key, state->lineNumber, state->filename, state->key, name, details);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


static int setTarget(MaState *state, cchar *name, cchar *details)
{
    if (httpSetRouteTarget(state->route, name, details) < 0) {
        mprError("Bad \"%s\" directive at line %d in %s\nLine: %s %s %s\n", 
                state->key, state->lineNumber, state->filename, state->key, name, details);
        return MPR_ERR_BAD_SYNTAX;
    }
    return 0;
}


/*
    This is used to create the outermost state only
 */
static MaState *createState(MaServer *server, int flags)
{
    MaState     *state;
    HttpHost    *host;
    HttpRoute   *route;

    host = server->defaultHost;
    route = host->defaultRoute;

    if ((state = mprAllocObj(MaState, manageState)) == 0) {
        return 0;
    }
    state->top = state;
    state->current = state;
    state->server = server;
    state->appweb = server->appweb;
    state->http = server->http;
    state->host = host;
    state->route = route;
    state->enabled = 1;
    state->lineNumber = 0;
    state->auth = state->route->auth;
    state->flags = flags;
    server->state = state;
    return state;
}


PUBLIC MaState *maPushState(MaState *prev)
{
    MaState   *state;

    if ((state = mprAllocObj(MaState, manageState)) == 0) {
        return 0;
    }
    state->top = prev->top;
    state->prev = prev;
    state->flags = prev->flags;
    state->appweb = prev->appweb;
    state->http = prev->http;
    state->server = prev->server;
    state->host = prev->host;
    state->route = prev->route;
    state->lineNumber = prev->lineNumber;
    state->enabled = prev->enabled;
    state->filename = prev->filename;
    state->configDir = prev->configDir;
    state->file = prev->file;
    state->auth = state->route->auth;
    state->top->current = state;
    return state;
}


PUBLIC MaState *maPopState(MaState *state)
{
    if (state->prev == 0) {
        mprError("Too many closing blocks.\nAt line %d in %s\n\n", state->lineNumber, state->filename);
    }
    state->prev->lineNumber = state->lineNumber;
    state = state->prev;
    state->top->current = state;
    return state;
}


static void manageState(MaState *state, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(state->appweb);
        mprMark(state->http);
        mprMark(state->server);
        mprMark(state->host);
        mprMark(state->auth);
        mprMark(state->route);
        mprMark(state->file);
        mprMark(state->key);
        mprMark(state->configDir);
        mprMark(state->filename);
        mprMark(state->prev);
        mprMark(state->top);
        mprMark(state->current);
        mprMark(state->endpoints);
    }
}


static int configError(MaState *state, cchar *key)
{
    mprError("Error in directive \"%s\"\nAt line %d in %s\n\n", key, state->lineNumber, state->filename);
    return MPR_ERR_BAD_SYNTAX;
}


static int64 getnum(cchar *value)
{
    char    *junk;
    int64   num;

    value = stok(slower(value), " \t", &junk);
    if (sends(value, "kb") || sends(value, "k")) {
        num = stoi(value) * 1024;
    } else if (sends(value, "mb") || sends(value, "m")) {
        num = stoi(value) * 1024 * 1024;
    } else if (sends(value, "gb") || sends(value, "g")) {
        num = stoi(value) * 1024 * 1024 * 1024;
    } else if (sends(value, "byte") || sends(value, "bytes")) {
        num = stoi(value);
    } else {
        num = stoi(value);
    }
    if (num == 0) {
        num = MAXINT;
    }
    return num;
}


static int getint(cchar *value)
{
    int64   num;

    num = getnum(value);
    if (num >= MAXINT) {
        num = MAXINT;
    }
    return (int) num;
}


/*
    Get the directive and value details. Return key and *valuep.
 */
static char *getDirective(char *line, char **valuep)
{
    char    *key, *value;
    ssize   len;
    
    assert(line);
    assert(valuep);

    *valuep = 0;
    key = stok(line, " \t", &value);
    key = strim(key, " \t\r\n>", MPR_TRIM_END);
    if (value) {
        value = strim(value, " \t\r\n>", MPR_TRIM_END);
        /*
            Trim quotes if wrapping the entire value and no spaces. Preserve embedded quotes and leading/trailing "" etc.
         */
        len = slen(value);
        if (*value == '\"' && value[len - 1] == '"' && len > 2 && value[1] != '\"' && !strpbrk(value, " \t")) {
            /*
                Cannot strip quotes if multiple args are quoted, only if one single arg is quoted
             */
            if (schr(&value[1], '"') == &value[len - 1]) {
                value = snclone(&value[1], len - 2);
            }
        }
        *valuep = value;
    }
    return key;
}


PUBLIC char *maGetNextArg(char *s, char **tok)
{
    char    *etok;
    int     quoted;

    if (s == 0) {
        return 0;
    }
    for (; isspace((uchar) *s); s++) {}

    for (quoted = 0, etok = s; *etok; etok++) {
        if (*etok == '\'' || *etok == '"') {
            quoted = !quoted;
        } else if (isspace((uchar) *etok) && !quoted && (etok > s && etok[-1] != '\\')) {
            break;
        }
    }
    if (*s == '\'' || *s == '"') {
        s++;
        if (etok > s && (etok[-1] == '\'' || etok[-1] == '"')) {
            etok--;
        }
    }
    if (*etok == '\0') {
        etok = NULL;
    } else {
        *etok++ = '\0';
        for (; isspace((uchar) *etok); etok++) {}  
    }
    *tok = etok;
    return s;
}


#if DEPRECATED || 1
PUBLIC char *maGetNextToken(char *s, char **tok)
{
    return maGetNextArg(s, tok);
}
#endif


PUBLIC int maWriteAuthFile(HttpAuth *auth, char *path)
{
    MprFile         *file;
    MprKey          *kp, *ap;
    HttpRole        *role;
    HttpUser        *user;
    char            *tempFile;

    tempFile = mprGetTempPath(mprGetPathDir(path));
    if ((file = mprOpenFile(tempFile, O_CREAT | O_TRUNC | O_WRONLY | O_TEXT, 0444)) == 0) {
        mprError("Cannot open %s", tempFile);
        return MPR_ERR_CANT_OPEN;
    }
    mprWriteFileFmt(file, "#\n#   %s - Authorization data\n#\n\n", mprGetPathBase(path));

    for (ITERATE_KEY_DATA(auth->roles, kp, role)) {
        mprWriteFileFmt(file, "Role %s", kp->key);
        for (ITERATE_KEYS(role->abilities, ap)) {
            mprWriteFileFmt(file, " %s", ap->key);
        }
        mprPutFileChar(file, '\n');
    }
    mprPutFileChar(file, '\n');
    for (ITERATE_KEY_DATA(auth->userCache, kp, user)) {
        mprWriteFileFmt(file, "User %s %s %s", user->name, user->password, user->roles);
        mprPutFileChar(file, '\n');
    }
    mprCloseFile(file);
    unlink(path);
    if (rename(tempFile, path) < 0) {
        mprError("Cannot create new %s", path);
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC void maAddDirective(MaAppweb *appweb, cchar *directive, MaDirective proc)
{
    mprAddKey(appweb->directives, directive, proc);
}


PUBLIC int maParseInit(MaAppweb *appweb)
{
    if ((appweb->directives = mprCreateHash(-1, MPR_HASH_STATIC_VALUES | MPR_HASH_CASELESS | MPR_HASH_STABLE)) == 0) {
        return MPR_ERR_MEMORY;
    }
    maAddDirective(appweb, "AddLanguageSuffix", addLanguageSuffixDirective);
    maAddDirective(appweb, "AddLanguageDir", addLanguageDirDirective);
    maAddDirective(appweb, "AddFilter", addFilterDirective);
    maAddDirective(appweb, "AddInputFilter", addInputFilterDirective);
    maAddDirective(appweb, "AddHandler", addHandlerDirective);
    maAddDirective(appweb, "AddOutputFilter", addOutputFilterDirective);
    maAddDirective(appweb, "AddType", addTypeDirective);
    maAddDirective(appweb, "Alias", aliasDirective);
    maAddDirective(appweb, "Allow", allowDirective);
    maAddDirective(appweb, "AuthAutoLogin", authAutoLoginDirective);
    maAddDirective(appweb, "AuthDigestQop", authDigestQopDirective);
    maAddDirective(appweb, "AuthType", authTypeDirective);
    maAddDirective(appweb, "AuthRealm", authRealmDirective);
    maAddDirective(appweb, "AuthStore", authStoreDirective);
    maAddDirective(appweb, "Cache", cacheDirective);
    maAddDirective(appweb, "Chroot", chrootDirective);
    maAddDirective(appweb, "Condition", conditionDirective);
    maAddDirective(appweb, "CrossOrigin", crossOriginDirective);
    maAddDirective(appweb, "DefaultLanguage", defaultLanguageDirective);
    maAddDirective(appweb, "Defense", defenseDirective);
    maAddDirective(appweb, "Deny", denyDirective);
    maAddDirective(appweb, "DirectoryIndex", directoryIndexDirective);
    maAddDirective(appweb, "Documents", documentsDirective);
    maAddDirective(appweb, "<Directory", directoryDirective);
    maAddDirective(appweb, "</Directory", closeDirective);
    maAddDirective(appweb, "<else", elseDirective);
    maAddDirective(appweb, "ErrorDocument", errorDocumentDirective);
    maAddDirective(appweb, "ErrorLog", errorLogDirective);
    maAddDirective(appweb, "ExitTimeout", exitTimeoutDirective);
    maAddDirective(appweb, "GroupAccount", groupAccountDirective);
    maAddDirective(appweb, "Header", headerDirective);
    maAddDirective(appweb, "Home", homeDirective);
    maAddDirective(appweb, "<If", ifDirective);
    maAddDirective(appweb, "</If", closeDirective);
    maAddDirective(appweb, "IgnoreEncodingErrors", ignoreEncodingErrorsDirective);
    maAddDirective(appweb, "InactivityTimeout", inactivityTimeoutDirective);
    maAddDirective(appweb, "Include", includeDirective);
    maAddDirective(appweb, "LimitBuffer", limitBufferDirective);
    maAddDirective(appweb, "LimitCache", limitCacheDirective);
    maAddDirective(appweb, "LimitCacheItem", limitCacheItemDirective);
    maAddDirective(appweb, "LimitChunk", limitChunkDirective);
    maAddDirective(appweb, "LimitClients", limitClientsDirective);
    maAddDirective(appweb, "LimitConnections", limitConnectionsDirective);
    maAddDirective(appweb, "LimitFiles", limitFilesDirective);
    maAddDirective(appweb, "LimitKeepAlive", limitKeepAliveDirective);
    maAddDirective(appweb, "LimitMemory", limitMemoryDirective);
    maAddDirective(appweb, "LimitProcesses", limitProcessesDirective);
    maAddDirective(appweb, "LimitRequestsPerClient", limitRequestsPerClientDirective);
    maAddDirective(appweb, "LimitRequestBody", limitRequestBodyDirective);
    maAddDirective(appweb, "LimitRequestForm", limitRequestFormDirective);
    maAddDirective(appweb, "LimitRequestHeaderLines", limitRequestHeaderLinesDirective);
    maAddDirective(appweb, "LimitRequestHeader", limitRequestHeaderDirective);
    maAddDirective(appweb, "LimitResponseBody", limitResponseBodyDirective);
    maAddDirective(appweb, "LimitSessions", limitSessionDirective);
    maAddDirective(appweb, "LimitUri", limitUriDirective);
    maAddDirective(appweb, "LimitUpload", limitUploadDirective);
    maAddDirective(appweb, "LimitWebSockets", limitWebSocketsDirective);
    maAddDirective(appweb, "LimitWebSocketsMessage", limitWebSocketsMessageDirective);
    maAddDirective(appweb, "LimitWebSocketsFrame", limitWebSocketsFrameDirective);
    maAddDirective(appweb, "LimitWebSocketsPacket", limitWebSocketsPacketDirective);
    maAddDirective(appweb, "LimitWorkers", limitWorkersDirective);
    maAddDirective(appweb, "Listen", listenDirective);
    maAddDirective(appweb, "ListenSecure", listenSecureDirective);
    maAddDirective(appweb, "Log", logDirective);
    maAddDirective(appweb, "LogRoutes", logRoutesDirective);
    maAddDirective(appweb, "LoadModulePath", loadModulePathDirective);
    maAddDirective(appweb, "LoadModule", loadModuleDirective);
    maAddDirective(appweb, "MakeDir", makeDirDirective);
    maAddDirective(appweb, "Map", mapDirective);
    maAddDirective(appweb, "MemoryPolicy", memoryPolicyDirective);
    maAddDirective(appweb, "Methods", methodsDirective);
    maAddDirective(appweb, "MinWorkers", minWorkersDirective);
    maAddDirective(appweb, "Monitor", monitorDirective);
    maAddDirective(appweb, "Name", nameDirective);
    maAddDirective(appweb, "NameVirtualHost", nameVirtualHostDirective);
    maAddDirective(appweb, "Order", orderDirective);
    maAddDirective(appweb, "Param", paramDirective);
    maAddDirective(appweb, "Prefix", prefixDirective);
    maAddDirective(appweb, "PreserveFrames", preserveFramesDirective);
    maAddDirective(appweb, "Redirect", redirectDirective);
    maAddDirective(appweb, "RequestHeader", requestHeaderDirective);
    maAddDirective(appweb, "RequestParseTimeout", requestParseTimeoutDirective);
    maAddDirective(appweb, "RequestTimeout", requestTimeoutDirective);
    maAddDirective(appweb, "Require", requireDirective);
    maAddDirective(appweb, "<Reroute", rerouteDirective);
    maAddDirective(appweb, "</Reroute", closeDirective);
    maAddDirective(appweb, "Reset", resetDirective);
    maAddDirective(appweb, "Role", roleDirective);
    maAddDirective(appweb, "<Route", routeDirective);
    maAddDirective(appweb, "</Route", closeDirective);
    maAddDirective(appweb, "ServerName", serverNameDirective);
    maAddDirective(appweb, "SessionCookie", sessionCookieDirective);
    maAddDirective(appweb, "SessionTimeout", sessionTimeoutDirective);
    maAddDirective(appweb, "Set", setDirective);
    maAddDirective(appweb, "SetConnector", setConnectorDirective);
    maAddDirective(appweb, "SetHandler", setHandlerDirective);
    maAddDirective(appweb, "ShowErrors", showErrorsDirective);
    maAddDirective(appweb, "Source", sourceDirective);
    maAddDirective(appweb, "Stealth", stealthDirective);
    maAddDirective(appweb, "StreamInput", streamInputDirective);
    maAddDirective(appweb, "Target", targetDirective);
    maAddDirective(appweb, "Template", templateDirective);
    maAddDirective(appweb, "ThreadStack", threadStackDirective);
    maAddDirective(appweb, "TypesConfig", typesConfigDirective);
    maAddDirective(appweb, "Update", updateDirective);
    maAddDirective(appweb, "UnloadModule", unloadModuleDirective);
    maAddDirective(appweb, "UploadAutoDelete", uploadAutoDeleteDirective);
    maAddDirective(appweb, "UploadDir", uploadDirDirective);
    maAddDirective(appweb, "User", userDirective);
    maAddDirective(appweb, "UserAccount", userAccountDirective);
    maAddDirective(appweb, "<VirtualHost", virtualHostDirective);
    maAddDirective(appweb, "</VirtualHost", closeVirtualHostDirective);
    maAddDirective(appweb, "WebSocketsProtocol", webSocketsProtocolDirective);
    maAddDirective(appweb, "WebSocketsPing", webSocketsPingDirective);

#if !ME_ROM
    maAddDirective(appweb, "AccessLog", accessLogDirective);
#endif

#if DEPRECATED || 1
    /* Use AuthStore */
    maAddDirective(appweb, "AuthMethod", authStoreDirective);
    maAddDirective(appweb, "AuthGroupFile", authGroupFileDirective);
    maAddDirective(appweb, "AuthUserFile", authUserFileDirective);
    /* Use AuthRealm */
    maAddDirective(appweb, "AuthName", authRealmDirective);
    /* Use Map */
    maAddDirective(appweb, "Compress", compressDirective);
    /* Use Documents */
    maAddDirective(appweb, "DocumentRoot", documentsDirective);
    /* Use LimitConnections or LimitRequestsPerClient instead */
    maAddDirective(appweb, "LimitRequests", limitRequestsDirective);
    /* Use LimitBuffer */
    maAddDirective(appweb, "LimitStageBuffer", limitBufferDirective);
    /* Use LimitUri */
    maAddDirective(appweb, "LimitUrl", limitUriDirective);
    /* Use LimitKeepAlive */
    maAddDirective(appweb, "MaxKeepAliveRequests", limitKeepAliveDirective);
    /* Use Methods */
    maAddDirective(appweb, "PutMethod", putMethodDirective);
    maAddDirective(appweb, "ResetPipeline", resetPipelineDirective);
    /* Use MinWorkers */
    maAddDirective(appweb, "StartWorkers", minWorkersDirective);
    maAddDirective(appweb, "StartThreads", minWorkersDirective);
    /* Use requestTimeout */
    maAddDirective(appweb, "Timeout", requestTimeoutDirective);
    maAddDirective(appweb, "ThreadLimit", limitWorkersDirective);
    /* Use Methods */
    maAddDirective(appweb, "TraceMethod", traceMethodDirective);
    maAddDirective(appweb, "WorkerLimit", limitWorkersDirective);
    /* Use LimitRequestHeaderLines */
    maAddDirective(appweb, "LimitRequestFields", limitRequestHeaderLinesDirective);
    /* Use LimitRequestHeader */
    maAddDirective(appweb, "LimitRequestFieldSize", limitRequestHeaderDirective);
    /* Use InactivityTimeout */
    maAddDirective(appweb, "KeepAliveTimeout", inactivityTimeoutDirective);
    /* Use <Route> */
    maAddDirective(appweb, "<Location", routeDirective);
    maAddDirective(appweb, "</Location", closeDirective);
    /* Use Home */
    maAddDirective(appweb, "ServerRoot", homeDirective);
#endif

    return 0;
}


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
