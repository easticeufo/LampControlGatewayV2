/*
    fileHandler.c -- Static file content handler

    This handler manages static file based content such as HTML, GIF /or JPEG pages. It supports all methods including:
    GET, PUT, DELETE, OPTIONS and TRACE. It is event based and does not use worker threads.

    The fileHandler also manages requests for directories that require redirection to an index or responding with
    a directory listing. 

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************* Includes ***********************************/

#include    "appweb.h"

/***************************** Forward Declarations ***************************/

static void handleDeleteRequest(HttpQueue *q);
static void handlePutRequest(HttpQueue *q);
static int manageDir(HttpConn *conn);
static ssize readFileData(HttpQueue *q, HttpPacket *packet, MprOff pos, ssize size);

/*********************************** Code *************************************/
/*
    Rewrite the request for directories, indexes and compressed content. 
 */
static int rewriteFileHandler(HttpConn *conn)
{
    HttpRx      *rx;
    HttpTx      *tx;
    MprPath     *info;

    rx = conn->rx;
    tx = conn->tx;
    info = &tx->fileInfo;

    httpMapFile(conn);
    assert(info->checked);

    if (rx->flags & (HTTP_DELETE | HTTP_PUT)) {
        return HTTP_ROUTE_OK;
    }
    if (info->isDir) {
        return manageDir(conn);
    }
    if (rx->flags & (HTTP_GET | HTTP_HEAD | HTTP_POST) && info->valid && tx->length < 0) {
        /*
            The sendFile connector is optimized on some platforms to use the sendfile() system call.
            Set the entity length for the sendFile connector to utilize.
         */
        httpSetEntityLength(conn, tx->fileInfo.size);
    }
    return HTTP_ROUTE_OK;
}



static void openFileHandler(HttpQueue *q)
{
    HttpRx      *rx;
    HttpTx      *tx;
    HttpConn    *conn;
    MprPath     *info;
    char        *date, dbuf[16];
    MprHash     *dateCache;

    conn = q->conn;
    tx = conn->tx;
    rx = conn->rx;
    info = &tx->fileInfo;

    if (conn->error) {
        return;
    }
    if (rx->flags & (HTTP_GET | HTTP_HEAD | HTTP_POST)) {
        if (!(info->valid || info->isDir)) {
            if (rx->referrer) {
                mprLog(2, "fileHandler: Cannot find filename %s from referrer %s", tx->filename, rx->referrer);
            } else {
                mprLog(2, "fileHandler: Cannot find filename %s", tx->filename);
            }
            httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot find document");
            return;
        } 
        if (!tx->etag) {
            /* Set the etag for caching in the client */
            tx->etag = sfmt("\"%llx-%llx-%llx\"", (int64) info->inode, (int64) info->size, (int64) info->mtime);
        }
        if (info->mtime) {
            dateCache = conn->http->dateCache;
            if ((date = mprLookupKey(dateCache, itosbuf(dbuf, sizeof(dbuf), (int64) info->mtime, 10))) == 0) {
                if (!dateCache || mprGetHashLength(dateCache) > 128) {
                    conn->http->dateCache = dateCache = mprCreateHash(0, 0);
                }
                date = httpGetDateString(&tx->fileInfo);
                mprAddKey(dateCache, itosbuf(dbuf, sizeof(dbuf), (int64) info->mtime, 10), date);
            }
            httpSetHeaderString(conn, "Last-Modified", date);
        }
        if (httpContentNotModified(conn)) {
            httpSetStatus(conn, HTTP_CODE_NOT_MODIFIED);
            httpOmitBody(conn);
            tx->length = -1;
        }
        if (!tx->fileInfo.isReg && !tx->fileInfo.isLink) {
            mprError("Document is not a regular file");
            httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot serve document");
            
        } else if (tx->fileInfo.size > conn->limits->transmissionBodySize) {
            httpError(conn, HTTP_ABORT | HTTP_CODE_REQUEST_TOO_LARGE,
                "Http transmission aborted. File size exceeds max body of %'lld bytes", 
                    conn->limits->transmissionBodySize);
            
        } else if (!(tx->connector == conn->http->sendConnector)) {
            /*
                If using the net connector, open the file if a body must be sent with the response. The file will be
                automatically closed when the request completes.
             */
            if (!(tx->flags & HTTP_TX_NO_BODY)) {
                tx->file = mprOpenFile(tx->filename, O_RDONLY | O_BINARY, 0);
                if (tx->file == 0) {
                    if (rx->referrer) {
                        mprLog(2, "fileHandler: Cannot find filename %s from referrer %s", tx->filename, rx->referrer);
                    } else {
                        mprLog(2, "fileHandler: Cannot find filename %s", tx->filename);
                    }
                    httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot find document");
                }
            }
        }
    } else if (rx->flags & (HTTP_DELETE | HTTP_OPTIONS | HTTP_PUT)) {
        ;
    } else {
        httpError(conn, HTTP_CODE_BAD_METHOD, "Unsupported method");
    }
}


