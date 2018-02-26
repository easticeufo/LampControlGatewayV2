/*
    output.c - WebSockets output sample.

    This sample demonstrates writing large, streaming response without blocking, buffering or consuming excessive memory.
    This sends a large file as a single web socket message using multiple frames.
 */
#include "esp.h"

#define OUTPUT_FILE "file.txt"

typedef struct Output {
    MprFile     *file;      /* Open file handle */
    MprPath     info;       /* File stat - size */
    ssize       written;    /* Number of bytes written so far */
} Output;

/*
    Event callback. Invoked for incoming web socket messages and other events of interest.
    We're interested in the WRITABLE event.
 */
static void output_callback(HttpConn *conn, int event, int arg)
{
    Output      *output;
    ssize       len, wrote;
    int         flags, type;
    char        buf[MPR_BUFSIZE];

    /*
        Get a writable event when the socket can absorb more data
     */
    if (event == HTTP_EVENT_WRITABLE) {
        output = getData();
        do {
            if ((len = mprReadFile(output->file, buf, sizeof(buf))) > 0) {

                /*
                    Set the HTTP_MORE flag on every write except the last. This means each write is sent as
                    a separate frame. The first frame has the type of WS_MSG_TEXT, all others must be 
                    continuation frames.
                 */
                flags = HTTP_NON_BLOCK;
                if ((output->written + len) < output->info.size) {
                    flags |= HTTP_MORE;
                }
                type = output->written == 0 ? WS_MSG_TEXT : WS_MSG_CONT;
                /*
                    Send the next chunk as a WebSockets frame using a non-blocking write. 
                    This may return having written only a portion of the requested data.
                 */
                if ((wrote = httpSendBlock(conn, type, buf, len, flags)) < 0) {
                    httpError(conn, HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot send message of %d bytes", len);
                    return;
                }
                output->written += wrote;
                if (wrote < len) {
                    /* Reposition if the send returned having written less than requested */
                    mprSeekFile(output->file, SEEK_CUR, wrote - len);
                    break;
                }
            } else {
                httpSendClose(conn, WS_STATUS_OK, "OK");
                break;
            }
        } while (len > 0);

    } else if (event == HTTP_EVENT_APP_CLOSE) {
        mprLog(0, "output.c: close event. Status status %d, orderly closed %d, reason %s", arg,
        httpWebSocketOrderlyClosed(conn), httpGetWebSocketCloseReason(conn));

    } else if (event == HTTP_EVENT_ERROR) {
        mprLog(0, "output.c: error event");
    }
}


static void manageOutput(Output *output, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(output->file);
    } else if (flags & MPR_MANAGE_FREE) {
        /* Let GC close the file for us automatically */
    }
}


/*
    Action to run in response to the "test/output" URI
 */
static void output_action() 
{ 
    Output  *output;

    /*
        Don't automatically finalize (complete) the request when this routine returns. This keeps the connection open.
     */
    dontAutoFinalize();

    /*
        Define the event notifier. We're interested in WRITABLE events
     */
    setNotifier(output_callback);

    /*
        Open a file for output. Could use open/write, but we use the MPR equivalents for cross-platform I/O.
     */
    output = mprAllocObj(Output, manageOutput);
    if ((output->file = mprOpenFile(OUTPUT_FILE, O_RDONLY, 0)) == 0) {
        httpError(getConn(), HTTP_CODE_INTERNAL_SERVER_ERROR, "Cannot open huge.txt");
        return;
    }
    mprGetPathInfo(OUTPUT_FILE, &output->info);
    /*
        Save a reference to our output state
     */ 
    setData(output);
}


/*
    Initialize the "output" loadable module
 */
ESP_EXPORT int esp_controller_output(HttpRoute *route, MprModule *module) {
    /*
        Define the "output" action that will run when the "test/output" URI is invoked
     */
    espDefineAction(route, "test-output", output_action);
    return 0;
}
