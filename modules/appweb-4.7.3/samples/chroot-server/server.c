/**
    server.c  -- AppWeb main program

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.

    usage: appweb [options] 
    or:    appweb [options] [documents] [[ip][:port] ...]
            --chroot dir            # Change root to directory (unix only)
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
#include    "esp.h"

/********************************** Locals ************************************/
/*
    Global application object. Provides the top level roots of all data objects for the Garbage collector.
    Appweb uses garbage collection and all memory must have a reference to stop being reclaimed by the GC.
 */
typedef struct AppwebApp {
    Mpr         *mpr;
    MaAppweb    *appweb;
    MaServer    *server;
    char        *documents;
    char        *home;
    char        *configFile;
    char        *pathVar;
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
    static int  unixSecurityChecks(cchar *program, cchar *home);
#elif ME_WIN_LIKE
    static long msgProc(HWND hwnd, uint msg, uint wp, long lp);
#endif

/*********************************** Code *************************************/

MAIN(appweb, int argc, char **argv, char **envp)
{
    Mpr     *mpr;
    cchar   *argp, *jail;
    char    *logSpec;
    int     argind, status, verbose;

    jail = 0;
    verbose = 0;
    logSpec = 0;

    if ((mpr = mprCreate(argc, argv, MPR_USER_EVENTS_THREAD)) == NULL) {
        exit(1);
    }
    mprSetAppName(ME_NAME, ME_TITLE, ME_VERSION);

    /*
        Allocate the top level application object. ManageApp is the GC manager function and is called
        by the GC to mark references in the app object.
     */
    if ((app = mprAllocObj(AppwebApp, manageApp)) == NULL) {
        exit(2);
    }
    mprAddRoot(app);
    mprAddStandardSignals();

    app->mpr = mpr;
    app->configFile = sclone("appweb.conf");
    app->home = mprGetCurrentPath();
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
            app->home = mprGetAbsPath(argv[++argind]);
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

        } else if (smatch(argp, "--verbose") || smatch(argp, "-v")) {
            verbose++;

        } else if (smatch(argp, "--version") || smatch(argp, "-V")) {
            mprPrintf("%s\n", ME_VERSION);
            exit(0);

        } else {
            if (!smatch(argp, "?")) {
                mprError("Unknown switch \"%s\"", argp);
            }
            usageError();
            exit(5);
        }
    }
    if (logSpec) {
        mprStartLogging(logSpec, 1);
        mprSetCmdlineLogging(1);

    } else if (verbose) {
        mprStartLogging(sfmt("stderr:%d", verbose + 1), 1);
        mprSetCmdlineLogging(1);
    }
    /*
        Start the multithreaded portable runtime (MPR)
     */
    if (mprStart() < 0) {
        mprError("Cannot start MPR for %s", mprGetAppName());
        mprDestroy();
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (checkEnvironment(argv[0]) < 0) {
        exit(6);
    }
    if (findAppwebConf() < 0) {
        exit(7);
    }

    /*
        Open the sockets to listen on
     */
    if (createEndpoints(argc - argind, &argv[argind]) < 0) {
        return MPR_ERR_CANT_INITIALIZE;
    }
    if (jail && changeRoot(jail) < 0) {
        exit(8);
    }
    /*
        Start HTTP services
     */
    if (maStartAppweb(app->appweb) < 0) {
        mprError("Cannot start HTTP service, exiting.");
        exit(9);
    }
    /*
        Service I/O events until instructed to exit
     */
    while (!mprIsStopping()) {
        mprServiceEvents(-1, 0);
    }
    status = mprGetExitStatus();
    mprLog(1, "Stopping Appweb ...");
    maStopAppweb(app->appweb);
    mprDestroy();
    return status;
}


static void manageApp(AppwebApp *app, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(app->appweb);
        mprMark(app->server);
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
        mprLog(MPR_CONFIG, "Chroot to: \"%s\"", jail);
    }
#endif
    return 0;
}


/*
    If doing a static build, must now reference required modules to force the linker to include them.
    Don't actually call init routines here. They will be called via maConfigureServer.
 */
static void loadStaticModules()
{
#if ME_STATIC
#if ME_COM_CGI
    mprNop(maCgiHandlerInit);
#endif
#if ME_COM_ESP
    mprNop(maEspHandlerInit);
#endif
#if ME_COM_PHP
    mprNop(maPhpHandlerInit);
#endif
#if ME_SSL
    mprNop(maSslModuleInit);
#endif
#endif /* ME_STATIC */
}


/*
    Create the listening endoints
 */
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

    if (argc > argind) {
        app->documents = sclone(argv[argind++]);
        mprLog(2, "Documents %s", app->documents);
    }
    if (argind == argc) {
        if (maParseConfig(app->server, app->configFile, 0) < 0) {
            return MPR_ERR_CANT_CREATE;
        }
    } else {
        while (argind < argc) {
            endpoint = argv[argind++];
            mprParseSocketAddress(endpoint, &ip, &port, &secure, 80);
            if (maConfigureServer(app->server, NULL, app->home, app->documents, ip, port) < 0) {
                return MPR_ERR_CANT_CREATE;
            }
        }
    }
    return 0;
}


/*
    Find the appweb.conf file
 */
static int findAppwebConf()
{
    cchar   *userPath;

    userPath = app->configFile;
    if (app->configFile == 0) {
        app->configFile = mprJoinPathExt(mprGetAppName(), ".conf");
    }
    if (!mprPathExists(app->configFile, R_OK)) {
        if (!userPath) {
            app->configFile = mprJoinPath(app->home, "appweb.conf");
            if (!mprPathExists(app->configFile, R_OK)) {
                app->configFile = mprJoinPath(mprGetAppDir(), sfmt("%s.conf", mprGetAppName()));
            }
        }
        if (!mprPathExists(app->configFile, R_OK)) {
            mprError("Cannot open config file \"%s\"", app->configFile);
            return MPR_ERR_CANT_OPEN;
        }
    }
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
/*
    Security checks. Make sure we are staring with a safe environment
 */
static int unixSecurityChecks(cchar *program, cchar *home)
{
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
    return 0;
}
#endif /* ME_UNIX_LIKE */


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
