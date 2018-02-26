/*
    vhost.tst - Virtual Host tests
 */

blend(App.config.uris, {
  http: "http://127.0.0.1:4100",
  named:"http://127.0.0.1:4200",
  virt: "http://127.0.0.1:4300",
}, {overwrite: false})

const HTTP = App.config.uris.http
const NAMED = App.config.uris.named
const VIRT = App.config.uris.virt

let http: Http = new Http

function mainHost() {
    http.get(HTTP + "/main.html")
    http.response.contains("HTTP SERVER")
    assert(http.status == 200)

    //  These should fail
    http.get(HTTP + "/iphost.html")
    assert(http.status == 404)
    http.get(HTTP + "/vhost1.html")
    assert(http.status == 404)
    http.get(HTTP + "/vhost2.html")
    assert(http.status == 404)
    http.close()
}


//  The first virtual host listens to "localhost", the second listens to HTTP.

function namedHost() {
    http = new Http
    http.setHeader("Host", "localhost:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost1.html")
    assert(http.status == 200)
    http.close()

    //  These should fail
    http = new Http
    http.setHeader("Host", "localhost:" + Uri(NAMED).port)
    http.get(NAMED + "/main.html")
    assert(http.status == 404)
    http.close()

    http = new Http
    http.setHeader("Host", "localhost:" + Uri(NAMED).port)
    http.get(NAMED + "/iphost.html")
    assert(http.status == 404)
    http.close()

    http = new Http
    http.setHeader("Host", "localhost:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost2.html")
    assert(http.status == 404)
    http.close()

    //  Now try the second vhost on 127.0.0.1
    http = new Http
    http.setHeader("Host", "127.0.0.1:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost2.html")
    assert(http.status == 200)
    http.close()

    //  These should fail
    http = new Http
    http.setHeader("Host", "127.0.0.1:" + Uri(NAMED).port)
    http.get(NAMED + "/main.html")
    assert(http.status == 404)
    http.close()

    http = new Http
    http.setHeader("Host", "127.0.0.1:" + Uri(NAMED).port)
    http.get(NAMED + "/iphost.html")
    assert(http.status == 404)
    http.close()

    http = new Http
    http.setHeader("Host", "127.0.0.1:" + Uri(NAMED).port)
    http.get(NAMED + "/vhost1.html")
    assert(http.status == 404)
    http.close()
}

function ipHost() {
    let http: Http = new Http
    http.setCredentials("mary", "pass2")
    http.get(VIRT + "/private.html")
    assert(http.status == 200)
    http.close()
}

mainHost()
namedHost()
ipHost()
