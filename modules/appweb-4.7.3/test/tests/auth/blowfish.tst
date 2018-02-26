/*
    blowfish.tst - Blowfish cipher authentication tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"

let http: Http = new Http

http.setCredentials("ralph", "pass5")
http.get(HTTP + "/auth/blowfish/ralph.html")
assert(http.status == 200)
assert(http.response.contains('Welcome to Blowfish Basic - Access for ralph'))
http.close()