static void closeFileHandler(HttpQueue *q)
{
    HttpTx  *tx;

    tx = q->conn->tx;
    if (tx->file) {
        mprCloseFile(tx->file);
        tx->file = 0;
    }
}


static void startFileHandler(HttpQueue *q)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpTx      *tx;
    HttpPacket  *packet;

    conn = q->conn;
    rx = conn->rx;
    tx = conn->tx;
    assert(!tx->finalized);
    
    if (rx->flags & HTTP_PUT) {
        handlePutRequest(q);
        
    } else if (rx->flags & HTTP_DELETE) {
        handleDeleteRequest(q);
        
    } else if (rx->flags & HTTP_OPTIONS) {
        httpHandleOptions(q->conn);
        
    } else if (!(tx->flags & HTTP_TX_NO_BODY)) {
        /* Create a single data packet based on the entity length */
        packet = httpCreateEntityPacket(0, tx->entityLength, readFileData);
        if (!tx->outputRanges) {
            /* Can set a content length */
            tx->length = tx->entityLength;
        }
        /* Add to the output service queue */
        httpPutForService(q, packet, 0);
    }
}


/*
    The ready callback is invoked when all body data has been received
 */
static void readyFileHandler(HttpQueue *q)
{
    /*
        The queue already contains a single data packet representing all the output data.
     */
    httpFinalize(q->conn);
}


/*  
    Populate a packet with file data. Return the number of bytes read or a negative error code. Will not return with
    a short read.
 */
static ssize readFileData(HttpQueue *q, HttpPacket *packet, MprOff pos, ssize size)
{
    HttpConn    *conn;
    HttpTx      *tx;
    ssize       nbytes;

    conn = q->conn;
    tx = conn->tx;

    if (packet->content == 0 && (packet->content = mprCreateBuf(size, -1)) == 0) {
        return MPR_ERR_MEMORY;
    }
    assert(size <= mprGetBufSpace(packet->content));    
    mprTrace(7, "readFileData size %zd, pos %lld", size, pos);
    
    if (pos >= 0) {
        mprSeekFile(tx->file, SEEK_SET, pos);
    }
    if ((nbytes = mprReadFile(tx->file, mprGetBufStart(packet->content), size)) != size) {
        /*  
            As we may have sent some data already to the client, the only thing we can do is abort and hope the client 
            notices the short data.
         */
        httpError(conn, HTTP_CODE_SERVICE_UNAVAILABLE, "Can't read file %s", tx->filename);
        return MPR_ERR_CANT_READ;
    }
    mprAdjustBufEnd(packet->content, nbytes);
    packet->esize -= nbytes;
    assert(packet->esize == 0);
    return nbytes;
}


/*  
    Prepare a data packet for sending downstream. This involves reading file data into a suitably sized packet. Return
    the 1 if the packet was sent entirely, return zero if the packet could not be completely sent. Return a negative
    error code for write errors. This may split the packet if it exceeds the downstreams maximum packet size.
 */
