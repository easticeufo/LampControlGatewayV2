/*
    len-1500.tst - Test a big single frame message

    WebSockets uses one byte for a length <= 125 bytes
 */

const PORT = App.config.test.http_port || "4100"
const WS = "ws://127.0.0.1:" + PORT + "/websockets/basic/len"
const TIMEOUT = 5000 * 1000
const LEN = 1500

assert(WebSocket)
let ws = new WebSocket(WS)
assert(ws)
assert(ws.readyState == WebSocket.CONNECTING)

let response
ws.onmessage = function (event) {
    assert(event.data is String)
    response = event.data
    ws.close()
}

ws.wait(WebSocket.OPEN, TIMEOUT)
let msg = "0123456789".times(LEN / 10)
ws.send(msg)

ws.wait(WebSocket.CLOSED, TIMEOUT)
assert(ws.readyState == WebSocket.CLOSED)

//  Text == 1, last == 1, first 10 data bytes
let info = deserialize(response)
assert(info.type == 1)
assert(info.last == 1)
assert(info.length == LEN)
assert(info.data == "0123456789")
