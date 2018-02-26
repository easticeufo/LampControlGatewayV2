/**
    appweb.c  -- AppWeb main program

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.

    usage: appweb [options] 
    or:    appweb [options] [documents] [[ip][:port] ...]
            --chroot dir            # Change root to dir (unix only)
            --config configFile     # Use given config file instead 
            --debugger              # Disable timeouts to make debugging easier
            --home path             # Set the home working directory
            --log logFile:level     # Log to file file at verbosity level
            --name uniqueName       # Name for this instance
            --version               # Output version information
            -v                      # Same as --log stderr:2
 */

/********************************* Includes ***********************************/

#include    "appweb.h"

/********************************** Locals ************************************/
/*
    Global application object. Provides the top level roots of all data objects for the GC.
 */
typedef struct AppwebApp {
    Mpr         *mpr;
    MaAppweb    *appweb;
    MaServer    *server;
    MprSignal   *traceToggle;
    MprSignal   *statusCheck;
    char        *documents;
    char        *home;
    char        *configFile;
    char        *pathVar;
    int         workers;
} AppwebApp;

static AppwebApp *app;

/***************************** Forward Declarations ***************************/

static int changeRoot(cchar *jail);
static int checkEnvironment(cchar *program);
static int findAppwebConf();
static void manageApp(AppwebApp *app, int flags);
static int createEndpoints(int argc, char **argv);
static void usageError();

#if ME_UNIX_LIKE
    #if defined(SIGINFO) || defined(SIGPWR) || defined(SIGRTMIN)
        static void statusCheck(void *ignored, MprSignal *sp);
        static void addSignals();
    #endif
    static void traceHandler(void *ignored, MprSignal *sp);
    static int  unixSecurityChecks(cchar *program, cchar *home);
#elif ME_WIN_LIKE
    static int writePort(MaServer *server);
    static long msgProc(HWND hwnd, uint msg, uint wp, long lp);
#endif

/*
    If customize.h does not define, set reasonable defaults.
 */
#ifndef ME_SERVER_ROOT
    #define ME_SERVER_ROOT mprGetCurrentPath()
#endif
#ifndef ME_CONFIG_FILE
    #define ME_CONFIG_FILE NULL
#endif

/*********************************** Code *************************************/