static int prepPacket(HttpQueue *q, HttpPacket *packet)
{
    HttpQueue   *nextQ;
    ssize       size, nbytes;

    if (mprNeedYield()) {
        httpScheduleQueue(q);
        return 0;
    }
    nextQ = q->nextQ;
    if (packet->esize > nextQ->packetSize) {
        httpPutBackPacket(q, httpSplitPacket(packet, nextQ->packetSize));
        size = nextQ->packetSize;
    } else {
        size = (ssize) packet->esize;
    }
    if ((size + nextQ->count) > nextQ->max) {
        /*  
            The downstream queue is full, so disable the queue and service downstream queue.
            Will re-enable via a writable event on the connection.
         */
        httpSuspendQueue(q);
        if (!(nextQ->flags & HTTP_QUEUE_SUSPENDED)) {
            httpScheduleQueue(nextQ);
        }
        return 0;
    }
    if ((nbytes = readFileData(q, packet, q->ioPos, size)) != size) {
        return MPR_ERR_CANT_READ;
    }
    q->ioPos += nbytes;
    return 1;
}


/*  
    The service callback will be invoked to service outgoing packets on the service queue. It will only be called 
    once all incoming data has been received and then when the downstream queues drain sufficiently to absorb 
    more data. This routine may flow control if the downstream stage cannot accept all the file data. It will
    then be re-called as required to send more data.
 */
static void outgoingFileService(HttpQueue *q)
{
    HttpConn    *conn;
    HttpTx      *tx;
    HttpPacket  *packet;
    bool        usingSend;
    int         rc;

    conn = q->conn;
    tx = conn->tx;
    usingSend = (tx->connector == conn->http->sendConnector);
    for (packet = httpGetPacket(q); packet; packet = httpGetPacket(q)) {
        if (!usingSend && !tx->outputRanges && packet->esize) {
            if ((rc = prepPacket(q, packet)) < 0) {
                return;
            } else if (rc == 0) {
                mprTrace(7, "OutgoingFileService downstream full, putback");
                httpPutBackPacket(q, packet);
                return;
            }
            mprTrace(7, "OutgoingFileService readData %d", rc);
        }
        httpPutPacketToNext(q, packet);
    }
    mprTrace(7, "OutgoingFileService complete");
}


/*
    The incoming callback is invoked to receive body data 
 */
static void incomingFile(HttpQueue *q, HttpPacket *packet)
{
    HttpConn    *conn;
    HttpTx      *tx;
    HttpRx      *rx;
    HttpRange   *range;
    MprBuf      *buf;
    MprFile     *file;
    ssize       len;

    conn = q->conn;
    tx = conn->tx;
    rx = conn->rx;
    file = (MprFile*) q->queueData;
    
    if (file == 0) {
        /*  Not a PUT so just ignore the incoming data.  */
        return;
    }
    if (httpGetPacketLength(packet) == 0) {
        /* End of input */
        if (file) {
            mprCloseFile(file);
        }
        q->queueData = 0;
        if (!tx->etag) {
            /* Set the etag for caching in the client */
            mprGetPathInfo(tx->filename, &tx->fileInfo);
            tx->etag = sfmt("\"%llx-%llx-%llx\"", tx->fileInfo.inode, tx->fileInfo.size, tx->fileInfo.mtime);
        }
        return;
    }
    buf = packet->content;
    len = mprGetBufLength(buf);
    assert(len > 0);

    range = rx->inputRange;
    if (range && mprSeekFile(file, SEEK_SET, range->start) != range->start) {
        httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't seek to range start to %lld", range->start);

    } else if (mprWriteFile(file, mprGetBufStart(buf), len) != len) {
        httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't PUT to %s", tx->filename);
    }
}


/*  
    This is called to setup for a HTTP PUT request. It is called before receiving the post data via incomingFileData
 */
