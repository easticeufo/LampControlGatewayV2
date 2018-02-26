/*
    echo.c - WebSockets echo server
 */
#include "esp.h"

/*
    Event callback. Invoked for incoming web socket messages and other events of interest.
 */
static void echo_callback(HttpConn *conn, int event, int arg)
{
    HttpPacket      *packet;

    if (event == HTTP_EVENT_READABLE) {
        /*
            Grab the packet off the read queue.
         */
        packet = httpGetPacket(conn->readq);
        if (packet->type == WS_MSG_TEXT || packet->type == WS_MSG_BINARY) {
            /*
                Echo back the contents
             */
            httpSendBlock(conn, packet->type, httpGetPacketStart(packet), httpGetPacketLength(packet), 0);
        }
    } else if (event == HTTP_EVENT_APP_CLOSE) {
        mprLog(0, "echo.c: close event. Status status %d, orderly closed %d, reason %s", arg,
        httpWebSocketOrderlyClosed(conn), httpGetWebSocketCloseReason(conn));

    } else if (event == HTTP_EVENT_ERROR) {
        mprLog(0, "echo.c: error event");
    }
}


/*
    Action to run in response to the "test/echo" URI
 */
static void echo_action() { 
    /*
        Don't automatically finalize (complete) the request when this routine returns. This keeps the connection open.
     */
    dontAutoFinalize();

    /*
        Establish the event callback
     */
    espSetNotifier(getConn(), echo_callback);
}


/*
    Initialize the "echo" loadable module
 */
ESP_EXPORT int esp_controller_echo(HttpRoute *route, MprModule *module) {
    /*
        Define the "echo" action that will run when the "test/echo" URI is invoked
     */
    espDefineAction(route, "test-echo", echo_action);
    return 0;
}
