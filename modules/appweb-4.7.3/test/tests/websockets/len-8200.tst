/*
    len-8200.tst - Test a just bigger than an 8K frame

    WebSockets uses one byte for a length <= 125 bytes
 */

const PORT = App.config.test.http_port || "4100"
const WS = "ws://127.0.0.1:" + PORT + "/websockets/basic/len"
const TIMEOUT = 5000
const LEN = 8200

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

/* UNUSED
ws.onclose = function (event) {
    print("ON CLOSE")
}
ws.onerror = function (event) {
    print("ON ERROR")
}
*/

ws.wait(WebSocket.OPEN, TIMEOUT)

let data = new ByteArray(LEN)
for (i in LEN / 50) {
    data.write("01234567890123456789012345678901234567890123456789")
}
ws.send(data)

ws.wait(WebSocket.CLOSED, TIMEOUT)
assert(ws.readyState == WebSocket.CLOSED)

//  Binary == 2, last == 0, first 10 data bytes
let info = deserialize(response)
assert(info.type == 2)
assert(info.last == 1)
assert(info.length == data.length)
assert(info.data == "23456789")
