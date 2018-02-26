/*
    client.c - Simple client using the GET method to retrieve a web page.
  
    This sample demonstrates retrieving content using the HTTP GET method.
  
    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
/***************************** Includes *******************************/

#include    "appweb.h"

/********************************* Code *******************************/

MAIN(simpleClient, int argc, char **argv, char **envp)
{
    int     code;
    char    *response, *err;

    /*
        This will create the Mpr and Http services. Alternatively, if you have existing
        Mpr and Http services, use httpRequest() directly (see the typical-client sample).
     */
    code = maRunWebClient("GET", "http://www.embedthis.com/index.html", NULL, &response, &err);
    if (code != 200) {
        mprPrintf("Server error code %d, %s\n", code, err);
        return 255;
    }
    printf("Server responded with: %s\n", response);
    /*
        Alternatively if you already have the MPR initialized, you can use the httpRequest API:

        conn = httpRequest(method, uri, data, err);
     */
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
