/*
    get.tst - Http GET tests
 */

const HTTP = App.config.uris.http || "127.0.0.1:4100"
let http: Http = new Http

//  Basic get. Validate response code and contents
http.get(HTTP + "/index.html")
assert(http.status == 200)
// assert(http.readString().contains("Hello"))

//  Validate get contents
http.get(HTTP + "/index.html")
assert(http.readString(12) == "<html><head>")
assert(http.readString(7) == "<title>")

//  Validate get contents
http.get(HTTP + "/index.html")
assert(http.response.endsWith("</html>\n"))

//  Test Get with a body. Yes this is valid Http, although unusual.
http.get(HTTP + "/index.html", 'name=John&address=700+Park+Ave')
assert(http.status == 200)
http.close()

if (test.os == "WIN") {
    http.get(HTTP + "/inDEX.htML")
    assert(http.status == 200)
    http.close()
}
