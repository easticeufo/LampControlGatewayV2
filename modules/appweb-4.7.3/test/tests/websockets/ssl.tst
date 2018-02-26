/*
    ssl.tst - WebSocket over SSL tests
 */

if (!Config.SSL) {
    test.skip("SSL not enabled in ejs")

} else if (!App.config.me_ssl) {
    test.skip("SSL not enabled")
} else {
    const HTTPS = App.config.test.http_port || "https://localhost:4443"
    var WS = HTTPS.replace('https', 'wss') + '/websockets/basic/ssl'
    const TIMEOUT = 5000

    assert(WebSocket)
    let ws = new WebSocket(WS, ['chat'], { verify: false })
    assert(ws)
    assert(ws.readyState == WebSocket.CONNECTING)

    let closed, opened, response
    let msg = "Hello Server"
    ws.onopen = function (event) {
        opened = true
        ws.send(msg)
    }
    ws.onclose = function (event) {
        closed = true
    }
    ws.onmessage = function (event) {
        assert(event.data is String)
        response = event.data
        ws.close()
    }
    ws.wait(WebSocket.OPEN, TIMEOUT)
    assert(opened)

    ws.wait(WebSocket.CLOSED, TIMEOUT)
    assert(ws.readyState == WebSocket.CLOSED)
    assert(closed)

    //  Text == 1, last == 1, first 10 data bytes
    let info = deserialize(response)
    assert(info.type == 1)
    assert(info.last == 1)
    assert(info.length == msg.length)
    assert(info.data == "Hello Serv")
}
