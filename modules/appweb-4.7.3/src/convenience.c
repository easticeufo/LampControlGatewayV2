/*
    convenience.c -- High level convenience API
    This module provides simple, high-level APIs for creating servers.
    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "appweb.h"

/************************************ Code ************************************/
//  TODO: REFACTOR with an inner function
/*  
    Create a web server described by a config file. 
 */
PUBLIC int maRunWebServer(cchar *configFile)
{
    Mpr         *mpr;
    MaAppweb    *appweb;
    MaServer    *server;
    int         rc;

    rc = MPR_ERR_CANT_CREATE;
    if ((mpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD)) == 0) {
        mprError("Cannot create the web server runtime");
    } else {
        if (mprStart() < 0) {
            mprError("Cannot start the web server runtime");
        } else {
            if ((appweb = maCreateAppweb(mpr)) == 0) {
                mprError("Cannot create appweb object");
            } else {
                mprAddRoot(appweb);
                if ((server = maCreateServer(appweb, 0)) == 0) {
                    mprError("Cannot create the web server");
                } else {
                    if (maParseConfig(server, configFile, 0) < 0) {
                        mprError("Cannot parse the config file %s", configFile);
                    } else {
                        if (maStartServer(server) < 0) {
                            mprError("Cannot start the web server");
                        } else {
                            mprServiceEvents(-1, 0);
                            rc = 0;
                        }
                        maStopServer(server);
                    }
                }
                mprRemoveRoot(appweb);
            }
        }
    }
    mprDestroy();
    return rc;
}


//  TODO: REFACTOR with an inner function
/*
    Run a web server not based on a config file.
 */
PUBLIC int maRunSimpleWebServer(cchar *ip, int port, cchar *home, cchar *documents)
{
    Mpr         *mpr;
    MaServer    *server;
    MaAppweb    *appweb;
    int         rc;

    /*  
        Initialize and start the portable runtime services.
     */
    rc = MPR_ERR_CANT_CREATE;
    if ((mpr = mprCreate(0, NULL, MPR_USER_EVENTS_THREAD)) == 0) {
        mprError("Cannot create the web server runtime");
    } else {
        if (mprStart(mpr) < 0) {
            mprError("Cannot start the web server runtime");
        } else {
            if ((appweb = maCreateAppweb(mpr)) == 0) {
                mprError("Cannot create the web server http services");
            } else {
                mprAddRoot(appweb);
                if ((server = maCreateServer(appweb, 0)) == 0) {
                    mprError("Cannot create the web server");
                } else {
                    if (maConfigureServer(server, 0, home, documents, ip, port) < 0) {
                        mprError("Cannot create the web server");
                    } else {
                        if (maStartServer(server) < 0) {
                            mprError("Cannot start the web server");
                        } else {
                            mprServiceEvents(-1, 0);
                            rc = 0;
                        }
                        maStopServer(server);
                    }
                }
                mprRemoveRoot(appweb);
            }
        }
        mprDestroy();
    }
    return rc;
}


/*
    This will restart the default server on a new IP:PORT. It will stop listening on the default endpoint on 
    the default server, optionally modify the IP:PORT and resume listening. NOTE: running requests will be
    unaffected.  WARNING: this is demonstration code and has no error checking.
 */
PUBLIC void maRestartServer(cchar *ip, int port)
{
    MaAppweb        *appweb;
    MaServer        *server;
    HttpEndpoint    *endpoint;

    appweb = MPR->appwebService;
    server = mprGetFirstItem(appweb->servers);
    lock(appweb->servers);
    endpoint = mprGetFirstItem(server->endpoints);
    httpStopEndpoint(endpoint);

    /* 
        Alternatively, iterate over all endpoints by

        Http *http = MPR->httpService;
        int  next;
        for (ITERATE_ITEMS(http->endpoints, endpoint, next)) {
            ...
        }
     */

    if (port) {
        endpoint->port = port;
    }
    if (ip) {
        endpoint->ip = sclone(ip);
    }
    httpStartEndpoint(endpoint);
    unlock(appweb->servers);
}


/*  
    Run the web client to retrieve a URI
    This will create the MPR and Http service on demand. As such, it is not the most
    efficient way to run a web request.
    @return HTTP status code or negative MPR error code. Returns a malloc string in response.
 */
PUBLIC int maRunWebClient(cchar *method, cchar *uri, cchar *data, char **response, char **err)
{
    Mpr         *mpr;
    HttpConn    *conn;
    int         status;

    if (err) {
        *err = 0;
    }
    if (response) {
        *response = 0;
    }
    if ((mpr = mprCreate(0, NULL, 0)) == 0) {
        mprError("Cannot create the MPR runtime");
        return MPR_ERR_CANT_CREATE;
    }
    if (mprStart() < 0) {
        mprError("Cannot start the web server runtime");
        return MPR_ERR_CANT_INITIALIZE;
    }
    httpCreate(HTTP_CLIENT_SIDE);
    conn = httpRequest(method, uri, data, err);
    status = httpGetStatus(conn);
    if (response) {
        *response = httpReadString(conn);
    }
    mprDestroy();
    return status;
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
