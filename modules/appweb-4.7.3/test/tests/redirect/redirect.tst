/*
    redirect.tst - Redirect directive
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http


//  Standard error messages
http.get(HTTP + "/old.html")
assert(http.status == 302)
assert(http.response.contains("<h1>Moved Temporarily</h1>"))
http.close()

http = new Http
http.followRedirects = true
http.get(HTTP + "/old/html")
assert(http.status == 200)
assert(http.response.contains("<title>index.html</title>"))
http.close()

http = new Http
http.get(HTTP + "/membersOnly")
assert(http.status == 410)
http.close()