static void handlePutRequest(HttpQueue *q)
{
    HttpConn    *conn;
    HttpTx      *tx;
    MprFile     *file;
    cchar       *path;

    assert(q->pair->queueData == 0);

    conn = q->conn;
    tx = conn->tx;
    assert(tx->filename);
    assert(tx->fileInfo.checked);

    path = tx->filename;
    if (tx->outputRanges) {
        /*  
            Open an existing file with fall-back to create
         */
        if ((file = mprOpenFile(path, O_BINARY | O_WRONLY, 0644)) == 0) {
            if ((file = mprOpenFile(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644)) == 0) {
                httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create the put URI");
                return;
            }
        } else {
            mprSeekFile(file, SEEK_SET, 0);
        }
    } else {
        if ((file = mprOpenFile(path, O_CREAT | O_TRUNC | O_BINARY | O_WRONLY, 0644)) == 0) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Can't create the put URI");
            return;
        }
    }
    if (!tx->fileInfo.isReg) {
        httpSetHeaderString(conn, "Location", conn->rx->uri);
    }
    httpSetStatus(conn, tx->fileInfo.isReg ? HTTP_CODE_NO_CONTENT : HTTP_CODE_CREATED);
    q->pair->queueData = (void*) file;
}


static void handleDeleteRequest(HttpQueue *q)
{
    HttpConn    *conn;
    HttpTx      *tx;

    conn = q->conn;
    tx = conn->tx;
    assert(tx->filename);
    assert(tx->fileInfo.checked);

    if (!tx->fileInfo.isReg) {
        httpError(conn, HTTP_CODE_NOT_FOUND, "Document not found");
        return;
    }
    if (mprDeletePath(tx->filename) < 0) {
        httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot remove document");
        return;
    }
    httpSetStatus(conn, HTTP_CODE_NO_CONTENT);
}


static int manageDir(HttpConn *conn)
{
    HttpRx      *rx;
    HttpTx      *tx;
    HttpRoute   *route;
    HttpUri     *req;
    MprPath     *info;
    cchar       *index, *pathInfo, *uri;
    char        *path;
    int         next;

    rx = conn->rx;
    tx = conn->tx;
    req = rx->parsedUri;
    route = rx->route;
    info = &tx->fileInfo;

    /*
        Manage requests for directories
     */
    if (!sends(req->path, "/")) {
        /*
           Append "/" and do an external redirect. Use the original request URI.
         */
        pathInfo = sjoin(req->path, "/", NULL);
        uri = httpFormatUri(req->scheme, req->host, req->port, pathInfo, req->reference, req->query, 0);
        httpRedirect(conn, HTTP_CODE_MOVED_PERMANENTLY, uri);
        return HTTP_ROUTE_OK;
    }
    if (route->indicies) {
        /*
            Ends with a "/" so do internal redirection to an index file
         */
        for (ITERATE_ITEMS(route->indicies, index, next)) {
            /*
                Internal directory redirections. Transparently append index. Test indicies in order.
             */
            path = mprJoinPath(tx->filename, index);
            if (mprPathExists(path, R_OK)) {
                pathInfo = sjoin(rx->scriptName, rx->pathInfo, index, NULL);
                uri = httpFormatUri(req->scheme, req->host, req->port, pathInfo, req->reference, req->query, 0);
                httpSetUri(conn, uri);
                tx->filename = path;
                tx->ext = httpGetExt(conn);
                mprGetPathInfo(tx->filename, info);
                return HTTP_ROUTE_REROUTE;
            }
        }
    }
#if ME_COM_DIR
    /*
        Directory Listing. If a directory, test if a directory listing should be rendered. If so, delegate to the
        dirHandler. Cannot use the sendFile handler and must use the netConnector.
     */
    if (info->isDir && maRenderDirListing(conn)) {
        tx->handler = conn->http->dirHandler;
        tx->connector = conn->http->netConnector;
        return HTTP_ROUTE_OK;
    }
#endif
    return HTTP_ROUTE_OK;
}


/*  
    Loadable module initialization
 */
PUBLIC int maOpenFileHandler(Http *http)
{
    HttpStage     *handler;

    /* 
        This handler serves requests without using thread workers.
     */
    if ((handler = httpCreateHandler(http, "fileHandler", NULL)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    handler->rewrite = rewriteFileHandler;
    handler->open = openFileHandler;
    handler->close = closeFileHandler;
    handler->start = startFileHandler;
    handler->ready = readyFileHandler;
    handler->outgoingService = outgoingFileService;
    handler->incoming = incomingFile;
    http->fileHandler = handler;
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
