/*
    simple.c - Create a simple AppWeb request handler
  
    This sample demonstrates creating a request handler to process requests.
  
    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */
 
/******************************* Includes *****************************/

#include    "appweb.h"

/********************************* Code *******************************/
/*
    Run the handler. This is called when all input data has been received.
 */
static void readySimple(HttpQueue *q)
{
    HttpConn    *conn;

    conn = q->conn;
    httpSetHeader(conn, "Custom-Date", conn->http->currentDate);
    httpSetStatus(conn, 200);

    /*
        Generate some dynamic data. If you generate a lot, this will buffer up to a configured maximum. 
        If that limit is exceeded, the packet will be sent downstream and the response headers will be created.
     */
    httpWrite(q, "Hello World\n");
    /*
        Call finalize when the response to the client is complete. Call httpFlushOutput if the response is 
        incomplete and you wish to immediately send any buffered output.
    */
    httpFinalize(conn);
}



static void incomingSimple(HttpQueue *q, HttpPacket *packet)
{
    /*
        Do something with the incoming data packet
     */
    if (packet->content) {
        printf("Data in packet is %s\n", mprGetBufStart(packet->content));
    }
}


/*
    Module load initialization. This is called when the module is first loaded.
 */
int maSimpleHandlerInit(Http *http, MprModule *module)
{
    HttpStage   *stage;

    if ((stage = httpCreateHandler(http, "simpleHandler", module)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    stage->ready = readySimple;
    stage->incoming = incomingSimple;
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
