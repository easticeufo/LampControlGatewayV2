/*
    header.tst - Http response header tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
const URL = HTTP + "/index.html"
let http: Http = new Http

http.get(HTTP + "/index.html")
connection = http.header("Connection")
assert(connection == "Keep-Alive")

http.get(HTTP + "/index.html")
assert(http.statusMessage == "OK")
assert(http.contentType == "text/html")
assert(http.date != "")
assert(http.lastModified != "")

http.post(HTTP + "/index.html")
assert(http.status == 200)

//  Request headers
http.reset()
http.setHeader("key", "value")
http.get(HTTP + "/index.html")
assert(http.status == 200)
http.close()
