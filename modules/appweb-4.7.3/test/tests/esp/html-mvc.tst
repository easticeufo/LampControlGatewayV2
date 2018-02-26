/*
    html-mvc.tst - ESP html-mvc tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http
let prefix = HTTP + "/html"

//  /html
http.followRedirects = true
http.get(prefix)
assert(http.status == 200)
assert(http.response.contains("<h1>Welcome to Embedded Server Pages</h1>"))
http.close()

//  /html/
http.get(prefix + "/")
assert(http.status == 200)
assert(http.response.contains("<h1>Welcome to Embedded Server Pages</h1>"))
http.close()

//  /html/index.esp
http.get(prefix + "/index.esp")
assert(http.status == 200)
assert(http.response.contains("<h1>Welcome to Embedded Server Pages</h1>"))
http.close()

//  /html/all.less
http.get(prefix + "/css/all.less")
assert(http.status == 200)
assert(http.response.contains("Aggregate all stylesheets"))
http.close()

//  /html/do/post/init - this tests a controller without view
http.get(prefix + "/do/post/init")
assert(http.status == 200)
assert(http.response.contains('<h1>Create Post</h1>'))
http.close()
