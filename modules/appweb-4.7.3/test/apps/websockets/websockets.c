/*
    websockets.c - Test WebSockets
 */
#include "esp.h"

/*
    Diagnostic trace for tests
 */
static void traceEvent(HttpConn *conn, int event, int arg)
{
    HttpPacket  *packet;

    if (event == HTTP_EVENT_READABLE) {
        /*
            Peek at the readq rather than doing httpGetPacket()
            The last frame in a message has packet->last == true
         */
        packet = conn->readq->first;
        mprLog(3, "websock.c: read %s event, last %d", packet->type == WS_MSG_TEXT ? "text" : "binary", packet->last);
        mprLog(3, "websock.c: read: (start of data only) \"%s\"", snclone(mprGetBufStart(packet->content), 40));

    } else if (event == HTTP_EVENT_APP_CLOSE) {
        mprLog(3, "websock.c: close event. Status status %d, orderly closed %d, reason %s", arg,
            httpWebSocketOrderlyClosed(conn), httpGetWebSocketCloseReason(conn));

    } else if (event == HTTP_EVENT_ERROR) {
        mprLog(2, "websock.c: error event");
    }
}

static void dummy_callback(HttpConn *conn, int event, int arg)
{
}

static void dummy_action() { 
    dontAutoFinalize();
    espSetNotifier(getConn(), dummy_callback);
}

static void len_callback(HttpConn *conn, int event, int arg)
{
    HttpPacket      *packet;
    HttpWebSocket   *ws;

    traceEvent(conn, event, arg);
    if (event == HTTP_EVENT_READABLE) {
        /*
            Get and discard the packet. traceEvent will have traced it for us.
         */
        packet = httpGetPacket(conn->readq);
        assert(packet);
        /* 
            Ignore precedding packets and just respond and echo the last 
         */
        if (packet->last) {
            ws = conn->rx->webSocket;
            httpSend(conn, "{type: %d, last: %d, length: %d, data: \"%s\"}\n", packet->type, packet->last,
                ws->messageLength, snclone(mprGetBufStart(packet->content), 10));
        }
    }
}

static void len_action() { 
    dontAutoFinalize();
    espSetNotifier(getConn(), len_callback);
}


/*
    Autobahn test echo server
    Must configure LimitWebSocketsPacket to be larger than the biggest expected message so we receive complete messages.
    Otherwise, we need to buffer and aggregate messages here.
 */
static void echo_callback(HttpConn *conn, int event, int arg)
{
    HttpPacket  *packet;

    if (event == HTTP_EVENT_READABLE) {
        packet = httpGetPacket(conn->readq);
        if (packet->type == WS_MSG_TEXT || packet->type == WS_MSG_BINARY) {
            httpSendBlock(conn, packet->type, httpGetPacketStart(packet), httpGetPacketLength(packet), 0);
        }
    }
}

static void echo_action() { 
    dontAutoFinalize();
    espSetNotifier(getConn(), echo_callback);
}


/*
    Test sending an empty text message, followed by an orderly close
 */
static void empty_response() 
{
    httpSendBlock(getConn(), WS_MSG_TEXT, "", 0, 0);
    httpSendClose(getConn(), WS_STATUS_OK, "OK");
}


/*
    Big single message written with one send(). The WebSockets filter will break this into frames as required.
 */
static void big_response() 
{
    HttpConn    *conn;
    MprBuf      *buf;
    ssize       wrote;
    int         i, count;

    conn = getConn();
    count = 10000;

    /*
        First message is big, but in a single send. The middleware should break this into frames unless you call:
            httpSetWebSocketPreserveFrames(conn, 1);
        This will regard each call to httpSendBlock as a frame.
        NOTE: this is not an ideal pattern (so don't copy it).
     */
    buf = mprCreateBuf(0, 0);
    for (i = 0; i < count; i++) {
        mprPutToBuf(buf, "%8d:01234567890123456789012345678901234567890\n", i);
    }
    mprAddNullToBuf(buf);
    /* Retain just for GC */
    httpSetWebSocketData(conn, buf);
    
    /*
        Note: this will block while writing the entire message. It may be quicker to use HTTP_BUFFER but will use more memory.
        Not point using HTTP_NON_BLOCK as we need to close after sending the message.
     */
    if ((wrote = httpSendBlock(conn, WS_MSG_TEXT, mprGetBufStart(buf), mprGetBufLength(buf), HTTP_BLOCK)) < 0) {
        httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot send big message");
        return;
    }
    httpSendClose(conn, WS_STATUS_OK, "OK");
}

/*
    Multiple-frame response message with explicit continuations.
    The WebSockets filter will encode each call to httpSendBlock into a frame. 
    Even if large blocks are written, HTTP_MORE assures that the block will be encoded as a single frame.
 */
static void frames_response() 
{
    HttpConn    *conn;
    cchar       *str;
    int         i, more, count;

    conn = getConn();
    count = 1000;

    for (i = 0; i < count; i++) {
        str = sfmt("%8d: Hello\n", i);
        more = (i < (count - 1)) ? HTTP_MORE : 0;
        if (httpSendBlock(conn, WS_MSG_TEXT, str, slen(str), HTTP_BUFFER | more) < 0) {
            httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot send message: %d", i);
            return;
        }
    }
    httpSendClose(conn, WS_STATUS_OK, "OK");
}


ESP_EXPORT int esp_controller_websockets(HttpRoute *route, MprModule *module) {
    espDefineAction(route, "basic-construct", dummy_action);
    espDefineAction(route, "basic-open", dummy_action);
    espDefineAction(route, "basic-send", dummy_action);
    espDefineAction(route, "basic-echo", echo_action);
    espDefineAction(route, "basic-ssl", len_action);
    espDefineAction(route, "basic-len", len_action);
    espDefineAction(route, "basic-echo", echo_action);
    espDefineAction(route, "basic-empty", empty_response);
    espDefineAction(route, "basic-big", big_response);
    espDefineAction(route, "basic-frames", frames_response);
    return 0;
}
