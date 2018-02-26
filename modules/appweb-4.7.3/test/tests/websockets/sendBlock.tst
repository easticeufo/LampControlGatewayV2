/*
    sendBlock.tst - WebSocket sendBlock API test
 */

const PORT = App.config.test.http_port || "4100"
const WS = "ws://127.0.0.1:" + PORT + "/websockets/basic/len"
const TIMEOUT = 5000
const LEN = 10 * 1024 * 1024

let data = new ByteArray(LEN)
for (i in LEN / 50) {
    data.write("01234567890123456789012345678901234567890123456789")
}
let dataLength = data.length

assert(WebSocket)
let ws = new WebSocket(WS)
assert(ws)
assert(ws.readyState == WebSocket.CONNECTING)


let opened
ws.onopen = function (event) {
    opened = true
}
ws.onerror = function (event) {
    opened = false
}

let msg
ws.onmessage = function (event) {
    assert(event.data is String)
    msg = event.data
}

function sendBlock(data, options) {
    let mark = new Date
    msg = null
    options.type = WebSocket.MSG_TEXT
    do {
        let rc = ws.sendBlock(data, options)
        data.readPosition += rc
        options.type = WebSocket.MSG_CONT
        /*
            Normally this would be event based, but for a simple unit test, we just wait
         */
        App.sleep(10);
    } while (data.length > 0 && opened)
    while (opened && !msg && mark.elapsed < TIMEOUT) {
        App.run(10, true)
    }
    data.readPosition = 0
    return msg
}

ws.wait(WebSocket.OPEN, TIMEOUT)
assert(opened)

/*
    Non-blocking test
 */
response = sendBlock(data, { mode: WebSocket.NON_BLOCK })
let info = deserialize(response)
assert(info.length == dataLength)
assert(info.type == 1)
assert(info.last == 1)

/*
    Buffered test
 */
response = sendBlock(data, { mode: WebSocket.BUFFER })
let info = deserialize(response)
assert(info.length == dataLength)
assert(info.type == 1)
assert(info.last == 1)

/*
    Buffered test
 */
response = sendBlock(data, { mode: WebSocket.BLOCK })
let info = deserialize(response)
assert(info.length == dataLength)
assert(info.type == 1)
assert(info.last == 1)

ws.close()
assert(ws.readyState == WebSocket.CLOSING)
ws.wait(WebSocket.CLOSED, TIMEOUT)
assert(ws.readyState == WebSocket.CLOSED)
