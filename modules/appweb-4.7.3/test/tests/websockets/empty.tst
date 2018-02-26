/*
    empty.tst - Test an empty message
 */

const PORT = App.config.test.http_port || "4100"
const WS = "ws://127.0.0.1:" + PORT + "/websockets/basic/empty"
const TIMEOUT = 5000 * 1000

assert(WebSocket)
let ws = new WebSocket(WS)
assert(ws)
assert(ws.readyState == WebSocket.CONNECTING)

let gotMsg, gotClose, gotError
ws.onmessage = function (event) {
    assert(event.data is String)
    assert(event.data == "")
    assert(event.last === true)
    assert(event.type == 1)
    gotMsg = true
}

ws.onclose = function (event) {
    assert(gotMsg)
    ws.close()
    gotClose = true
}
ws.onerror = function (event) {
    print("ON ERROR")
    gotError = true
}

ws.wait(WebSocket.OPEN, TIMEOUT)
ws.send("Hello")
ws.wait(WebSocket.CLOSED, TIMEOUT)
assert(ws.readyState == WebSocket.CLOSED)

assert(gotMsg)
assert(gotClose)
assert(!gotError)
