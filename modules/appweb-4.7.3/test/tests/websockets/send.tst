/*
    send.tst - WebSocket send test
 */

const PORT = App.config.test.http_port || "4100"
const WS = "ws://127.0.0.1:" + PORT + "/websockets/basic/send"
const TIMEOUT = 5000

assert(WebSocket)
let ws = new WebSocket(WS)
assert(ws)
assert(ws.readyState == WebSocket.CONNECTING)

let opened
ws.onopen = function (event) {
    opened = true
    ws.send("Hello World")
}

ws.wait(WebSocket.OPEN, TIMEOUT)
assert(opened)

ws.close()
assert(ws.readyState == WebSocket.CLOSING)
ws.wait(WebSocket.CLOSED, TIMEOUT)
assert(ws.readyState == WebSocket.CLOSED)
