/*
    legacy-mvc.tst - Legacy ESP MVC tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http
let prefix = HTTP + "/legacy"

//  /legacy
http.get(prefix)
assert(http.status == 200)
assert(http.response.contains("Welcome to ESP"))
http.close()

//  /legacy/
http.get(prefix + "/")
assert(http.status == 200)
assert(http.response.contains("Welcome to ESP"))
http.close()

//  /legacy/static/css/all.css
http.get(prefix + "/static/css/all.css")
assert(http.status == 200)
assert(http.response.contains("Default layout style sheet"))
http.close()

//  /legacy/static/index.esp
http.get(prefix + "/static/index.esp")
assert(http.status == 200)
assert(http.response.contains("Welcome to ESP"))
http.close()

//  /legacy/post/noview - this tests a controller without view
http.post(prefix + "/post/noview")
assert(http.status == 200)
assert(http.response.contains("Check: OK"))
http.close()

//  /legacy/post/details - this tests templates
http.post(prefix + "/post/view")
assert(http.status == 200)
assert(http.response.contains("<title>Test App</title>"))
assert(http.response.contains("Powered by Appweb"))
assert(http.response.contains("SECRET: 42"))
http.close()