MAIN(appweb, int argc, char **argv, char **envp)
{
    Mpr     *mpr;
    cchar   *argp, *jail;
    char    *logSpec;
    int     argind, level;

    jail = 0;
    level = 0;
    logSpec = 0;

    if ((mpr = mprCreate(argc, argv, MPR_USER_EVENTS_THREAD)) == NULL) {
        exit(1);
    }
    if ((app = mprAllocObj(AppwebApp, manageApp)) == NULL) {
        exit(2);
    }
    mprAddRoot(app);
    mprAddStandardSignals();

#if ME_ROM
    extern MprRomInode romFiles[];
    mprSetRomFileSystem(romFiles);
#endif

    app->mpr = mpr;
    app->workers = -1;
    app->home = sclone(ME_SERVER_ROOT);
    app->documents = app->home;
    argc = mpr->argc;
    argv = (char**) mpr->argv;

    for (argind = 1; argind < argc; argind++) {
        argp = argv[argind];
        if (*argp != '-') {
            break;
        }
        if (smatch(argp, "--config") || smatch(argp, "--conf")) {
            if (argind >= argc) {
                usageError();
            }
            app->configFile = sclone(argv[++argind]);

#if ME_UNIX_LIKE
        } else if (smatch(argp, "--chroot")) {
            if (argind >= argc) {
                usageError();
            }
            jail = mprGetAbsPath(argv[++argind]);
#endif

        } else if (smatch(argp, "--debugger") || smatch(argp, "-D")) {
            mprSetDebugMode(1);

        } else if (smatch(argp, "--exe")) {
            if (argind >= argc) {
                usageError();
            }
            mpr->argv[0] = mprGetAbsPath(argv[++argind]);
            mprSetAppPath(mpr->argv[0]);
            mprSetModuleSearchPath(NULL);

        } else if (smatch(argp, "--home")) {
            if (argind >= argc) {
                usageError();
            }
            app->home = sclone(argv[++argind]);
            if (chdir(app->home) < 0) {
                mprError("%s: Cannot change directory to %s", mprGetAppName(), app->home);
                exit(4);
            }

        } else if (smatch(argp, "--log") || smatch(argp, "-l")) {
            if (argind >= argc) {
                usageError();
            }
            logSpec = argv[++argind];

        } else if (smatch(argp, "--name") || smatch(argp, "-n")) {
            if (argind >= argc) {
                usageError();
            }
            mprSetAppName(argv[++argind], 0, 0);

        } else if (smatch(argp, "--threads")) {
            if (argind >= argc) {
                usageError();
            }
            app->workers = atoi(argv[++argind]);

        } else if (smatch(argp, "--verbose") || smatch(argp, "-v")) {
            level = 2;

        } else if (smatch(argp, "--version") || smatch(argp, "-V")) {
            mprPrintf("%s\n", ME_VERSION);
            exit(0);

        } else if (*argp == '-' && isdigit((uchar) argp[1])) {
            level = (int) stoi(&argp[1]);

        } else if (smatch(argp, "-?") || scontains(argp, "-help")) {
            usageError();
            exit(5);

        } else if (!smatch(argp, "?")) {
            mprError("Unknown switch \"%s\"", argp);
            usageError();
            exit(5);
        }
    }
    app->home = mprGetAbsPath(app->home);

    if (logSpec) {
        mprStartLogging(logSpec, 1);
        mprSetCmdlineLogging(1);
    } else if (level) {
        mprStartLogging(sfmt("stderr:%d", level), 1);
        mprSetCmdlineLogging(1);
    }
    if (mprStart() < 0) {
        mprError("Cannot start MPR for %s", mprGetAppName());
        mprDestroy();
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (checkEnvironment(argv[0]) < 0) {
        exit(6);
    }
    if (argc == argind && !app->configFile) {
        if (findAppwebConf() < 0) {
            exit(7);
        }
    }
    if (jail && changeRoot(jail) < 0) {
        exit(8);
    }
    if (createEndpoints(argc - argind, &argv[argind]) < 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (maStartAppweb(app->appweb) < 0) {
        mprError("Cannot start HTTP service, exiting.");
        exit(9);
    }
    mprServiceEvents(-1, 0);

    mprLog(1, "Stopping Appweb ...");
    mprDestroy();
    return mprGetExitStatus();
}


static void manageApp(AppwebApp *app, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(app->appweb);
        mprMark(app->server);
        mprMark(app->traceToggle);
        mprMark(app->statusCheck);
        mprMark(app->documents);
        mprMark(app->configFile);
        mprMark(app->pathVar);
        mprMark(app->home);
    }
}


/*
    Move into a chroot jail
 */
static int changeRoot(cchar *jail)
{
#if ME_UNIX_LIKE
    if (chdir(app->home) < 0) {
        mprError("%s: Cannot change directory to %s", mprGetAppName(), app->home);
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (chroot(jail) < 0) {
        if (errno == EPERM) {
            mprError("%s: Must be super user to use the --chroot option", mprGetAppName());
        } else {
            mprError("%s: Cannot change change root directory to %s, errno %d", mprGetAppName(), jail, errno);
        }
        return MPR_ERR_CANT_INITIALIZE;
    } else {
        mprLog(MPR_INFO, "Chroot to: \"%s\"", jail);
    }
#endif
    return 0;
}


static void loadStaticModules()
{
#if ME_STATIC
    /*
        If doing a static build, must now reference required modules to force the linker to include them.
        On linux we cannot lookup symbols, so we must invoke explicitly here.
        Users: add your modules here if you are doing a static link.
     */
    Http        *http = MPR->httpService;
#if ME_COM_CGI
    maCgiHandlerInit(http, mprCreateModule("cgiHandler", 0, 0, http));
#endif
#if ME_COM_ESP
    maEspHandlerInit(http, mprCreateModule("espHandler", 0, 0, http));
#endif
#if ME_COM_EJS
    maEspHandlerInit(http, mprCreateModule("ejsHandler", 0, 0, http));
#endif
#if ME_COM_PHP
    maPhpHandlerInit(http, mprCreateModule("phpHandler", 0, 0, http));
#endif
#if ME_COM_SSL
    maSslModuleInit(http, mprCreateModule("sslModule", 0, 0, http));
#endif
#endif /* ME_STATIC */
}


static int createEndpoints(int argc, char **argv)
{
    cchar   *endpoint;
    char    *ip;
    int     argind, port, secure;

    ip = 0;
    port = -1;
    endpoint = 0;
    argind = 0;

    if ((app->appweb = maCreateAppweb()) == 0) {
        mprError("Cannot create HTTP service for %s", mprGetAppName());
        return MPR_ERR_CANT_CREATE;
    }
    if ((app->server = maCreateServer(app->appweb, "default")) == 0) {
        mprError("Cannot create HTTP server for %s", mprGetAppName());
        return MPR_ERR_CANT_CREATE;
    }
    loadStaticModules();
    mprGC(MPR_GC_FORCE | MPR_GC_COMPLETE);

    if (argind == argc) {
        if (maParseConfig(app->server, app->configFile, 0) < 0) {
            return MPR_ERR_CANT_CREATE;
        }
    } else {
        app->documents = sclone(argv[argind++]);
        mprLog(2, "Documents %s", app->documents);
        if (argind == argc) {
            if (maConfigureServer(app->server, NULL, app->home, app->documents, NULL, ME_HTTP_PORT) < 0) {
                return MPR_ERR_CANT_CREATE;
            }
        } else while (argind < argc) {
            endpoint = argv[argind++];
            mprParseSocketAddress(endpoint, &ip, &port, &secure, 80);
            if (maConfigureServer(app->server, NULL, app->home, app->documents, ip, port) < 0) {
                return MPR_ERR_CANT_CREATE;
            }
        }
        httpLogRoutes(app->server->defaultHost, 0);
    }
    if (app->workers >= 0) {
        mprSetMaxWorkers(app->workers);
    }
    /*
        Call any ESP initializers from slink.c
     */
    appwebStaticInitialize();
    
#if ME_WIN_LIKE
    writePort(app->server);
#elif ME_UNIX_LIKE
    addSignals();
#endif
    mprGC(MPR_GC_FORCE | MPR_GC_COMPLETE);
    return 0;
}


/*
    If ME_CONFIG_FILE is defined, use that as the base name for appweb.conf. Otherwise, use
    "programName.conf". Search order is:

        "."/BASE
        ME_SERVER_ROOT/BASE
        EXE/../BASE
        EXE/../appweb.conf
 */
static int findAppwebConf()
{
    char    *base, *filename;

    if (ME_CONFIG_FILE) {
        base = sclone(ME_CONFIG_FILE);
    } else {
        base = mprJoinPathExt(mprGetAppName(), ".conf");
    }
#if !ME_ROM
    filename = base;
    if (!mprPathExists(filename, R_OK)) {
        filename = mprJoinPath(app->home, base);
        if (!mprPathExists(filename, R_OK)) {
            filename = mprJoinPath(mprGetPathParent(mprGetAppDir()), base);
            if (!mprPathExists(filename, R_OK)) {
                filename = mprJoinPath(mprGetPathParent(mprGetAppDir()), "appweb.conf");
                if (!mprPathExists(filename, R_OK)) {
                    mprError("Cannot find config file %s", base);
                    return MPR_ERR_CANT_OPEN;
                }
            }
        }
    }
#endif
    app->configFile = filename;
    return 0;
}


static void usageError(Mpr *mpr)
{
    cchar   *name;

    name = mprGetAppName();

    mprEprintf("\n%s Usage:\n\n"
        "  %s [options]\n"
        "  %s [options] documents ip[:port] ...\n\n"
        "  Without [documents ip:port], %s will read the appweb.conf configuration file.\n\n"
        "  Options:\n"
        "    --config configFile    # Use named config file instead appweb.conf\n"
        "    --chroot directory     # Change root directory to run more securely (Unix)\n"
        "    --debugger             # Disable timeouts to make debugging easier\n"
        "    --exe path             # Set path to Appweb executable on Vxworks\n"
        "    --home directory       # Change to directory to run\n"
        "    --log logFile:level    # Log to file file at verbosity level\n"
        "    --name uniqueName      # Unique name for this instance\n"
        "    --verbose              # Same as --log stderr:2\n"
        "    --version              # Output version information\n\n",
        mprGetAppTitle(), name, name, name);
    exit(10);
}


/*
    Security checks. Make sure we are staring with a safe environment
 */
static int checkEnvironment(cchar *program)
{
#if ME_UNIX_LIKE
    char   *home;
    home = mprGetCurrentPath();
    if (unixSecurityChecks(program, home) < 0) {
        return -1;
    }
    app->pathVar = sjoin("PATH=", getenv("PATH"), ":", mprGetAppDir(), NULL);
    putenv(app->pathVar);
#endif
    return 0;
}


#if ME_UNIX_LIKE
static void addSignals()
{
    app->traceToggle = mprAddSignalHandler(SIGUSR2, traceHandler, 0, 0, MPR_SIGNAL_AFTER);

    /*
        Signal to dump memory stats. Must configure with ./configure --set memoryCheck=true
     */
#if defined(SIGINFO)
    app->statusCheck = mprAddSignalHandler(SIGINFO, statusCheck, 0, 0, MPR_SIGNAL_AFTER);
#elif defined(SIGPWR)
    app->statusCheck = mprAddSignalHandler(SIGPWR, statusCheck, 0, 0, MPR_SIGNAL_AFTER);
#elif defined(SIGRTMIN)
    app->statusCheck = mprAddSignalHandler(SIGRTMIN, statusCheck, 0, 0, MPR_SIGNAL_AFTER);
#endif
}


/*
    SIGUSR2 will toggle trace from level 2 to 4
 */
static void traceHandler(void *ignored, MprSignal *sp)
{
    int     level;

    level = mprGetLogLevel() > 2 ? 2 : 4;
    mprLog(0, "Change log level to %d", level);
    mprSetLogLevel(level);
}


/*
    SIGINFO|SIGPWR|SIGRTMIN will dump memory stats
    For detailed memory stats, use: ./configure --set memoryCheck=true
 */
static void statusCheck(void *ignored, MprSignal *sp)
{
    mprRawLog(0, "%s", httpStatsReport(0));
    if (MPR->heap->track) {
        mprPrintMem("Memory Report", MPR_MEM_DETAIL);
    } else {
        mprPrintMem("Memory Report", 0);
    }
}


/*
    Security checks. Make sure we are staring with a safe environment
 */
static int unixSecurityChecks(cchar *program, cchar *home)
{
#if !ME_ROM
    struct stat     sbuf;

    if (((stat(home, &sbuf)) != 0) || !(S_ISDIR(sbuf.st_mode))) {
        mprError("Cannot access directory: %s", home);
        return MPR_ERR_BAD_STATE;
    }
    if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
        mprError("Security risk, directory %s is writeable by others", home);
    }

    /*
        Should always convert the program name into a fully qualified path. Otherwise this fails.
     */
    if (*program == '/') {
        if ((stat(program, &sbuf)) != 0) {
            mprError("Cannot access program: %s", program);
            return MPR_ERR_BAD_STATE;
        }
        if ((sbuf.st_mode & S_IWOTH) || (sbuf.st_mode & S_IWGRP)) {
            mprError("Security risk, Program %s is writeable by others", program);
        }
        if (sbuf.st_mode & S_ISUID) {
            mprError("Security risk, %s is setuid", program);
        }
        if (sbuf.st_mode & S_ISGID) {
            mprError("Security risk, %s is setgid", program);
        }
    }
#endif /* !ME_ROM */
    return 0;
}
#endif /* ME_UNIX_LIKE */


#if ME_WIN_LIKE
/*
    Write the port so the monitor can manage
 */ 
static int writePort(MaServer *server)
{
    HttpEndpoint    *endpoint;
    char            numBuf[16], *path;
    int             fd, len;

    if ((endpoint = mprGetFirstItem(server->http->endpoints)) == 0) {
        mprError("No configured endpoints");
        return MPR_ERR_CANT_ACCESS;
    }
    //  FUTURE - should really go to a ME_LOG_DIR (then fix uninstall.sh)
    path = mprJoinPath(mprGetAppDir(), "../.port.log");
    if ((fd = open(path, O_CREAT | O_WRONLY | O_TRUNC, 0666)) < 0) {
        mprError("Could not create port file %s", path);
        return MPR_ERR_CANT_CREATE;
    }
    fmt(numBuf, sizeof(numBuf), "%d", endpoint->port);

    len = (int) strlen(numBuf);
    numBuf[len++] = '\n';
    if (write(fd, numBuf, len) != len) {
        mprError("Write to file %s failed", path);
        return MPR_ERR_CANT_WRITE;
    }
    close(fd);
    return 0;
}
#endif /* ME_WIN_LIKE */


#if VXWORKS
/*
    VxWorks link resolution
 */
PUBLIC int _cleanup() {
    return 0;
}
PUBLIC int _exit() {
    return 0;
}

/*
    Create a routine to pull in the GCC support routines for double and int64 manipulations. Do this
    incase any modules reference these routines. Without this, the modules have to reference them. Which leads to
    multiple defines if two modules include them.
 */
double  __dummy_appweb_floating_point_resolution(double a, double b, int64 c, int64 d, uint64 e, uint64 f) {
    /*
        Code to pull in moddi3, udivdi3, umoddi3, etc .
     */
    a = a / b; a = a * b; c = c / d; c = c % d; e = e / f; e = e % f;
    c = (int64) a;
    d = (uint64) a;
    a = (double) c;
    a = (double) e;
    if (a == b) {
        return a;
    } return b;
}

#endif

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
